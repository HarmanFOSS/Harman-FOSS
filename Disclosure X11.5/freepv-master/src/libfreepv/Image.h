/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: Image.h 150 2008-10-15 14:18:53Z leonox $
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

/* This file contains a platform independent interface for the
 *  freepv player
 */

#ifndef FPV_IMAGE_H
#define FPV_IMAGE_H

//#include <vector>
#include <string>
#include <assert.h>
#include <cstdlib>

#include "Platform.h"

namespace FPV
{

    enum colorChannels{GRAY=1,RGB=3,RGBA=4};

/** This is a very simple image type.
 *
 *  Just raw RGB data so far, in RGB RGB triplets,
 *  row column memory organisation.
 *
 */
class Image
{

    typedef unsigned char T;

public:

    Image()
    {
        m_data = 0;
        m_rowStride = 0;
	m_color_channels=RGB;
    }

    Image(Size2D sz, colorChannels channels = RGB)
    {
        m_data = 0;
        setSize(sz, channels);
	m_color_channels=channels;
    }

    virtual ~Image()
    {
        if (m_data) {
            free(m_data);
        }
    }

    /** Set image size.
     *  
     *  Can be used to resize an existing image as well.
     *  In this case, the previous image data is lost
     */
    bool setSize(Size2D size, colorChannels channels = RGB);

    unsigned char * getData()
    {
	return m_data;  
    }

    size_t getRowStride()
    {   return m_rowStride;   }
    
    int getColorChannels(){
	return (int)m_color_channels;}

    colorChannels getType(){
        return m_color_channels;
    }

    Size2D size()
    {   return m_size;    }

    /** access the red pixel component at coordinates \p x and \p y */
    unsigned char & operator()(int x, int y) 
    {
        assert(x < m_size.w);
        assert(y < m_size.h);
        return m_data[m_rowStride*y + getColorChannels()*x];
    }

    /** write image to a ppm file.
     *
     *  Useful for debugging
     */
    void writePPM(std::string file);

    /** return a subset of an image */
    Image * getSubImage(Point2D pos, Size2D size);

protected:
    unsigned char * m_data;
    Size2D m_size;
    size_t m_rowStride;
    colorChannels m_color_channels;

private:
// copying is forbidden.
    Image(const Image & other) {};
    void operator=(const Image & other) {};
};

// various utility functions for image "processing"
/** Copy the \p src image into the \p dest image.
 *  \p destPos gives the target position in the dest image.
 *
 *  if the src image is to large, nothing will be copied.
 *
 *  This function will repeat the src image 1 pixel on the
 *  left and bottom edge, if there is space in the source image.
 *  This will lead to textures that can be used for mip-mapping.
 */
void copyImgToTexImg(Image * dest, Image * src, Point2D destPos,
                     Point2D srcPos, Size2D srcSize=Size2D(-1,-1),
                     bool pad = false);

Image ** ChopToCubeFace(Image &img);

} // namespace

#endif
