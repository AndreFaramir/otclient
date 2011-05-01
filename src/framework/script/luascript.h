/* The MIT License
 *
 * Copyright (c) 2010 OTClient, https://github.com/edubart/otclient
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


#ifndef LUASCRIPT_H
#define LUASCRIPT_H

#include <prerequisites.h>
#include <script/scriptable.h>
#include <lua.hpp>

#define reportFuncError(a) reportError(a, __FUNCTION__)
#define reportFuncErrorWithTraceback(a) reportErrorWithTraceback(a, __FUNCTION__)

class LuaScript
{
public:
    LuaScript();
    virtual ~LuaScript();

    void loadAllModules();
    bool loadFile(const std::string& fileName);
    bool loadBuffer(const std::string& text, const std::string& what = "luaBuffer");
    int loadBufferAsFunction(const std::string& text, const std::string& what = "luaBuffer");
    void reportError(const std::string& errorDesc, const char *funcName = NULL);
    void reportErrorWithTraceback(const std::string& errorDesc, const char *funcName = NULL);

    int getStackSize();
    void moveTop(int index);

    void pushNil();
    void pushBoolean(bool b);
    void pushInteger(int i);
    void pushString(const std::string& str);
    void pushUserdata(void* ptr);

    void pop(int n = 1);
    bool popBoolean();
    int32_t popInteger();
    std::string popString();

    void pushFunction(int functionRef);
    int popFunction();
    void releaseFunction(int functionRef);
    void callFunction(int numArgs = 0);

    SimpleCallback createSimpleFuncCallback(int funcRef);
    boost::function<void(ScriptablePtr)> createScriptableSelfFuncCallback(int funcRef);

    void setLocal(const ScriptablePtr& scriptable, const char *varName, int envIndex = -1);

    void pushClassInstance(const ScriptablePtr& object);
    ScriptablePtr popClassInstance();

    typedef int (*LuaCFunction)();

    void setupPackageLoader();
    void registerClass(const std::string& klass, const std::string& baseClass = "");
    void registerMemberFunction(const std::string& klass, const std::string& functionName, LuaCFunction function);
    void registerGlobalFunction(const std::string& functionName, LuaCFunction function);
    void registerModule(const std::string& module);

    static int luaFunctionCallback(lua_State* L);
    static int luaPackageLoader(lua_State* L);
    static int luaCollectClassInstance(lua_State* L);
    static int luaCompareClassInstances(lua_State* L);
    static int luaErrorHandler(lua_State *L);

private:
    std::vector<LuaCFunction> m_functions;
    lua_State *L;
};

extern LuaScript g_lua;

#endif // LUASCRIPT_H