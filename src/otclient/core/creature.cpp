/*
 * Copyright (c) 2010-2012 OTClient <https://github.com/edubart/otclient>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "creature.h"
#include "thingstype.h"
#include "localplayer.h"
#include "map.h"
#include "tile.h"
#include "item.h"
#include "game.h"
#include "effect.h"

#include <framework/graphics/graphics.h>
#include <framework/core/eventdispatcher.h>
#include <framework/core/clock.h>

#include <framework/graphics/paintershaderprogram.h>
#include <framework/graphics/paintershadersources.h>
#include <framework/graphics/texturemanager.h>
#include "spritemanager.h"

Creature::Creature() : Thing()
{
    m_healthPercent = 0;
    m_showVolatileSquare = false;
    m_showStaticSquare = false;
    m_direction = Otc::South;
    m_walkTimePerPixel = 1000.0/32.0;

    m_walking = false;
    m_preWalking = false;

    m_skull = Otc::SkullNone;
    m_shield = Otc::ShieldNone;
    m_emblem = Otc::EmblemNone;
    m_shieldBlink = false;
    m_showShieldTexture = true;

    m_informationFont = g_fonts.getFont("verdana-11px-rounded");
}

PainterShaderProgramPtr outfitProgram;
int HEAD_COLOR_UNIFORM = 10;
int BODY_COLOR_UNIFORM = 11;
int LEGS_COLOR_UNIFORM = 12;
int FEET_COLOR_UNIFORM = 13;
int MASK_TEXTURE_UNIFORM = 14;

void Creature::draw(const Point& p, const Rect&)
{
    if(m_showVolatileSquare) {
        g_painter.setColor(m_volatileSquareColor);
        g_painter.drawBoundingRect(Rect(p + m_walkOffset - Point(m_type->parameters[ThingType::DisplacementX], m_type->parameters[ThingType::DisplacementY]) + 3, Size(28, 28)), 2);
    }

    if(m_showStaticSquare) {
        g_painter.setColor(m_staticSquareColor);
        g_painter.drawBoundingRect(Rect(p + m_walkOffset - Point(m_type->parameters[ThingType::DisplacementX], m_type->parameters[ThingType::DisplacementY]) + 1, Size(32, 32)), 2);
    }

    g_painter.setColor(Fw::white);
    if(!outfitProgram) {
        outfitProgram = PainterShaderProgramPtr(new PainterShaderProgram);
        outfitProgram->addShaderFromSourceCode(Shader::Vertex, glslMainWithTexCoordsVertexShader + glslPositionOnlyVertexShader);
        outfitProgram->addShaderFromSourceFile(Shader::Fragment, "/game_shaders/outfit.frag");
        assert(outfitProgram->link());
        outfitProgram->bindUniformLocation(HEAD_COLOR_UNIFORM, "headColor");
        outfitProgram->bindUniformLocation(BODY_COLOR_UNIFORM, "bodyColor");
        outfitProgram->bindUniformLocation(LEGS_COLOR_UNIFORM, "legsColor");
        outfitProgram->bindUniformLocation(FEET_COLOR_UNIFORM, "feetColor");
        outfitProgram->bindUniformLocation(MASK_TEXTURE_UNIFORM, "maskTexture");
    }

    // Render creature
    if(m_outfit.getCategory() == ThingsType::Creature) {
        for(m_yPattern = 0; m_yPattern < m_type->dimensions[ThingType::PatternY]; m_yPattern++) {

            // continue if we dont have this addon.
            if(m_yPattern > 0 && !(m_outfit.getAddons() & (1 << (m_yPattern-1))))
                continue;

            g_painter.setCustomProgram(outfitProgram);

            outfitProgram->bind();
            outfitProgram->setUniformValue(HEAD_COLOR_UNIFORM, m_outfit.getHeadColor());
            outfitProgram->setUniformValue(BODY_COLOR_UNIFORM, m_outfit.getBodyColor());
            outfitProgram->setUniformValue(LEGS_COLOR_UNIFORM, m_outfit.getLegsColor());
            outfitProgram->setUniformValue(FEET_COLOR_UNIFORM, m_outfit.getFeetColor());

            for(int h = 0; h < m_type->dimensions[ThingType::Height]; h++) {
                for(int w = 0; w < m_type->dimensions[ThingType::Width]; w++) {
                    int spriteId = m_type->getSpriteId(w, h, 0, m_xPattern, m_yPattern, m_zPattern, m_animation);
                    if(!spriteId)
                        continue;
                    TexturePtr spriteTex = g_sprites.getSpriteTexture(spriteId);
                    if(!spriteTex)
                        continue;

                    if(m_type->dimensions[ThingType::Layers] > 1) {
                        int maskId = m_type->getSpriteId(w, h, 1, m_xPattern, m_yPattern, m_zPattern, m_animation);
                        TexturePtr maskTex = g_sprites.getSpriteTexture(maskId);
                        outfitProgram->setUniformTexture(MASK_TEXTURE_UNIFORM, maskTex, 1);
                    }

                    Rect drawRect(((p + m_walkOffset).x - w*32) - m_type->parameters[ThingType::DisplacementX],
                                ((p + m_walkOffset).y - h*32) - m_type->parameters[ThingType::DisplacementY],
                                32, 32);
                    g_painter.drawTexturedRect(drawRect, spriteTex);
                }
            }

            g_painter.releaseCustomProgram();
        }
    }
    else if(m_outfit.getCategory() == ThingsType::Item) {
        for(int l = 0; l < m_type->dimensions[ThingType::Layers]; l++)
            internalDraw(p + m_walkOffset, l);
    }
    else if(m_outfit.getCategory() == ThingsType::Effect)
        internalDraw(p + m_walkOffset, 0);
}

void Creature::drawInformation(int x, int y, bool useGray, const Rect& visibleRect)
{
    Color fillColor = Color(96, 96, 96);

    if(!useGray)
        fillColor = m_informationColor;

    // calculate main rects
    Rect backgroundRect = Rect(x-(13.5), y, 27, 4);
    backgroundRect.bound(visibleRect);

    Rect textRect = Rect(x - m_nameSize.width() / 2.0, y-12, m_nameSize);
    textRect.bound(visibleRect);

    // distance them
    if(textRect.top() == visibleRect.top())
        backgroundRect.moveTop(textRect.top() + 12);
    if(backgroundRect.bottom() == visibleRect.bottom())
        textRect.moveTop(backgroundRect.top() - 12);

    // health rect is based on background rect, so no worries
    Rect healthRect = backgroundRect.expanded(-1);
    healthRect.setWidth((m_healthPercent / 100.0) * 25);

    // draw
    g_painter.setColor(Fw::black);
    g_painter.drawFilledRect(backgroundRect);

    g_painter.setColor(fillColor);
    g_painter.drawFilledRect(healthRect);

    if(m_informationFont)
        m_informationFont->renderText(m_name, textRect, Fw::AlignTopCenter, fillColor);

    if(m_skull != Otc::SkullNone && m_skullTexture) {
        g_painter.setColor(Fw::white);
        g_painter.drawTexturedRect(Rect(x + 12, y + 5, m_skullTexture->getSize()), m_skullTexture);
    }
    if(m_shield != Otc::ShieldNone && m_shieldTexture && m_showShieldTexture) {
        g_painter.setColor(Fw::white);
        g_painter.drawTexturedRect(Rect(x, y + 5, m_shieldTexture->getSize()), m_shieldTexture);
    }
    if(m_emblem != Otc::EmblemNone && m_emblemTexture) {
        g_painter.setColor(Fw::white);
        g_painter.drawTexturedRect(Rect(x + 12, y + 16, m_emblemTexture->getSize()), m_emblemTexture);
    }
}

void Creature::walk(const Position& oldPos, const Position& newPos, bool preWalk)
{
    // get walk direction
    Otc::Direction direction = oldPos.getDirectionFromPosition(newPos);

    // already pre walking to the same direction
    if(m_preWalking && preWalk && direction == m_direction)
        return;

    // pre walking was already going on, just change to normal waking
    if(m_preWalking && !preWalk && direction == m_direction) {
        m_preWalking = false;
        m_walking = true;
        updateWalk();
        return;
    }

    setDirection(direction);

    // diagonal walking lasts 3 times more.
    int walkTimeFactor = 1;
    if(direction == Otc::NorthWest || direction == Otc::NorthEast || direction == Otc::SouthWest || direction == Otc::SouthEast)
        walkTimeFactor = 3;

    // calculate walk interval
    int groundSpeed = g_map.getTile(oldPos)->getGroundSpeed();
    float walkInterval = 1000.0 * (float)groundSpeed / m_speed;
    walkInterval = (walkInterval == 0) ? 1000 : walkInterval;
    walkInterval = std::ceil(walkInterval / g_game.getServerBeat()) * g_game.getServerBeat();

    m_walkTimePerPixel = walkInterval / 32.0;
    m_walkOffset = Point();
    m_walkStart = g_clock.ticks();
    m_walkEnd = m_walkStart + walkInterval * walkTimeFactor;
    m_walking = true;
    m_preWalking = preWalk;
    m_turnDirection = m_direction;
    updateWalk();
}

void Creature::turn(Otc::Direction direction)
{
    if(!m_walking)
        setDirection(direction);
    else
        m_turnDirection = direction;
}

void Creature::updateWalk()
{
    if(!m_walking)
        return;

    int elapsedTicks = g_clock.ticksElapsed(m_walkStart);
    int totalPixelsWalked = std::min((int)round(elapsedTicks / m_walkTimePerPixel), 32);

    if(!m_preWalking) {
        if(m_direction == Otc::North || m_direction == Otc::NorthEast || m_direction == Otc::NorthWest)
            m_walkOffset.y = 32 - totalPixelsWalked;
        else if(m_direction == Otc::South || m_direction == Otc::SouthEast || m_direction == Otc::SouthWest)
            m_walkOffset.y = totalPixelsWalked - 32;

        if(m_direction == Otc::East || m_direction == Otc::NorthEast || m_direction == Otc::SouthEast)
            m_walkOffset.x = totalPixelsWalked - 32;
        else if(m_direction == Otc::West || m_direction == Otc::NorthWest || m_direction == Otc::SouthWest)
            m_walkOffset.x = 32 - totalPixelsWalked;
    } else {
        if(m_direction == Otc::North || m_direction == Otc::NorthEast || m_direction == Otc::NorthWest)
            m_walkOffset.y = -totalPixelsWalked;
        else if(m_direction == Otc::South || m_direction == Otc::SouthEast || m_direction == Otc::SouthWest)
            m_walkOffset.y = totalPixelsWalked;

        if(m_direction == Otc::East || m_direction == Otc::NorthEast || m_direction == Otc::SouthEast)
            m_walkOffset.x = totalPixelsWalked;
        else if(m_direction == Otc::West || m_direction == Otc::NorthWest || m_direction == Otc::SouthWest)
            m_walkOffset.x = -totalPixelsWalked;
    }

    if(m_outfit.getCategory() == ThingsType::Creature) {
        if(totalPixelsWalked == 32 || m_type->dimensions[ThingType::AnimationPhases] <= 1)
            m_animation = 0;
        else if(m_type->dimensions[ThingType::AnimationPhases] > 1)
            m_animation = 1 + totalPixelsWalked * 4 / Map::NUM_TILE_PIXELS % (m_type->dimensions[ThingType::AnimationPhases] - 1);
    }

    if(g_clock.ticks() > m_walkEnd) {
        cancelWalk(m_turnDirection);
    } else
        g_dispatcher.scheduleEvent(std::bind(&Creature::updateWalk, asCreature()), m_walkTimePerPixel);
}

void Creature::cancelWalk(Otc::Direction direction, bool force)
{
    if(force) {
        m_walkOffset = Point();
        m_preWalking = false;
    } else if(!m_preWalking)
        m_walkOffset = Point();
    m_walking = false;

    if(direction != Otc::InvalidDirection)
        setDirection(direction);

    m_animation = 0;
}

void Creature::setName(const std::string& name)
{
    if(m_informationFont)
        m_nameSize = m_informationFont->calculateTextRectSize(name);
    m_name = name;
}

void Creature::setHealthPercent(uint8 healthPercent)
{
    m_informationColor = Fw::black;

    if(healthPercent > 92) {
        m_informationColor.setGreen(188);
    }
    else if(healthPercent > 60) {
        m_informationColor.setRed(80);
        m_informationColor.setGreen(161);
        m_informationColor.setBlue(80);
    }
    else if(healthPercent > 30) {
        m_informationColor.setRed(161);
        m_informationColor.setGreen(161);
    }
    else if(healthPercent > 8) {
        m_informationColor.setRed(160);
        m_informationColor.setGreen(39);
        m_informationColor.setBlue(39);
    }
    else if(healthPercent > 3) {
        m_informationColor.setRed(160);
    }
    else {
        m_informationColor.setRed(79);
    }

    m_healthPercent = healthPercent;
}

void Creature::setDirection(Otc::Direction direction)
{
    if(m_outfit.getCategory() == ThingsType::Creature) {
        if(direction == Otc::NorthEast || direction == Otc::SouthEast)
            m_xPattern = Otc::East;
        else if(direction == Otc::NorthWest || direction == Otc::SouthWest)
            m_xPattern = Otc::West;
        else
            m_xPattern = direction;
    } else {
        m_xPattern = 0;
    }

    m_direction = direction;
}

void Creature::setOutfit(const Outfit& outfit)
{
    if(m_outfit.getCategory() != ThingsType::Effect && outfit.getCategory() == ThingsType::Effect) {
        auto self = asCreature();
        g_dispatcher.scheduleEvent([self]() {
            self->updateInvisibleAnimation();
        }, INVISIBLE_TICKS);

        m_xPattern = 0;
        m_yPattern = 0;
    }
    if(m_outfit.getCategory() == ThingsType::Item) {
        m_xPattern = 0;
        m_yPattern = 0;
    }

    m_outfit = outfit;
    m_type = getType();

    if(m_outfit.getCategory() == ThingsType::Creature && m_type->dimensions[ThingType::Layers] == 1) {
        m_animation = 0;
        m_outfit.resetClothes();
    }
}

void Creature::setSkull(uint8 skull)
{
    m_skull = skull;
    g_lua.callGlobalField("Creature","onSkullChange", asCreature(), m_skull);
}

void Creature::setShield(uint8 shield)
{
    m_shield = shield;
    g_lua.callGlobalField("Creature","onShieldChange", asCreature(), m_shield);
}

void Creature::setEmblem(uint8 emblem)
{
    m_emblem = emblem;
    g_lua.callGlobalField("Creature","onEmblemChange", asCreature(), m_emblem);
}

void Creature::setSkullTexture(const std::string& filename)
{
    m_skullTexture = g_textures.getTexture(filename);
}

void Creature::setShieldTexture(const std::string& filename, bool blink)
{
    m_shieldTexture = g_textures.getTexture(filename);
    m_showShieldTexture = true;

    if(blink && !m_shieldBlink) {
        auto self = asCreature();
        g_dispatcher.scheduleEvent([self]() {
            self->updateShield();
        }, SHIELD_BLINK_TICKS);
    }

    m_shieldBlink = blink;
}

void Creature::setEmblemTexture(const std::string& filename)
{
    m_emblemTexture = g_textures.getTexture(filename);
}

void Creature::addVolatileSquare(uint8 color)
{
    m_showVolatileSquare = true;
    m_volatileSquareColor = Color::from8bit(color);

    // schedule removal
    auto self = asCreature();
    g_dispatcher.scheduleEvent([self]() {
        self->removeVolatileSquare();
    }, VOLATILE_SQUARE_DURATION);
}

void Creature::updateInvisibleAnimation()
{
    if(m_animation == 1)
        m_animation = 2;
    else if(m_animation == 2)
        m_animation = 3;
    else if(m_animation == 3)
        m_animation = 1;
    else
        m_animation = 1;

    if(g_game.isOnline() && m_outfit.getCategory() == ThingsType::Effect) {
        auto self = asCreature();
        g_dispatcher.scheduleEvent([self]() {
            self->updateInvisibleAnimation();
        }, INVISIBLE_TICKS);
    }
}

void Creature::updateShield()
{
    m_showShieldTexture = !m_showShieldTexture;

    if(m_shield != Otc::ShieldNone && m_shieldBlink) {
        auto self = asCreature();
        g_dispatcher.scheduleEvent([self]() {
            self->updateShield();
        }, SHIELD_BLINK_TICKS);
    }
    else if(!m_shieldBlink)
        m_showShieldTexture = true;
}

ThingType *Creature::getType()
{
    return g_thingsType.getThingType(m_outfit.getId(), m_outfit.getCategory());
}

