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

#include "paintershaderprogram.h"
#include "painter.h"
#include "texture.h"
#include "texturemanager.h"
#include <framework/core/clock.h>

bool PainterShaderProgram::link()
{
    bindAttributeLocation(VERTEX_COORDS_ATTR, "vertexCoord");
    bindAttributeLocation(TEXTURE_COORDS_ATTR, "textureCoord");
    if(ShaderProgram::link()) {
        bindUniformLocation(PROJECTION_MATRIX_UNIFORM, "projectionMatrix");
        bindUniformLocation(TEXTURE_TRANSFORM_MATRIX_UNIFORM, "textureTransformMatrix");
        bindUniformLocation(COLOR_UNIFORM, "color");
        bindUniformLocation(OPACITY_UNIFORM, "opacity");
        bindUniformLocation(TEXTURE_UNIFORM, "texture");
        bindUniformLocation(TIME_UNIFORM, "time");
        return true;
    }
    m_startTimer.restart();
    return false;
}

void PainterShaderProgram::setProjectionMatrix(const Matrix3& projectionMatrix)
{
    bind();
    setUniformValue(PROJECTION_MATRIX_UNIFORM, projectionMatrix);
}

void PainterShaderProgram::setColor(const Color& color)
{
    bind();
    setUniformValue(COLOR_UNIFORM, color);
}

void PainterShaderProgram::setOpacity(float opacity)
{
    bind();
    setUniformValue(OPACITY_UNIFORM, opacity);
}

void PainterShaderProgram::setUniformTexture(int location, const TexturePtr& texture, int index)
{
    if(index > 0)
        glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_2D, texture ? texture->getId() : 0);
    if(index > 0)
        glActiveTexture(GL_TEXTURE0);
    setUniformValue(location, index);
}

void PainterShaderProgram::setTexture(const TexturePtr& texture)
{
    if(!texture)
        return;

    float w = texture->getWidth();
    float h = texture->getHeight();

    Matrix2 textureTransformMatrix = { 1.0f/w,  0.0f,
                                       0.0f,    1.0f/h };
    textureTransformMatrix.transpose();

    bind();
    setUniformTexture(TEXTURE_UNIFORM, texture, 0);
    setUniformValue(TEXTURE_TRANSFORM_MATRIX_UNIFORM, textureTransformMatrix);
}

void PainterShaderProgram::draw(const CoordsBuffer& coordsBuffer, DrawMode drawMode)
{
    assert(bind());

    setUniformValue(TIME_UNIFORM, (float)m_startTimer.timeElapsed());

    int numVertices = coordsBuffer.getVertexCount();
    if(numVertices == 0)
        return;

    bool mustDisableVertexArray = false;
    if(coordsBuffer.getVertexCount() > 0) {
        enableAttributeArray(PainterShaderProgram::VERTEX_COORDS_ATTR);
        setAttributeArray(PainterShaderProgram::VERTEX_COORDS_ATTR, coordsBuffer.getVertices(), 2);
        mustDisableVertexArray = true;
    }

    bool mustDisableTexCoordsArray = false;
    if(coordsBuffer.getTextureCoordsCount() > 0) {
        enableAttributeArray(PainterShaderProgram::TEXTURE_COORDS_ATTR);
        setAttributeArray(PainterShaderProgram::TEXTURE_COORDS_ATTR, coordsBuffer.getTextureCoords(), 2);
        mustDisableTexCoordsArray = true;
    }

    glDrawArrays(drawMode, 0, numVertices);

    if(mustDisableVertexArray)
        disableAttributeArray(PainterShaderProgram::VERTEX_COORDS_ATTR);

    if(mustDisableTexCoordsArray)
        disableAttributeArray(PainterShaderProgram::TEXTURE_COORDS_ATTR);

    //release();
}

