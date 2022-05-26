/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Leon Moctezuma <densedev_at_gmail_dot_com>
 *
 *  $Id$
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

#ifndef FPV_PNG_H
#define FPV_PNG_H

#include "Image.h"
#include <iostream>
#include <stdlib.h>
#include <png.h>

namespace FPV
{
  typedef struct s_infoPNG
  {
    png_structp p_png_struct;
    png_infop p_png_info;
    png_uint_32 png_width;
    png_uint_32 png_height;
    unsigned char * data;
    png_bytepp p_png_rows;
    unsigned int rowbytes;
    png_byte channels;
    bool done;
    Image *img;
    bool empty;
    s_infoPNG()
    {
      p_png_struct=NULL;
      p_png_info=NULL;
      rowbytes=0;
      channels=0;
      done=false;
      empty=true;
      img = NULL;
      p_png_rows=NULL;
      png_width=0;
      png_height=0;
      data = NULL;
    }
  } infoPNG;
  bool decodePNG(FILE *p_file, Image &img);
  bool decodePNG(unsigned char* data, unsigned int size, infoPNG *&s_png_info , Image *&img);
  void png_info_clbk(png_structp p_png_struct, png_infop p_png_info);
  void png_row_clbk(png_structp p_png_struct, png_bytep row, png_uint_32 row_number, int pass);
  void png_end_clbk(png_structp p_png_struct, png_infop p_png_info);
  void png_decode_data(unsigned char* rawbuf, unsigned int size,infoPNG *info );
  bool check_png_signature(unsigned char* data, unsigned int size);
}
#endif
