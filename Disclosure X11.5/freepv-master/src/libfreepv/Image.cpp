/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: Image.cpp 150 2008-10-15 14:18:53Z leonox $
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

#include <assert.h>
#include <string.h>

#include "Image.h"
#include "JpegReader.h"
#include "pngReader.h"

using namespace FPV;

bool Image::setSize(Size2D size, colorChannels channels)
{
    if (m_data)
    {
        free(m_data);
    }
    m_data = (unsigned char *)malloc(size.w * size.h * channels);
    m_size = size;
    m_rowStride = channels * m_size.w;
    m_color_channels = channels;
    return true;
}

void Image::writePPM(std::string file)
{
    FILE *f;
    f = fopen(file.c_str(), "wb");
    assert(f);
    fprintf(f, "P6\n%d %d\n%d\n", m_size.w, m_size.h, 255);
    for (int i = 0; i < m_size.h * m_size.w; i++)
    {
        fwrite(m_data + (i * m_color_channels), sizeof(unsigned char), 3, f);
    }
    fclose(f);
}

Image *Image::getSubImage(Point2D pos, Size2D size)
{
    assert(pos.x >= 0);
    assert(pos.y >= 0);
    assert(pos.x + size.w <= m_size.w);
    assert(pos.y + size.h <= m_size.h);

    Image *ret = new Image(size);
    if (ret == 0)
        return 0;

    int srcStride = m_color_channels * sizeof(T) * m_size.w;
    int destStride = m_color_channels * sizeof(T) * size.w;
    // copy image
    unsigned char *srcPtr = this->getData() + m_color_channels * sizeof(T) * pos.x + m_color_channels * sizeof(T) * m_size.w * pos.y;
    unsigned char *destPtr = ret->getData();

    for (int y = size.h; y; y--)
    {
        memcpy(destPtr, srcPtr, destStride);
        destPtr += destStride;
        srcPtr += srcStride;
    }
    return ret;
}

void FPV::copyImgToTexImg(Image *dest, Image *src, Point2D destPos, Point2D srcPos, Size2D srcSize, bool pad)
{
    if (srcSize.w == -1)
        srcSize = src->size() - Size2D(srcPos.x, srcPos.y);

    if (srcSize.w + srcPos.x > src->size().w)
    {
        srcSize.w = src->size().w - srcPos.x;
    }
    if (srcSize.h + srcPos.y > src->size().h)
    {
        srcSize.h = src->size().h - srcPos.y;
    }

    assert((srcPos.x + srcSize.w <= src->size().w) && (srcPos.x + srcSize.w <= src->size().w));
    assert((destPos.x + srcSize.w <= dest->size().w) && (destPos.y + srcSize.h <= dest->size().h));

    //get the color channels neeeded.
    int color_channels = src->getColorChannels();

    // pad if requested.
    bool padX = (destPos.x + srcSize.w != dest->size().w) && pad;
    bool padY = (destPos.y + srcSize.h != dest->size().h) && pad;

    int srcStride = src->getRowStride();
    int destStride = dest->getRowStride();

    //int pixelStride = 3;
    // copy image
    unsigned char *destPtr = dest->getData() + color_channels * destPos.x + destStride * destPos.y;
    unsigned char *srcPtr = src->getData() + color_channels * srcPos.x + srcStride * srcPos.y;

    if (padX)
    {
        for (int y = srcSize.h; y; y--)
        {
            // pad texture
            memcpy(destPtr, srcPtr, srcSize.w * color_channels);
            unsigned char *srcPtrt = srcPtr + color_channels * (srcSize.w - 1);
            unsigned char *destPtrt = destPtr + srcSize.w * color_channels;
            for (int x = destPos.x + srcSize.w; x < dest->size().w; x++)
            {
                for (int j = 0; j < color_channels; j++)
                    *destPtrt++ = *(srcPtrt + j);
            }
            srcPtr += srcStride;
            destPtr += destStride;
        }
    }
    else
    {
        for (int y = srcSize.h; y; y--)
        {
            memcpy(destPtr, srcPtr, srcSize.w * color_channels);
            destPtr += destStride;
            srcPtr += srcStride;
        }
    }
    srcPtr -= srcStride;
    if (padY)
    {
        for (int y = (destPos.y + srcSize.h); y < dest->size().h; y++)
        {
            memcpy(destPtr, srcPtr, srcSize.w * color_channels);
            unsigned char *srcPtrt = srcPtr + color_channels * (srcSize.w - 1);
            unsigned char *destPtrt = destPtr + srcSize.w * color_channels;
            for (int x = destPos.x + srcSize.w; x < dest->size().w; x++)
            {
                for (int j = 0; j < color_channels; j++)
                    *destPtrt++ = *(srcPtrt + j);
            }
            destPtr += destStride;
        }
    }
}

Image **FPV::ChopToCubeFace(Image &img)
{
    Image **faces = new Image *[6];
    int rowbyte;
    unsigned char *img_data;
    unsigned char *sub_img_data;
    if (img.size().w > img.size().h)
    {
        rowbyte = (img.size().w / 6) * img.getColorChannels();
        Size2D sub_size;
        sub_size.w = img.size().w / 6;
        sub_size.h = img.size().h;
        img_data = img.getData();
        for (int i = 0; i < 6; i++)
            faces[i] = new Image(sub_size, img.getType());
        for (int h = 0; h < sub_size.h; h++)
        {
            for (int i = 0; i < 6; i++)
            {
                sub_img_data = faces[i]->getData();
                for (int j = 0; j < rowbyte; j++)
                    sub_img_data[j + (h * rowbyte)] = img_data[(i * rowbyte) + j + (h * rowbyte * 6)];
            }
        };
    }
    else if (img.size().w < img.size().h)
    {
        rowbyte = (img.size().w) * img.getColorChannels();
        Size2D sub_size;
        sub_size.w = img.size().w;
        sub_size.h = img.size().h / 6;
        img_data = img.getData();
        for (int i = 0; i < 6; i++)
            faces[i] = new Image(sub_size, img.getType());
        for (int i = 0; i < 6; i++)
        {
            for (int h = 0; h < sub_size.h; h++)
            {

                sub_img_data = faces[i]->getData();
                for (int j = 0; j < rowbyte; j++)
                    sub_img_data[j + (h * rowbyte)] = img_data[(i * rowbyte * sub_size.h) + j + (h * rowbyte)];
            }
        };
    }
    return faces;
}
