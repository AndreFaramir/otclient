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

#include "game.h"
#include "localplayer.h"
#include "map.h"
#include "tile.h"
#include <framework/core/eventdispatcher.h>
#include <framework/ui/uimanager.h>
#include <otclient/luascript/luavaluecasts.h>
#include <otclient/core/statictext.h>
#include <otclient/net/protocolgame.h>

Game g_game;

void Game::loginWorld(const std::string& account, const std::string& password, const std::string& worldHost, int worldPort, const std::string& characterName)
{
    m_online = false;
    m_dead = false;
    m_walkFeedback = true;
    m_selectedThing = nullptr;
    m_walkPing = 0;
    m_protocolGame = ProtocolGamePtr(new ProtocolGame);
    m_protocolGame->login(account, password, worldHost, (uint16)worldPort, characterName);
}

void Game::cancelLogin()
{
    processLogout();
}

void Game::logout(bool force)
{
    if(!m_protocolGame || !m_online)
        return;

    m_protocolGame->sendLogout();

    if(force)
        processLogout();
}

void Game::processLoginError(const std::string& error)
{
    g_lua.callGlobalField("Game", "onLoginError", error);
}

void Game::processConnectionError(const boost::system::error_code& error)
{
    // connection errors only have meaning if we still have a protocol
    if(m_protocolGame) {
        if(error != asio::error::eof)
            g_lua.callGlobalField("Game", "onConnectionError", error.message());

        processLogout();
    }
}

void Game::processLogin(const LocalPlayerPtr& localPlayer, int serverBeat)
{
    m_localPlayer = localPlayer;
    m_online = true;
    m_serverBeat = serverBeat;

    // NOTE: the entire map description is not known yet
    g_lua.callGlobalField("Game", "onLogin", localPlayer);
}

void Game::processLogout()
{
    if(m_online) {
        g_lua.callGlobalField("Game", "onLogout", m_localPlayer);

        m_localPlayer.reset();
        m_online = false;
    }

    if(m_protocolGame) {
        m_protocolGame->disconnect();
        m_protocolGame.reset();
    }
}

void Game::processDeath()
{
    m_dead = true;
    g_lua.callGlobalField("Game","onDeath");
}

void Game::processCreatureSpeak(const std::string& name, int level, const std::string& type, const std::string& message, int channelId, const Position& creaturePos)
{
    if(creaturePos.isValid() && (type == "say" || type == "whisper" || type == "yell" || type == "monsterSay" || type == "monsterYell")) {
        StaticTextPtr staticText = StaticTextPtr(new StaticText);
        staticText->addMessage(name, type, message);
        g_map.addThing(staticText, creaturePos);
    }

    g_lua.callGlobalField("Game", "onCreatureSpeak", name, level, type, message, channelId, creaturePos);
}

void Game::processTextMessage(const std::string& type, const std::string& message)
{
    g_lua.callGlobalField("Game","onTextMessage", type, message);
}

void Game::processContainerAddItem(int containerId, const ItemPtr& item)
{
    if(item)
        item->setPos(Position(65535, containerId + 0x40, 0));

    g_lua.callGlobalField("Game", "onContainerAddItem", containerId, item);
}

void Game::processInventoryChange(int slot, const ItemPtr& item)
{
    if(item)
        item->setPos(Position(65535, slot, 0));

    g_lua.callGlobalField("Game","onInventoryChange", slot, item);
}

void Game::processCreatureMove(const CreaturePtr& creature, const Position& oldPos, const Position& newPos)
{
    /*
    // walk
    if(oldPos.isInRange(newPos, 1, 1, 0)) {
        Otc::Direction direction = oldPos.getDirectionFromPosition(newPos);
        creature->setDirection(direction);
    // teleport
    } else {
        // stop animation on teleport
        if(creature->isWalking())
            creature->cancelWalk();
    }
    */
    if(!m_walkFeedback && creature == m_localPlayer) {
        updatePing();
        m_walkFeedback = true;
    }
    creature->walk(newPos);
}

void Game::processAttackCancel()
{
    if(m_localPlayer->isAttacking())
        m_localPlayer->setAttackingCreature(nullptr);
}

void Game::processWalkCancel(Otc::Direction direction)
{
    if(!m_walkFeedback) {
        updatePing();
        m_walkFeedback = true;
    }
    m_localPlayer->cancelWalk(direction, true);
}

void Game::walk(Otc::Direction direction)
{
    if(m_localPlayer->isFollowing()) {
        cancelFollow();
        return;
    }

    if(!isOnline() || isDead() || !checkBotProtection() || !m_localPlayer->canWalk(direction))
        return;

    m_localPlayer->clientWalk(direction);

    // ping calculation restarts when the local players try to walk one tile
    m_walkPingTimer.restart();

    switch(direction) {
    case Otc::North:
        m_protocolGame->sendWalkNorth();
        break;
    case Otc::East:
        m_protocolGame->sendWalkEast();
        break;
    case Otc::South:
        m_protocolGame->sendWalkSouth();
        break;
    case Otc::West:
        m_protocolGame->sendWalkWest();
        break;
    case Otc::NorthEast:
        m_protocolGame->sendWalkNorthEast();
        break;
    case Otc::SouthEast:
        m_protocolGame->sendWalkSouthEast();
        break;
    case Otc::SouthWest:
        m_protocolGame->sendWalkSouthWest();
        break;
    case Otc::NorthWest:
        m_protocolGame->sendWalkNorthWest();
        break;
    }

    m_walkFeedback = false;
}

void Game::turn(Otc::Direction direction)
{
    if(!m_online)
        return;

    switch(direction) {
    case Otc::North:
        m_protocolGame->sendTurnNorth();
        break;
    case Otc::East:
        m_protocolGame->sendTurnEast();
        break;
    case Otc::South:
        m_protocolGame->sendTurnSouth();
        break;
    case Otc::West:
        m_protocolGame->sendTurnWest();
        break;
    }
}

void Game::look(const ThingPtr& thing)
{
    if(!m_online || !thing || !checkBotProtection())
        return;

    int stackpos = getThingStackpos(thing);
    if(stackpos != -1)
        m_protocolGame->sendLookAt(thing->getPos(), thing->getId(), stackpos);
}

void Game::open(const ThingPtr& thing, int containerId)
{
    if(!m_online || !thing || !checkBotProtection())
        return;

    int stackpos = getThingStackpos(thing);
    if(stackpos != -1)
        m_protocolGame->sendUseItem(thing->getPos(), thing->getId(), stackpos, containerId);
}

void Game::use(const ThingPtr& thing)
{
    if(!m_online || !thing || !checkBotProtection())
        return;

    int stackpos = getThingStackpos(thing);
    if(stackpos != -1)
        m_protocolGame->sendUseItem(thing->getPos(), thing->getId(), stackpos, 0);
}

void Game::useWith(const ThingPtr& fromThing, const ThingPtr& toThing)
{
    if(!m_online || !fromThing || !toThing || !checkBotProtection())
        return;

    int fromStackpos = getThingStackpos(fromThing), toStackpos = getThingStackpos(toThing);
    if(fromStackpos != -1 && toStackpos != -1)
        m_protocolGame->sendUseItemEx(fromThing->getPos(), fromThing->getId(), fromStackpos, toThing->getPos(), toThing->getId(), toStackpos);
}

void Game::attack(const CreaturePtr& creature)
{
    if(!m_online || !creature || !checkBotProtection())
        return;

    if(m_localPlayer->isFollowing())
        cancelFollow();

    m_localPlayer->setAttackingCreature(creature);
    m_protocolGame->sendAttack(creature->getId());
}

void Game::cancelAttack()
{
    m_localPlayer->setAttackingCreature(nullptr);
    m_protocolGame->sendAttack(0);
}

void Game::follow(const CreaturePtr& creature)
{
    if(!m_online || !creature || !checkBotProtection())
        return;

    if(m_localPlayer->isAttacking())
        cancelAttack();

    m_localPlayer->setFollowingCreature(creature);
    m_protocolGame->sendFollow(creature->getId());
}

void Game::cancelFollow()
{
    m_localPlayer->setFollowingCreature(nullptr);
    m_protocolGame->sendFollow(0);
}

void Game::rotate(const ThingPtr& thing)
{
    if(!m_online || !thing || !checkBotProtection())
        return;

    int stackpos = getThingStackpos(thing);
    if(stackpos != -1)
        m_protocolGame->sendRotateItem(thing->getPos(), thing->getId(), stackpos);
}

int Game::getThingStackpos(const ThingPtr& thing)
{
    // thing is at map
    if(thing->getPos().x != 65535) {
        TilePtr tile = g_map.getTile(thing->getPos());
        return tile->getThingStackpos(thing);
    }

    // thing is at container or inventory
    return 0;
}

void Game::talk(const std::string& message)
{
    talkChannel(1, 0, message);
}

void Game::talkChannel(int channelType, int channelId, const std::string& message)
{
    if(!m_online || !checkBotProtection())
        return;

    m_protocolGame->sendTalk(channelType, channelId, "", message);
}

void Game::talkPrivate(int channelType, const std::string& receiver, const std::string& message)
{
    if(!m_online || !checkBotProtection())
        return;

    m_protocolGame->sendTalk(channelType, 0, receiver, message);
}

void Game::partyInvite(int creatureId)
{
    if(!m_online || !checkBotProtection())
        return;

    m_protocolGame->sendInviteToParty(creatureId);
}

void Game::partyJoin(int creatureId)
{
    if(!m_online || !checkBotProtection())
        return;

    m_protocolGame->sendJoinParty(creatureId);
}

void Game::partyRevokeInvitation(int creatureId)
{
    if(!m_online || !checkBotProtection())
        return;

    m_protocolGame->sendRevokeInvitation(creatureId);
}


void Game::partyPassLeadership(int creatureId)
{
    if(!m_online || !checkBotProtection())
        return;

    m_protocolGame->sendPassLeadership(creatureId);
}

void Game::partyLeave()
{
    if(!m_online || !checkBotProtection())
        return;

    m_protocolGame->sendLeaveParty();
}

void Game::partyShareExperience(bool active)
{
    if(!m_online || !checkBotProtection())
        return;

    m_protocolGame->sendShareExperience(active, 0);
}

void Game::openOutfitWindow()
{
    if(!m_online || !checkBotProtection())
        return;

    m_protocolGame->sendGetOutfit();
}

void Game::setOutfit(const Outfit& outfit)
{
    if(!m_online || !checkBotProtection())
        return;

    m_protocolGame->sendSetOutfit(outfit);
}

void Game::addVip(const std::string& name)
{
    if(!m_online || name.empty() || !checkBotProtection())
        return;

    m_protocolGame->sendAddVip(name);
}

void Game::removeVip(int playerId)
{
    if(!m_online || !checkBotProtection())
        return;

    m_protocolGame->sendRemoveVip(playerId);
}

bool Game::checkBotProtection()
{
#ifndef NO_BOT_PROTECTION
    if(g_lua.isInCppCallback() && !g_ui.isOnInputEvent()) {
        logError("caught a lua call to a bot protected game function, the call was canceled");
        return false;
    }
#endif
    return true;
}

void Game::updatePing()
{
    m_walkPing = m_walkPingTimer.ticksElapsed();
    g_lua.callGlobalField("Game", "onWalkPingUpdate", m_walkPing);
}
