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

#include "animatedtext.h"
#include "map.h"
#include <framework/core/clock.h>
#include <framework/core/eventdispatcher.h>
#include <framework/graphics/graphics.h>

AnimatedText::AnimatedText()
{
    m_font = g_fonts.getFont("verdana-11px-rounded");
}

void AnimatedText::start()
{
    m_startTime = g_clock.time();

    auto self = asAnimatedText();

    // schedule removal
    g_dispatcher.scheduleEvent([self]() {
        g_map.removeThing(self);
    }, DURATION);
}

void AnimatedText::draw(const Point& p, const Rect& visibleRect)
{
    if(m_font) {
        Rect rect = Rect(p + Point(20 - m_textSize.width() / 2, -20.0 * g_clock.timeElapsed(m_startTime) / (DURATION / 1000)), m_textSize);
        if(visibleRect.contains(rect))
            m_font->renderText(m_text, rect, Fw::AlignLeft, m_color);
    }
}

void AnimatedText::setColor(int color)
{
    m_color = Color::from8bit(color);
}

void AnimatedText::setText(const std::string& text)
{
    if(m_font)
        m_textSize = m_font->calculateTextRectSize(text);
    m_text = text;
}
