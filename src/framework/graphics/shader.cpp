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

#include "shader.h"
#include <framework/core/resourcemanager.h>

Shader::Shader(Shader::ShaderType shaderType)
{
    m_shaderType = shaderType;
    switch(shaderType) {
        case Vertex:
            m_shaderId = glCreateShader(GL_VERTEX_SHADER);
            break;
        case Fragment:
            m_shaderId = glCreateShader(GL_FRAGMENT_SHADER);
            break;
    }

    if(!m_shaderId)
        logFatal("Unable to create GL shader");
}

Shader::~Shader()
{
    glDeleteShader(m_shaderId);
}

bool Shader::compileSourceCode(const std::string& sourceCode)
{
#ifndef OPENGL_ES2
    static const char *qualifierDefines =
        "#define lowp\n"
        "#define mediump\n"
        "#define highp\n";
#else
    static const char *qualifierDefines =
        "#ifndef GL_FRAGMENT_PRECISION_HIGH\n"
        "#define highp mediump\n"
        "#endif\n"
        "precision highp float;\n";
#endif

    std::string code = qualifierDefines;
    code.append(sourceCode);
    const char *c_source = code.c_str();
    glShaderSource(m_shaderId, 1, &c_source, NULL);
    glCompileShader(m_shaderId);

    int res;
    glGetShaderiv(m_shaderId, GL_COMPILE_STATUS, &res);
    return (res == GL_TRUE);
}

bool Shader::compileSourceFile(const std::string& sourceFile)
{
    std::string sourceCode = g_resources.loadFile(sourceFile);
    return compileSourceCode(sourceCode);
}

std::string Shader::log()
{
    std::string infoLog;
    GLint infoLogLength;
    glGetShaderiv(m_shaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
    if(infoLogLength > 1) {
        std::vector<char> buf(infoLogLength);
        glGetShaderInfoLog(m_shaderId, infoLogLength-1, NULL, &buf[0]);
        infoLog = &buf[0];
    }
    return infoLog;
}
