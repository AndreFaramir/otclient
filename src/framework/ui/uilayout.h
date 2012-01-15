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

#ifndef UILAYOUT_H
#define UILAYOUT_H

#include "declarations.h"
#include <framework/luascript/luaobject.h>
#include <framework/otml/otml.h>

class UILayout : public LuaObject
{
public:
    UILayout(UIWidgetPtr parentWidget) : m_parentWidget(parentWidget) { }

    void update();
    void updateLater();

    virtual void applyStyle(const OTMLNodePtr& styleNode) { }
    virtual void addWidget(const UIWidgetPtr& widget) = 0;
    virtual void removeWidget(const UIWidgetPtr& widget) = 0;
    void disableUpdates() { m_updateDisabled = true; }
    void enableUpdates() { m_updateDisabled = false; }

    void setParent(UIWidgetPtr parentWidget) { m_parentWidget = parentWidget; }
    UIWidgetPtr getParentWidget() { return m_parentWidget.lock(); }

    bool isUpdateDisabled() { return m_updateDisabled; }
    bool isUpdating() { return m_updating; }

    UILayoutPtr asUILayout() { return std::static_pointer_cast<UILayout>(shared_from_this()); }
    virtual UIAnchorLayoutPtr asUIAnchorLayout() { return nullptr; }
    virtual UIBoxLayoutPtr asUIBoxLayout() { return nullptr; }
    virtual UIHorizontalLayoutPtr asUIHorizontalLayout() { return nullptr; }
    virtual UIVerticalLayoutPtr asUIVerticalLayout() { return nullptr; }
    virtual UIGridLayoutPtr asUIGridLayout() { return nullptr; }

protected:
    virtual void internalUpdate() = 0;

    Boolean<false> m_updateDisabled;
    Boolean<false> m_updating;
    Boolean<false> m_updateScheduled;
    UIWidgetWeakPtr m_parentWidget;
};

#endif
