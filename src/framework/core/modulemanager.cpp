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

#include "modulemanager.h"
#include "resourcemanager.h"

#include <framework/otml/otml.h>
#include <framework/application.h>

ModuleManager g_modules;

void ModuleManager::discoverModules()
{
    auto moduleDirs = g_resources.listDirectoryFiles("/");
    for(const std::string& moduleDir : moduleDirs) {
        auto moduleFiles = g_resources.listDirectoryFiles("/" + moduleDir);
        for(const std::string& moduleFile : moduleFiles) {
            if(boost::ends_with(moduleFile, ".otmod")) {
                ModulePtr module = discoverModule("/" + moduleDir + "/" + moduleFile);
                if(module && module->isAutoLoad())
                    m_autoLoadModules.insert(make_pair(module->getAutoLoadAntecedence(), module));
            }
        }
    }
}

void ModuleManager::autoLoadModules(int maxAntecedence)
{
    for(auto& pair : m_autoLoadModules) {
        int priority = pair.first;
        if(priority > maxAntecedence)
            break;
        ModulePtr module = pair.second;
        if(!module->isLoaded() && !module->load())
            logFatal("A required module has failed to load, cannot continue to run.");
    }
}

void ModuleManager::discoverModulesPath()
{
    // search for modules directory
    std::string possibleDirs[] = { "modules",
                                    g_resources.getBaseDir() + "modules",
                                    g_resources.getBaseDir() + "../modules",
                                    g_resources.getBaseDir() + "../share/" + g_app->getAppName() + "/modules",
                                    "" };
    bool found = false;
    for(const std::string& dir : possibleDirs) {
        // try to add module directory
        if(g_resources.addToSearchPath(dir, false)) {
            logInfo("Using modules directory '", dir.c_str(), "'");
            found = true;
            break;
        }
    }

    if(!found)
        logFatal("Could not find modules directory");
}

ModulePtr ModuleManager::discoverModule(const std::string& moduleFile)
{
    ModulePtr module;
    try {
        OTMLDocumentPtr doc = OTMLDocument::parse(moduleFile);
        OTMLNodePtr moduleNode = doc->at("Module");

        std::string name = moduleNode->valueAt("name");
        if(getModule(name))
            Fw::throwException("module '", name, "' already exists, cannot have duplicate module names");

        module = ModulePtr(new Module(name));
        module->discover(moduleNode);
        m_modules.push_back(module);
    } catch(Exception& e) {
        logError("Unable to discover module from file '", moduleFile, "': ", e.what());
    }
    return module;
}

void ModuleManager::ensureModuleLoaded(const std::string& moduleName)
{
    ModulePtr module = g_modules.getModule(moduleName);
    if(!module || !module->load())
        logFatal("Unable to load '", moduleName, "' module");
}

void ModuleManager::unloadModules()
{
    for(const ModulePtr& module : m_modules)
        module->unload();
}

ModulePtr ModuleManager::getModule(const std::string& moduleName)
{
    for(const ModulePtr& module : m_modules)
        if(module->getName() == moduleName)
            return module;
    return nullptr;
}
