/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Authors: Pablo d'Angelo <pablo.dangelo@web.de>
 *	     Leon Moctezuma <densedev_at_gmail_dot_com>
 *
 *  $Id: OpenGLRenderer.cpp 150 2008-10-15 14:18:53Z leonox $
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 2.1 of
 * the License
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA, or see the FSF site: http://www.fsf.org.
 */

#include <config.h>

#include <vector>
#include <cmath>

#include "OpenGLRenderer.h"
#include "Scene.h"
#include "utils.h"
#include "glutfont/freeglut_font_copy.h"
#include <assert.h>
#include "Error.h"

using namespace FPV;

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

///////////////////////////////////////////////
//
// utility functions
//

GLenum getChannels(Image *img)
{
    if (img->getColorChannels() == 3)
        return GL_RGB;
    if (img->getColorChannels() == 4)
        return GL_RGBA;
}

void queryTileSize(size_t maxTextureMem, Size2D imgsize, int nImg,
                   int &tileSize, Size2D &tileDim, int colorChannels = 3)
{
    if (maxTextureMem == 0)
    {
        maxTextureMem = 256 * 1024 * 1024;
    }

    int maxFaceMem = maxTextureMem / nImg;

    GLint maxTileSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTileSize);

    //Prevent a division by zero
    if (!imgsize.w)
        throw ZeroDivision();

    // texture identifier.
    int maxWidthMem = (int)sqrt((double)maxFaceMem / colorChannels * imgsize.w / imgsize.h);
    int maxWidth = std::min(imgsize.w, maxWidthMem);
    int maxHeight = maxWidth * imgsize.h / imgsize.w;

    int xTiles, yTiles;
    for (tileSize = 64; tileSize < maxTileSize; tileSize = tileSize << 1)
    {
        // calculate number of tiles
        xTiles = (int)ceil(maxWidth / (float)tileSize);
        yTiles = (int)ceil(maxHeight / (float)tileSize);
        if (std::min(xTiles, yTiles) <= 3)
        {
            break;
        }
    }
    // make sure this fits into the texture memory
    while (xTiles > 0 && xTiles * yTiles * tileSize * tileSize * colorChannels > maxFaceMem)
    {
        xTiles--;
        // calculate new number of y tiles.
        maxWidth = xTiles * tileSize;
        maxHeight = maxWidth * imgsize.h / imgsize.w;
        yTiles = (int)ceil(maxHeight / (float)tileSize);
    }
    tileDim.w = xTiles;
    tileDim.h = yTiles;
}

class TiledTexture
{

public:
    TiledTexture()
        : init(false)
    {
    }

    ~TiledTexture()
    {
        if (init)
        {
            glDeleteTextures(xTiles * yTiles, &(texIds[0]));
        }
    }

    void create(Image *img, Size2D tiles, int texWidth, RenderQuality quality)
    {
        tileWidth = texWidth;
        // calculate x tile size
        xTiles = (int)ceil(img->size().w / (float)tileWidth);
        yTiles = (int)ceil(img->size().h / (float)tileWidth);

        texIds.resize(xTiles * yTiles);
        glGenTextures(xTiles * yTiles, &(texIds[0])); /* create textures */

        effTexSize.w = img->size().w / xTiles;
        effTexSize.h = img->size().h / yTiles;

        int texId = 0;
        Image tile(Size2D(tileWidth, tileWidth), img->getType());
        for (int y = 0; y < yTiles; y++) // y
        {
            for (int x = 0; x < xTiles; x++) // x
            {
                copyImgToTexImg(&tile, img, Point2D(0, 0), Point2D(x * effTexSize.w, y * effTexSize.h),
                                Size2D(effTexSize.w, effTexSize.h), true);

                /*{
                std::string fn;
                FPV_S2S(fn, "/tmp/tile_" << "_y" << y << "_x" << x <<  ".pnm");
                tile.writePPM(fn);
                }*/

                glBindTexture(GL_TEXTURE_2D, texIds[texId]);
                texId++;
                if (quality == RQ_HIGH)
                {
                    gluBuild2DMipmaps(GL_TEXTURE_2D, img->getColorChannels(),
                                      tileWidth, tileWidth,
                                      getChannels(img), GL_UNSIGNED_BYTE, tile.getData());
                }
                else
                {
                    glTexImage2D(GL_TEXTURE_2D, 0, img->getColorChannels(),
                                 tileWidth, tileWidth, 0,
                                 getChannels(img), GL_UNSIGNED_BYTE, tile.getData());
                }
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                switch (quality)
                {
                case RQ_LOW:
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    break;
                case RQ_MEDIUM:
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    break;
                case RQ_HIGH:
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    break;
                }
            }
        }
        init = true;
    }

    GLuint getTile(int x, int y)
    {
        assert(x < xTiles);
        assert(y < yTiles);
        return texIds[x + y * xTiles];
    }

    bool init;
    std::vector<GLuint> texIds;
    Size2D size;
    // size of a single tile
    int tileWidth;
    int xTiles;
    int yTiles;
    Size2D effTexSize;
};

class OGL_RenderData : public RenderData
{

protected:
    Matrix4 m_rotation_mx;
    float z;

public:
    virtual ~OGL_RenderData()
    {
    }
    virtual void render() = 0;
    void rotation(quaternion q)
    {
        q.toMatrix(m_rotation_mx);
    }
    void depth(float _z) { z = _z; }
};

/** render data for a text string */
class OGL_TextRenderData : public OGL_RenderData
{
public:
    /// Create textures
    OGL_TextRenderData(TextElement *txt)
        : m_text(txt)
    {
    }

    void render()
    {
        // render text in single line
        // TODO: support multiple lines of text
        glDisable(GL_TEXTURE_2D);
        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos3f(20.0f, 20.0f, 0.9f);
        glColor3f(1.0f, 1.0f, 1.0f);
        const unsigned char *str = (const unsigned char *)m_text->m_text.c_str();
        //DEBUG_DEBUG("string output: " << str << ", " << m_text->m_text.c_str());
        FPVglutBitmapString(FPVGLUT_BITMAP_8_BY_13, str);
        //glutBitmapString(GLUT_BITMAP_HELVETICA_18, (const unsigned char *)"Hello World");
        glEnable(GL_TEXTURE_2D);
    }

private:
    TextElement *m_text;
};

// flat renderer with a tiled texture
/** render data for flat panoramas */
class OGL_FlatRenderData : public OGL_RenderData
{
public:
    /// Create textures
    OGL_FlatRenderData(FlatPano *pano, float depth, size_t maxTexMem, RenderQuality quality)
    {
        // load opengl textures
        if (pano->m_image != 0)
        {
            m_srcSize = pano->m_image->size();
            glGenTextures(1, &Texture);
            update(pano, depth, maxTexMem, quality);
        }
        else
            throw ImageError(pano->m_image);
    }

    void update(FlatPano *pano, float depth, size_t maxTexMem, RenderQuality quality)
    {
        int tileWidth;
        Size2D tileDim;

        //The image must have an area bigger than 0
        if (pano->m_image->size().w <= 0 || pano->m_image->size().h <= 0)
            throw ImageError(pano->m_image);

        // select a suitable texture size.
        queryTileSize(maxTexMem, pano->m_image->size(), 1,
                      tileWidth, tileDim, pano->m_image->getColorChannels());

        if (tileWidth * tileDim.w < pano->m_image->size().w ||
            tileWidth * tileDim.h < pano->m_image->size().h)
        {
            // create downscaled image for texturing
            Image scaled(Size2D(tileWidth * tileDim.w, tileWidth * tileDim.h), pano->m_image->getType());
            gluScaleImage(getChannels(pano->m_image), pano->m_image->size().w,
                          pano->m_image->size().h, GL_UNSIGNED_BYTE, pano->m_image->getData(),
                          scaled.size().w, scaled.size().h, GL_UNSIGNED_BYTE, scaled.getData());
            if (quality == RQ_HIGH)
                quality = RQ_MEDIUM;
            m_texture.create(&scaled, tileDim, tileWidth, quality);
        }
        else
        {
            // non scaled texture
            m_texture.create(pano->m_image, tileDim, tileWidth, quality);
        }
        //z =2*atan(pano->hfov()*3.1416/360);
        m_z = depth;
        m_a = 2 * m_z * tan(pano->hfov() * 3.1416 / 360);
        if (pano->m_image->size().w > pano->m_image->size().h)
        {
            m_size_x = m_a * pano->m_image->size().w / pano->m_image->size().h;
            m_size_y = m_a;
        }
        else
        {
            m_size_y = m_a * pano->m_image->size().h / pano->m_image->size().w;
            m_size_x = m_a;
        }
        m_init = true;
    }

    /// Delete textures
    ~OGL_FlatRenderData()
    {
    }

    /** Draw the flat */
    void render()
    {

        float x_step = m_size_x / (float)m_texture.xTiles;
        float y_step = m_size_y / (float)m_texture.yTiles;

        glPushMatrix();
        //we get the rotation matrix and multiply it.
        glMultMatrixf(m_rotation_mx.get());
        for (int y = 0; y < m_texture.yTiles; y++)
        {
            for (int x = 0; x < m_texture.xTiles; x++)
            {
                glBindTexture(GL_TEXTURE_2D, m_texture.getTile(x, y));
                glBegin(GL_QUADS);
                glTexCoord2f(0.0f, 1.0f);
                glVertex3f(-m_size_x / 2 + x_step * x, m_size_y / 2 - y_step * (y + 1), -m_z);
                glTexCoord2f(1.0f, 1.0f);
                glVertex3f(-m_size_x / 2 + x_step * (x + 1), m_size_y / 2 - y_step * (y + 1), -m_z);
                glTexCoord2f(1.0f, 0.0f);
                glVertex3f(-m_size_x / 2 + x_step * (x + 1), m_size_y / 2 - y_step * y, -m_z);
                glTexCoord2f(0.0f, 0.0f);
                glVertex3f(-m_size_x / 2 + x_step * x, m_size_y / 2 - y_step * y, -m_z);
                glEnd();
            }
        }
        glPopMatrix();
        //glPushMatrix();
        //glTranslatef(0,0,-4);
        //glPopMatrix();
    }

protected:
    bool m_init;
    bool m_alpha;
    float m_size_x;
    float m_size_y;
    int m_tex_size;
    TiledTexture m_texture;
    GLuint Texture;
    float m_a, m_z;
    Size2D m_srcSize;
};

// spherical renderer with a tiled texture
/** render data for cylindrical panoramas, some parts of the algorithm taken from panoglview */
class OGL_SphericalRenderData : public OGL_RenderData
{
public:
    /// Create textures
    OGL_SphericalRenderData(SphericalPano *pano, float depth, size_t maxTexMem, RenderQuality quality)
    {
        // load opengl textures
        if (pano->m_image != 0)
        {
            m_srcSize = pano->m_image->size();
            update(pano, depth, maxTexMem, quality);
        }
        else
            throw ImageError(pano->m_image);
    }

    void update(SphericalPano *pano, float depth, size_t maxTexMem, RenderQuality quality)
    {
        int tileWidth;
        Size2D tileDim;
        m_radius = depth;

        //The image must have an area bigger than 0
        if (pano->m_image->size().w <= 0 || pano->m_image->size().h <= 0)
            throw ImageError(pano->m_image);

        // select a suitable texture size.
        queryTileSize(maxTexMem, pano->m_image->size(), 1,
                      tileWidth, tileDim);
        if (tileWidth * tileDim.w < pano->m_image->size().w ||
            tileWidth * tileDim.h < pano->m_image->size().h)
        {
            // create downscaled image for texturing
            Image scaled(Size2D(tileWidth * tileDim.w, tileWidth * tileDim.h));
            gluScaleImage(GL_RGB, pano->m_image->size().w, pano->m_image->size().h,
                          GL_UNSIGNED_BYTE, pano->m_image->getData(),
                          scaled.size().w, scaled.size().h, GL_UNSIGNED_BYTE, scaled.getData());
            if (quality == RQ_HIGH)
                quality = RQ_MEDIUM;
            m_texture.create(&scaled, tileDim, tileWidth, quality);
        }
        else
        {
            // non scaled texture
            if (quality == RQ_HIGH)
                quality = RQ_MEDIUM;
            m_texture.create(pano->m_image, tileDim, tileWidth, quality);
        }
        m_init = true;
    }

    /// Delete textures
    ~OGL_SphericalRenderData()
    {
    }

    /** Draw the sphere */
    void render()
    {
        //glPolygonMode( GL_FRONT_AND_BACK, mode );

        // radius of sphere
        float r = m_radius;
        // circumfence of cylinder
        //float c = (float)(2*M_PI*r);

        int nSegmentsPerTileX = (int)ceil(120.0f / m_texture.xTiles);
        int nSegmentsPerTileY = (int)ceil(60.0f / m_texture.yTiles);
        //int nSegmentsX = nSegmentsPerTileX* m_texture.xTiles;
        //int nSegmentsY = nSegmentsPerTileY* m_texture.yTiles;

        // Angular interval for each patch
        double phiinterval = 2.0 * M_PI / m_texture.xTiles;
        double thetainterval = M_PI / m_texture.yTiles;

        // Angular increment for eatch patch step
        double phistep = phiinterval / nSegmentsPerTileX;
        double thetastep = thetainterval / nSegmentsPerTileY;

        glPushMatrix();
        //we get the rotation matrix and multiply it.
        glMultMatrixf(m_rotation_mx.get());

        for (int y = 0; y < m_texture.yTiles; y++)
        {
            for (int x = 0; x < m_texture.xTiles; x++)
            {
                glBindTexture(GL_TEXTURE_2D, m_texture.getTile(x, y)); /* select right texture */

                // loop over theta (y)
                for (int k = 0; k < nSegmentsPerTileY; k++)
                {
                    double theta = y * thetainterval - M_PI / 2.0 + k * thetastep;
                    double nexttheta = theta + thetastep;

                    glBegin(GL_QUAD_STRIP);

                    // loop over phi (x)
                    for (int l = 0; l <= nSegmentsPerTileX; l++)
                    {
                        double phi = (x + 1) * phiinterval + M_PI / 2.0 - l * phistep;

                        GLfloat u0 = (m_texture.effTexSize.w - ((l) / (double)nSegmentsPerTileX * m_texture.effTexSize.w)) / m_texture.tileWidth;

                        GLfloat v0 = (m_texture.effTexSize.h - ((nSegmentsPerTileY - k - 1) / (double)nSegmentsPerTileY * m_texture.effTexSize.h)) / m_texture.tileWidth;
                        GLfloat v1 = (m_texture.effTexSize.h - ((nSegmentsPerTileY - k) / (double)nSegmentsPerTileY * m_texture.effTexSize.h)) / m_texture.tileWidth;

                        glTexCoord2f(u0, v0);
                        //                        glTexCoord2f(m_maxtexturex - (l / (double) m_stepsPerTexture.x * m_maxtexturex),
                        //                                     (k+1) / (double) m_stepsPerTexture.y * m_maxtexturey);

                        glVertex3d(r * cos(nexttheta) * cos(phi), r * -sin(nexttheta), r * cos(nexttheta) * sin(phi));

                        glTexCoord2f(u0, v1);
                        //glTexCoord2f(m_maxtexturex - (l / (double) m_stepsPerTexture.x * m_maxtexturex),
                        //             k / (double) m_stepsPerTexture.y * m_maxtexturey);

                        glVertex3d(r * cos(theta) * cos(phi), r * -sin(theta), r * cos(theta) * sin(phi));
                    }
                    glEnd();
                }
            }
        }
        glPopMatrix();
    }

protected:
    bool m_init;
    TiledTexture m_texture;
    //GLuint m_textures[6];
    int m_enabledCubefaces;
    int m_tex_size;
    float m_radius;
    Size2D m_srcSize;
};

// cylindrical renderer with a tiled texture
/** render data for cylindrical panoramas */
class OGL_CylindricalRenderData : public OGL_RenderData
{
public:
    /// Create textures
    OGL_CylindricalRenderData(CylindricalPano *pano, float depth, size_t maxTexMem, RenderQuality quality)
    {
        // load opengl textures
        if (pano->m_image != 0)
        {
            m_srcSize = pano->m_image->size();
            std::cerr << "image size " << m_srcSize.w << std::endl;
            update(pano, depth, maxTexMem, quality);
        }
        else
            throw ImageError(pano->m_image);
    }

    void update(CylindricalPano *pano, float depth, size_t maxTexMem, RenderQuality quality)
    {
        int tileWidth;
        Size2D tileDim;
        m_radius = depth;

        //The image must have an area bigger than 0
        if (pano->m_image->size().w <= 0 || pano->m_image->size().h <= 0)
            throw ImageError(pano->m_image);

        // select a suitable texture size.
        queryTileSize(maxTexMem, pano->m_image->size(), 1,
                      tileWidth, tileDim);
        if (tileWidth * tileDim.w < pano->m_image->size().w ||
            tileWidth * tileDim.h < pano->m_image->size().h)
        {
            // create downscaled image for texturing
            Image scaled(Size2D(tileWidth * tileDim.w, tileWidth * tileDim.h));
            gluScaleImage(GL_RGB, pano->m_image->size().w, pano->m_image->size().h,
                          GL_UNSIGNED_BYTE, pano->m_image->getData(),
                          scaled.size().w, scaled.size().h, GL_UNSIGNED_BYTE, scaled.getData());
            m_texture.create(&scaled, tileDim, tileWidth, quality);
        }
        else
        {
            // non scaled texture
            m_texture.create(pano->m_image, tileDim, tileWidth, quality);
        }
        m_init = true;
    }

    /// Delete textures
    ~OGL_CylindricalRenderData()
    {
    }

    /** Draw the cylinder */
    void render()
    {

        // radius of cylinder
        float r = m_radius;
        // circumfence of cylinder
        float c = (float)(2 * M_PI * r);
        // half height of cylinder
        float height2 = m_srcSize.h * c / m_srcSize.w / 2.0;

        //float m_segments = 128;

        int nSegmentsPerTile = 100 / m_texture.xTiles;
        int nSegments = nSegmentsPerTile * m_texture.xTiles;

        // Angular interval for each patch
        //double phiinterval   = 2.0 * M_PI/m_texture.xTiles;

        // Angular increment for eatch patch step
        //double phistep   = phiinterval  / nSegmentsPerTile;

        glPushMatrix();
        //we get the rotation matrix and multiply it.
        glMultMatrixf(m_rotation_mx.get());

        // y1 coordinate for each patch
        GLfloat v1 = m_texture.effTexSize.h / (GLfloat)m_texture.tileWidth;
        for (int x = 0; x < m_texture.xTiles; x++)
        {
            for (int y = 0; y < m_texture.yTiles; y++)
            {
                float hBeg = height2 - 2.0f * height2 * ((y + 1.0f) / m_texture.yTiles);
                float hEnd = height2 - 2.0f * height2 * (y / (float)m_texture.yTiles);

                glBindTexture(GL_TEXTURE_2D, m_texture.getTile(x, y)); /* select right texture */
                glBegin(GL_QUADS);
                //double phi = (x+1) * phiinterval - M_PI_2;
                for (int i = 0; i < nSegmentsPerTile; i++)
                {
                    GLfloat u0 = (m_texture.effTexSize.w - (i / (double)nSegmentsPerTile * m_texture.effTexSize.w)) / m_texture.tileWidth;
                    GLfloat u1 = (m_texture.effTexSize.w - ((i + 1) / (double)nSegmentsPerTile * m_texture.effTexSize.w)) / m_texture.tileWidth;
                    GLfloat v0 = 0;

                    int ii = nSegmentsPerTile - i - 1;
                    float x1 = (float)(-r * sin(2.0 * M_PI * (x * nSegmentsPerTile + ii) / nSegments));
                    float x0 = (float)(-r * sin(2.0 * M_PI * (x * nSegmentsPerTile + ii + 1) / nSegments));
                    float z1 = (float)(r * cos(2.0 * M_PI * (x * nSegmentsPerTile + ii) / nSegments));
                    float z0 = (float)(r * cos(2.0 * M_PI * (x * nSegmentsPerTile + ii + 1) / nSegments));

                    glTexCoord2f(u0, v0);
                    glVertex3f(x0, hEnd, z0);
                    glTexCoord2f(u0, v1);
                    glVertex3f(x0, hBeg, z0);
                    glTexCoord2f(u1, v1);
                    glVertex3f(x1, hBeg, z1);
                    glTexCoord2f(u1, v0);
                    glVertex3f(x1, hEnd, z1);
                }
                glEnd();
            }
        }
        glPopMatrix();
    }

protected:
    bool m_init;
    TiledTexture m_texture;
    //GLuint m_textures[6];
    int m_enabledCubefaces;
    int m_tex_size;
    float m_radius;
    Size2D m_srcSize;
};

/** render data for cubic panoramas */
class OGL_CubicRenderData : public OGL_RenderData
{
public:
    /// Create textures
    OGL_CubicRenderData(CubicPano *pano, float depth, size_t maxTextureMem, RenderQuality quality)
    {
        if (maxTextureMem == 0)
        {
            maxTextureMem = 256 * 1024 * 1024;
        }

        if (pano->m_images[0] != NULL)
        {
            if (pano->m_images[0]->size().w != pano->m_images[0]->size().h)
                pano->setCubeFaces(ChopToCubeFace(*pano->m_images[0]));
        }

        int maxFaceMem = maxTextureMem / 6;

        GLint maxTileSize;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTileSize);

        // try to create not more than 6x6 tiles per face
        m_srcSize = pano->m_size.w;

        int maxFaceWidthMem = (int)sqrt((double)maxFaceMem / 3);
        int maxFaceWidth = std::min(m_srcSize, maxFaceWidthMem);
        for (m_tileSize = 64; m_tileSize < maxTileSize; m_tileSize = m_tileSize << 1)
        {
            m_nTileDim = (int)ceil((float)maxFaceWidth / m_tileSize);
            if (m_nTileDim <= 6)
            {
                // make sure this fits into the texture memory
                while (m_nTileDim * m_tileSize > maxFaceWidthMem)
                {
                    m_nTileDim--;
                }
                break;
            }
        }

        m_faceSize = m_nTileDim * m_tileSize;

        fprintf(stderr, "original cube face width: %d, using %d tiles with a width of %d each.\n", pano->m_size.w,
                m_nTileDim * m_nTileDim * 6, m_tileSize);
        DEBUG_DEBUG("tiled face size: " << m_faceSize << "  original size: " << pano->m_size.w);
        DEBUG_NOTICE("Pano cube width: " << m_srcSize << ". RAM for a face width: " << maxFaceWidthMem);
        DEBUG_NOTICE("Used cube width: " << m_faceSize << ". " << m_nTileDim * m_nTileDim * 6 << " tiles with a width of " << m_tileSize << " pixels");

        if (m_srcSize > m_faceSize)
        {
            m_realFaceWidth = m_faceSize;
            fprintf(stderr, "Warning: reducing cube size from %d to %d, due to memory limitations.\n", m_srcSize, m_faceSize);
        }
        else
        {
            m_realFaceWidth = m_srcSize;
        }

        DEBUG_DEBUG("Tiling cube faces: " << m_nTileDim << "x" << m_nTileDim << " tiles, with " << m_tileSize << "x" << m_tileSize << " pixels");
        DEBUG_DEBUG("tiled face size: " << m_faceSize << "  original size: " << pano->m_size.w);

        /* create textures */
        m_textures.resize(m_nTileDim * m_nTileDim * 6);
        glGenTextures(m_nTileDim * m_nTileDim * 6, &(m_textures[0]));
        // select generate a suitable texture size.

        m_enabledCubefaces = 0;
        update(pano, depth, quality);
    }

    void update(CubicPano *pano, float depth, RenderQuality quality)
    {
        Image *scaled = 0;
        m_cubeSize = depth;
        bool downscale = (m_faceSize < m_srcSize);
        if (downscale)
        {
            scaled = new Image(Size2D(m_faceSize, m_faceSize));
        }
        Image *tile = new Image(Size2D(m_tileSize, m_tileSize));

        //        int texId=0;
        for (int i = 0; i < 6; i++)
        {
            if (pano->m_images[i] == 0 || m_enabledCubefaces & (1 << i))
            {
                continue;
            }

            Image *img = pano->m_images[i];
            if (downscale)
            {
                gluScaleImage(GL_RGB, pano->m_size.w, pano->m_size.h,
                              GL_UNSIGNED_BYTE, pano->m_images[i]->getData(),
                              m_faceSize, m_faceSize, GL_UNSIGNED_BYTE, scaled->getData());
                img = scaled;

                /*
                {
                    std::string fn;
                    FPV_S2S(fn, "/tmp/scaled_face_" << i <<  ".pnm");
                    img->writePPM(fn);
                }
                */
            }

            // enable cubeface
            m_enabledCubefaces |= (1 << i);
            int texId = i * m_nTileDim * m_nTileDim;

            for (int k = 0; k < m_nTileDim; k++) // y
            {
                for (int j = 0; j < m_nTileDim; j++) // x
                {
                    //DEBUG_TRACE("timing tile creation");
                    //int edgeTile=false;
                    unsigned char *tilePixels;
                    if ((j < (m_nTileDim - 1)) && (k < (m_nTileDim - 1)))
                    {
                        if (quality == RQ_HIGH)
                        {
                            // inner tile, copy texture image.
                            // gluBuild2DMipmaps doesn't respect GL_UNPACK_ROW_LENGTH on my machine.
                            copyImgToTexImg(tile, img, Point2D(0, 0), Point2D(j * m_tileSize, k * m_tileSize),
                                            Size2D(m_tileSize, m_tileSize));
                            tilePixels = tile->getData();
                            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                        }
                        else
                        {
                            tilePixels = img->getData() + j * m_tileSize * 3 + k * m_tileSize * img->getRowStride();
                            glPixelStorei(GL_UNPACK_ROW_LENGTH, img->size().w);
                        }
                    }
                    else
                    {
                        // right or bottom tile, copy as much as possible, with padding
                        copyImgToTexImg(tile, img, Point2D(0, 0), Point2D(j * m_tileSize, k * m_tileSize),
                                        Size2D(m_tileSize, m_tileSize), true);
                        tilePixels = tile->getData();
                        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                    }
                    /*
{
    std::string fn;
                    FPV_S2S(fn, "/tmp/face_" << i << "_y" << k << "_x" << j <<  ".pnm");
    tile->writePPM(fn);
}
                    */
                    glBindTexture(GL_TEXTURE_2D, m_textures[texId]);
                    texId++;
                    if (quality == RQ_HIGH)
                    {
                        gluBuild2DMipmaps(GL_TEXTURE_2D, 3,
                                          m_tileSize, m_tileSize,
                                          GL_RGB, GL_UNSIGNED_BYTE, tilePixels);
                    }
                    else
                    {
                        glTexImage2D(GL_TEXTURE_2D, 0, 3,
                                     m_tileSize, m_tileSize, 0,
                                     GL_RGB, GL_UNSIGNED_BYTE, tilePixels);
                    }
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    /* use linear filtering */
                    switch (quality)
                    {
                    case RQ_LOW:
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                        break;
                    case RQ_MEDIUM:
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        break;
                    case RQ_HIGH:
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                        break;
                    }
                }
            }
        }
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        if (scaled)
        {
            delete scaled;
        }
        m_init = true;
    }

    /// Delete textures
    ~OGL_CubicRenderData()
    {
        if (m_init)
        {
            for (int i = 0; i < 6; i++)
            {
                glDeleteTextures(1, &m_textures[i]);
            }
        }
    }
    /** Draw the cube */
    void render()
    {
        // permutation index for x,y,z for the different cube sides:
        static const int perm[6][6] =
            {
                // idx      signs of the new axis
                {0, 1, 2, 1, 1, 1},   // front
                {2, 1, 0, -1, 1, 1},  // right
                {0, 1, 2, -1, 1, -1}, // back
                {2, 1, 0, 1, 1, -1},  // left
                {0, 2, 1, 1, -1, 1},  // top
                {0, 2, 1, 1, 1, -1}   // bottom
            };

        glPushMatrix();
        //we get the rotation matrix and multiply it.
        glMultMatrixf(m_rotation_mx.get());
        for (int i = 0; i < 6; i++)
        {
            if (m_enabledCubefaces & (1 << i))
            {
                int texId = i * m_nTileDim * m_nTileDim;
                // loop over y
                for (int k = 0; k < m_nTileDim; k++)
                {
                    // loop over x
                    for (int j = 0; j < m_nTileDim; j++)
                    {
                        glBindTexture(GL_TEXTURE_2D, m_textures[texId]); /* select right texture */
                        texId++;
                        glBegin(GL_QUADS);

                        GLfloat z = -m_cubeSize / 2.f;

                        // position on cube:
                        GLfloat left = j * m_tileSize / (float)m_realFaceWidth;
                        left = (left - 0.5f) * m_cubeSize;
                        GLfloat right = (j == m_nTileDim - 1) ? 1.0 : (j + 1) * m_tileSize / (float)m_realFaceWidth;
                        right = (right - 0.5f) * m_cubeSize;
                        GLfloat top = (m_realFaceWidth - k * m_tileSize) / (float)m_realFaceWidth;
                        top = (top - 0.5f) * m_cubeSize;
                        GLfloat bottom = (k == m_nTileDim - 1) ? 0.f : (m_realFaceWidth - (k + 1) * m_tileSize) / (float)m_realFaceWidth;
                        bottom = (bottom - 0.5f) * m_cubeSize;

                        // texture coordinates
                        GLfloat u_max = (j == m_nTileDim - 1) ? ((m_realFaceWidth - (m_nTileDim - 1) * m_tileSize)) / (float)m_tileSize : 1.f;
                        GLfloat v_max = (k == m_nTileDim - 1) ? ((m_realFaceWidth - (m_nTileDim - 1) * m_tileSize)) / (float)m_tileSize : 1.f;

                        GLfloat vec[3];
                        GLfloat rot[3];
                        // lower left
                        glTexCoord2f(0, v_max);
                        vec[0] = left;
                        vec[1] = bottom;
                        vec[2] = z;
                        rot[0] = perm[i][3] * vec[perm[i][0]];
                        rot[1] = perm[i][3 + 1] * vec[perm[i][1]];
                        rot[2] = perm[i][3 + 2] * vec[perm[i][2]];

                        glVertex3f(perm[i][3] * vec[perm[i][0]],
                                   perm[i][3 + 1] * vec[perm[i][1]],
                                   perm[i][3 + 2] * vec[perm[i][2]]);

                        // lower right
                        glTexCoord2f(u_max, v_max);
                        vec[0] = right;
                        glVertex3f(perm[i][3] * vec[perm[i][0]],
                                   perm[i][3 + 1] * vec[perm[i][1]],
                                   perm[i][3 + 2] * vec[perm[i][2]]);

                        // upper right
                        glTexCoord2f(u_max, 0);
                        vec[1] = top;
                        glVertex3f(perm[i][3] * vec[perm[i][0]],
                                   perm[i][3 + 1] * vec[perm[i][1]],
                                   perm[i][3 + 2] * vec[perm[i][2]]);

                        // upper left
                        glTexCoord2f(0, 0);
                        vec[0] = left;
                        vec[1] = top;
                        vec[2] = z;
                        glVertex3f(perm[i][3] * vec[perm[i][0]],
                                   perm[i][3 + 1] * vec[perm[i][1]],
                                   perm[i][3 + 2] * vec[perm[i][2]]);
                        glEnd();
                    }
                }
            }
        }
        glPopMatrix();
    }

protected:
    bool m_init;
    float m_cubeSize;
    std::vector<GLuint> m_textures;
    int m_enabledCubefaces;
    // number of tiles in x and y direction
    int m_nTileDim;
    // size of a single tile
    int m_tileSize;
    // size of QTVR/panorama cube face
    int m_srcSize;
    // width of acutally used image data in the texture. Is smaller
    // than m_faceSize, if m_faceSize is > m_srcSize
    int m_realFaceWidth;
    // size of a cube face for rendering, after possible downscaling
    int m_faceSize;
};

OpenGLRenderer::OpenGLRenderer(Platform *platform, size_t texMem, RenderQuality q)
    : m_quality(q),
      m_platform(platform),
      m_maxTexMem(texMem)
{
    m_maxDepth = 100.0f;
    m_minDepth = 0.1f;
    m_currentDepth = 20;
};

void OpenGLRenderer::max_depth(float maxz)
{
    if (m_maxDepth < 1000)
        m_maxDepth = maxz;
    else
        m_maxDepth = 1000;
}

void OpenGLRenderer::min_depth(float minz)
{
    if (m_minDepth > 0.1f)
        m_minDepth = minz;
    else
        m_minDepth = 0.1f;
}

void OpenGLRenderer::init()
{
    m_platform->glBegin();
    glEnable(GL_TEXTURE_2D); /* Enable Texture Mapping */
    glShadeModel(GL_FLAT);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    /* we use resizeGLScene once to set up our initial perspective */
    //resizeGLScene(GLWin.width, GLWin.height);

    // we cannot guarantee that all the image rows are aligned to 4 bytes
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    switch (m_quality)
    {
    case RQ_LOW:
        glDisable(GL_DITHER);
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
        break;
    case RQ_MEDIUM:
    case RQ_HIGH:
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
        break;
    }
    glFlush();
    m_platform->glEnd();
}

void OpenGLRenderer::initElement(SceneElement &pano)
{
    try
    {
        switch (pano.getType())
        {
        case SceneElement::PANO_CUBIC:
            if (pano.m_renderData)
            {
                static_cast<OGL_CubicRenderData *>(pano.m_renderData)->update((CubicPano *)&pano, m_currentDepth, m_quality);
            }
            else
            {
                pano.m_renderData = new OGL_CubicRenderData((CubicPano *)&pano, m_currentDepth, m_maxTexMem, m_quality);
            }
            break;
        case SceneElement::PANO_CYLINDRICAL:
            if (pano.m_renderData)
            {
                static_cast<OGL_CylindricalRenderData *>(pano.m_renderData)->update((CylindricalPano *)&pano, m_currentDepth, m_maxTexMem, m_quality);
            }
            else
            {
                //Render data should not be created if the image doesn't exist or there is no data
                if (((CylindricalPano *)&pano)->m_image != NULL)
                {
                    if (((CylindricalPano *)&pano)->m_image->size().w <= 0 || ((CylindricalPano *)&pano)->m_image->size().h <= 0)
                        return;
                }
                pano.m_renderData = new OGL_CylindricalRenderData((CylindricalPano *)&pano, m_currentDepth, m_maxTexMem, m_quality);
            }
            break;
        case SceneElement::PANO_SPHERICAL:
            if (pano.m_renderData)
            {
                static_cast<OGL_SphericalRenderData *>(pano.m_renderData)->update((SphericalPano *)&pano, m_currentDepth, m_maxTexMem, m_quality);
            }
            else
            {
                //Render data should not be created if the image doesn't exist or there is no data
                if (((SphericalPano *)&pano)->m_image != NULL)
                {
                    if (((SphericalPano *)&pano)->m_image->size().w <= 0 || ((SphericalPano *)&pano)->m_image->size().h <= 0)
                        return;
                }
                pano.m_renderData = new OGL_SphericalRenderData((SphericalPano *)&pano, m_currentDepth, m_maxTexMem, m_quality);
            }
            break;
        case SceneElement::PANO_FLAT:
            if (pano.m_renderData)
            {
                static_cast<OGL_FlatRenderData *>(pano.m_renderData)->update((FlatPano *)&pano, m_currentDepth, m_maxTexMem, m_quality);
            }
            else
            {
                //Render data should not be created if the image doesn't exist or there is no data
                if (((FlatPano *)&pano)->m_image != NULL)
                {
                    if (((FlatPano *)&pano)->m_image->size().w <= 0 || ((FlatPano *)&pano)->m_image->size().h <= 0)
                        return;
                }
                pano.m_renderData = new OGL_FlatRenderData((FlatPano *)&pano, m_currentDepth, m_maxTexMem, m_quality);
            }
            break;
        case SceneElement::GROUP:
            break;
        case SceneElement::PLACE_HOLDER:
            break;
        case SceneElement::TEXT:
            if (!pano.m_renderData)
            {
                pano.m_renderData = new OGL_TextRenderData((TextElement *)&pano);
            }
            break;
        default:
            fprintf(stderr, "OpenGLRender: panoelement %d is not yet implemented\n",
                    pano.getType());
            break;
        }
    }
    catch (ImageError &e)
    {
        std::cerr << e.what() << std::endl;
    }
    catch (Error &e)
    {
        std::cerr << e.what() << std::endl;
    }
    catch (...)
    {
        throw;
    }
};

void OpenGLRenderer::changeCurrentDepth(SceneElement &pano)
{
    switch (pano.getType())
    {
    case SceneElement::PANO_CUBIC:
        m_currentDepth /= 2;
        break;
    case SceneElement::PANO_FLAT:
        m_currentDepth *= (180 - ((FlatPano *)&pano)->hfov()) / 180;
        break;
    case SceneElement::PANO_SPHERICAL:
        m_currentDepth -= 0.1;
        break;
    case SceneElement::PANO_CYLINDRICAL:
        m_currentDepth -= 0.1;
        break;
    }
}

std::list<Subject *> *OpenGLRenderer::getPointedSubjects(Scene &scene, Point2D point)
{
    GLint viewport[4];
    unsigned int p_color;
    Subject *p_subject;
    std::list<Subject *> *aux = new std::list<Subject *>();
    std::list<Subject *> *final = new std::list<Subject *>();
    unsigned int color = 0;
    unsigned int mask = 1;

    m_platform->glBegin();

    glDisable(GL_DITHER);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_OR);

    GLfloat ratio;
    if (m_size.h == 0)
    {
        ratio = 1;
    }
    else
    {
        ratio = (GLfloat)m_size.w / (GLfloat)m_size.h;
    }

    glGetIntegerv(GL_VIEWPORT, viewport);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //gluPickMatrix(point.x, point.y, 50, 50, viewport);
    gluPerspective(scene.getCamera()->get_fov(), ratio, m_minDepth, m_maxDepth);

    // clear buffer if no render element was available
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRotatef(scene.getCamera()->get_pitch(), 1.0f, 0.0f, 0.0f); /* rotate on the X axis */
    glRotatef(scene.getCamera()->get_yaw(), 0.0f, 1.0f, 0.0f);   /* rotate on the Y axis */

    {
        // render all PanoElements
        SceneElementNode::Iterator iter = scene.getSceneElementRoot()->getIterator();
        while (iter.element() != NULL)
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            p_color = 1;
            for (int rendered_elements = 0;
                 rendered_elements < 32;
                 rendered_elements++)
            {

                if (iter.element())
                {
                    glColor4ubv((GLubyte *)&p_color);
                    if (iter.element()->m_renderData)
                    {
                        //update the element rotation.
                        static_cast<OGL_RenderData *>(iter.element()->m_renderData)->rotation(getRotation(iter));
                        static_cast<OGL_RenderData *>(iter.element()->m_renderData)->render();
                    }
                }
                aux->push_back(iter.element());
                iter++;

                p_color <<= 1;
                if (!iter.element())
                    break;
            } //for

            glReadPixels(point.x, viewport[3] - point.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, (void *)&p_color);

            mask = 1 << aux->size() - 1;
            while (aux->size() > 0)
            {
                p_subject = aux->back();
                aux->pop_back();
                if (mask & p_color)
                    final->push_back(p_subject);
                mask >>= 1;
            }
        } //while
    }
    glDisable(GL_COLOR_LOGIC_OP);

    m_platform->glEnd();
    //m_platform->glSwapBuffers();
    return final;
}

void OpenGLRenderer::render(Scene &scene)
{
    m_platform->glBegin();

    GLfloat ratio;
    if (m_size.h == 0)
    {
        ratio = 1;
    }
    else
    {
        ratio = (GLfloat)m_size.w / (GLfloat)m_size.h;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(scene.getCamera()->get_fov(), ratio, m_minDepth, m_maxDepth);

    // clear buffer if no render element was available
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();

    glRotatef(scene.getCamera()->get_pitch(), 1.0f, 0.0f, 0.0f); /* rotate on the X axis */
    glRotatef(scene.getCamera()->get_yaw(), 0.0f, 1.0f, 0.0f);   /* rotate on the Y axis */

    {
        glEnable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glColor3f(1.0, 1.0, 1.0);
        // render all PanoElements
        SceneElementNode::Iterator iter = scene.getSceneElementRoot()->getIterator();
        m_currentDepth = m_maxDepth;
        while (iter.element() != NULL)
        {
            if (iter.element())
            {
                changeCurrentDepth(*iter.element());

                //PLACEHOLDER
                if (iter.element()->getType() == SceneElement::PLACE_HOLDER)
                {
                    Image *img = NULL;
                    if (img = ((PlaceHolder *)iter.element())->getImage())
                    {
                        Size2D size = img->size();
                        if (size.w * size.h > 0)
                        {
                            //CUBE
                            if (((size.w * 6) == size.h) || ((size.h * 6) == size.w))
                            {
                                iter.swap(new CubicPano());
                                ((CubicPano *)iter.element())->setCubeFaces(ChopToCubeFace(*img));
                            }
                            //SPHERICAL
                            else if (size.w == 2 * size.h)
                            {
                                iter.swap(new SphericalPano());
                                ((SphericalPano *)iter.element())->setImage(img);
                            }
                            //CYLINDRICAL
                            else if (size.w > 2 * size.h)
                            {
                                iter.swap(new CylindricalPano());
                                ((CylindricalPano *)iter.element())->setImage(img);
                            }
                        }
                    }
                } //palce-holder

                if (!iter.element()->m_renderData)
                {
                    initElement(*iter.element());
                }

                // render only if the render data is really available
                if (iter.element()->m_renderData && iter.element()->isVisible())
                {
                    //update the element rotation.
                    static_cast<OGL_RenderData *>(iter.element()->m_renderData)->rotation(getRotation(iter));
                    static_cast<OGL_RenderData *>(iter.element()->m_renderData)->render();
                }
                else
                {
                }
            }
            else
            {
                // clear buffer if no render element was available*/
                glClear(GL_COLOR_BUFFER_BIT);
            }
            iter++;
        } //while
        glDisable(GL_BLEND);
        glDisable(GL_TEXTURE_2D);
    }

    // switch to projection mode
    glMatrixMode(GL_PROJECTION);
    // reset matrix
    glLoadIdentity();
    // set a 2D orthographic projection
    gluOrtho2D(0, (GLfloat)m_size.w, 0, (GLfloat)m_size.h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    {
        SceneElement *elem = scene.getUIElement();
        if (elem)
        {
            if (!elem->m_renderData)
            {
                initElement(*elem);
            }
            // render only if the render data is really available
            if (elem->m_renderData)
            {
                static_cast<OGL_RenderData *>(elem->m_renderData)->render();
            }
        }
    }

    m_platform->glEnd();
    m_platform->glSwapBuffers();
};

void OpenGLRenderer::resize(Size2D size)
{
    if (size == m_size)
    {
        return;
    }

    if (size.h == 0) /* Prevent A Divide By Zero If The Window Is Too Small */
        size.h = 1;
    m_size = size;
    /* Reset The Current Viewport And Perspective Transformation */
    glViewport(0, 0, m_size.w, m_size.h);
    /*
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (GLfloat)m_size.w / (GLfloat)m_size.h, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    */
}
