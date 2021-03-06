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

#include "application.h"
#include <framework/core/clock.h>
#include <framework/core/resourcemanager.h>
#include <framework/core/modulemanager.h>
#include <framework/core/eventdispatcher.h>
#include <framework/core/configmanager.h>
#include <framework/net/connection.h>
#include <framework/platform/platformwindow.h>
#include <framework/ui/uimanager.h>
#include <framework/ui/uiwidget.h>
#include <framework/graphics/graphics.h>
#include <framework/graphics/particlemanager.h>
#include <framework/graphics/painter.h>
#include <framework/luascript/luainterface.h>
#include <framework/platform/crashhandler.h>

Application *g_app = nullptr;

void exitSignalHandler(int sig)
{
    static bool signaled = false;
    switch(sig) {
        case SIGTERM:
        case SIGINT:
            if(!signaled) {
                signaled = true;
                g_dispatcher.addEvent(std::bind(&Application::close, g_app));
            }
            break;
    }
}

Application::Application(const std::string& appName)
{
    g_app = this;
    m_appName = appName;
    m_pollCycleDelay = POLL_CYCLE_DELAY;
}

Application::~Application()
{
    g_app = nullptr;
}

void Application::init(const std::vector<std::string>& args, int appFlags)
{
    m_appFlags = appFlags;
    logInfo("Starting application...");

    // capture exit signals
    signal(SIGTERM, exitSignalHandler);
    signal(SIGINT, exitSignalHandler);

#ifdef HANDLE_EXCEPTIONS
    installCrashHandler();
#endif

    // initialize lua
    g_lua.init();
    registerLuaFunctions();

    // initialize resources
    g_resources.init(args[0].c_str());

    if(m_appFlags & Fw::AppEnableConfigs) {
        // setup configs write directory
        if(!g_resources.setupWriteDir(m_appName))
            logError("Could not setup write directory");

        // load configs
        if(!g_configs.load("/config.otml"))
            logInfo("Using default configurations.");
    }

    // setup platform window
    if(m_appFlags & Fw::AppEnableGraphics) {
        g_ui.init();

        g_window.init();
        g_window.setOnResize(std::bind(&Application::resize, this, _1));
        g_window.setOnInputEvent(std::bind(&Application::inputEvent, this, _1));
        g_window.setOnClose(std::bind(&Application::close, this));

        // initialize graphics
        g_graphics.init();

        // fire first resize
        resize(g_window.getSize());

        // display window when the application starts running
        g_dispatcher.addEvent([]{ g_window.show(); });
    }

    if(m_appFlags & Fw::AppEnableModules)
        g_modules.discoverModulesPath();
}

void Application::terminate()
{
    // hide the window because there is no render anymore
    if(m_appFlags & Fw::AppEnableGraphics)
        g_window.hide();

    // run modules unload events
    if(m_appFlags & Fw::AppEnableModules)
        g_modules.unloadModules();

    // release remaining lua object references
    g_lua.collectGarbage();

    // poll remaining events
    poll();

    // terminate network
    Connection::terminate();

    // terminate graphics
    if(m_appFlags & Fw::AppEnableGraphics) {
        g_ui.terminate();
        g_window.terminate();
    }

    // flush remaining dispatcher events
    g_dispatcher.flush();

    // save configurations
    if(m_appFlags & Fw::AppEnableConfigs)
        g_configs.save();

    // release resources
    g_resources.terminate();

    // terminate script environment
    g_lua.terminate();

    logInfo("Application ended successfully.");
}

void Application::run()
{
    ticks_t lastPollTicks = g_clock.updateTicks();
    m_stopping = false;
    m_running = true;

    // run the first poll
    poll();

    while(!m_stopping) {
        g_clock.updateTicks();

        // poll events every POLL_CYCLE_DELAY
        // this delay exists to avoid massive polling thus increasing framerate
        if(g_clock.ticksElapsed(lastPollTicks) >= m_pollCycleDelay) {
            poll();
            lastPollTicks = g_clock.ticks();
        }

        if(m_appFlags & Fw::AppEnableGraphics && g_window.isVisible()) {
            g_graphics.beginRender();
            render();
            g_graphics.endRender();

            // update screen pixels
            g_window.swapBuffers();
        } else {
            // sleeps until next poll to avoid massive cpu usage
            g_clock.sleep(POLL_CYCLE_DELAY+1);
        }
    }

    m_stopping = false;
    m_running = false;
}

void Application::exit()
{
    logInfo("Exiting application..");
    m_stopping = true;
}

void Application::poll()
{
    // poll input events
    if(m_appFlags & Fw::AppEnableGraphics) {
        g_window.poll();
        g_particleManager.update();
    }

    Connection::poll();
    g_dispatcher.poll();
}

void Application::close()
{
    g_lua.getGlobalField("g_app", "onClose");
    if(!g_lua.isNil())
        g_lua.protectedCall();
    else
        exit();
}

void Application::render()
{
    // everything is rendered by UI components
    g_ui.render();

    g_particleManager.render();
}

void Application::resize(const Size& size)
{
    g_graphics.resize(size);
    g_ui.resize(size);
}

void Application::inputEvent(const InputEvent& event)
{
    g_ui.inputEvent(event);
}
