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

#include "pngReader.h"
#include "utils.h"
#include <png.h>
#include <assert.h>

namespace FPV
{
  bool decodePNG(FILE *p_file, Image &img)
  {
    //Rowbytes
    unsigned int rowbytes;

    //data
    unsigned char *data;

    //Image width
    png_uint_32 png_width;
    int width;

    //Image height
    png_uint_32 png_height;
    int height;

    //Depth
    int depth;

    //Color_type
    int color_t;

    //Pointer to PNG rows
    png_bytepp p_png_rows = NULL;

    //Pointer to PNG structure
    png_structp p_png_struct;

    //Pointer to PNG's information structure
    png_infop p_png_info;
    png_infop p_png_end;

    //Signature for png files
    unsigned char png_signature[8];

    if (!p_file)
      return false;

    //The first 8 bytes are readed
    fread(png_signature, 1, 8, p_file);

    //The signature is checked by using the first 8 bytes
    if (png_sig_cmp(png_signature, 0, 8))
      return false;

    //A new PNG structure is created
    p_png_struct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!p_png_struct)
    {
      fclose(p_file);
      return false;
    }

    //A new PNG info structure is created
    p_png_info = png_create_info_struct(p_png_struct);

    if (!p_png_info)
    {
      png_destroy_read_struct(&p_png_struct, NULL, NULL);
      fclose(p_file);
      return false;
    }

    p_png_end = png_create_info_struct(p_png_struct);

    if (!p_png_end)
    {
      png_destroy_read_struct(&p_png_struct, &p_png_info, NULL);
      fclose(p_file);
      return false;
    }

    if (setjmp(png_jmpbuf(p_png_struct)))
    {
      png_destroy_read_struct(&p_png_struct, &p_png_info, &p_png_end);
      fclose(p_file);
      return false;
    }

    //The input code is set up
    png_init_io(p_png_struct, p_file);

    //Then, the numbers of read bytes are
    //indicated to libpng.
    png_set_sig_bytes(p_png_struct, 8);

    //Lets read the file
    png_read_info(p_png_struct, p_png_info);

    //The low level interface is used in order
    //to get the png info.

    png_get_IHDR(p_png_struct, p_png_info, &png_width, &png_height, &depth, &color_t, NULL, NULL, NULL);
    height = (unsigned int)png_height;
    width = (unsigned int)png_width;

    //Transforming the image to RGB(A)

    //we need to expand the palette
    if (color_t == PNG_COLOR_TYPE_PALETTE)
      png_set_expand(p_png_struct);

    //Transfor grayscale images with less
    //than 8 bits to 8 bits
    if (color_t == PNG_COLOR_TYPE_GRAY && depth < 8)
      png_set_expand_gray_1_2_4_to_8(p_png_struct);

    //Add a full alpha channel if there is
    //transparency information in the tRNS chunk
    if (png_get_valid(p_png_struct, p_png_info, PNG_INFO_tRNS))
      png_set_tRNS_to_alpha(p_png_struct);

    //we need to transform grayscale images
    //to rgb images.
    if (color_t == PNG_COLOR_TYPE_GRAY || color_t == PNG_COLOR_TYPE_GRAY_ALPHA)
      png_set_gray_to_rgb(p_png_struct);

    //Strip the pixels down to 8 bits
    if (depth == 16)
      png_set_strip_16(p_png_struct);

    //Now we update the png info structure
    png_read_update_info(p_png_struct, p_png_info);

    //We get the number of bytes in a row
    rowbytes = png_get_rowbytes(p_png_struct, p_png_info);

    //data=(unsigned char*)malloc(height*rowbytes);

    Size2D size;
    size.w = width;
    size.h = height;
    img.setSize(size, (colorChannels)png_get_channels(p_png_struct, p_png_info));
    data = img.getData();

    if (data == NULL)
    {
      png_destroy_read_struct(&p_png_struct, &p_png_info, &p_png_end);
      fclose(p_file);
      return false;
    }

    p_png_rows = new png_bytep[height];

    if (p_png_rows == NULL)
    {
      png_destroy_read_struct(&p_png_struct, &p_png_info, &p_png_end);
      fclose(p_file);
      return false;
    }

    for (unsigned int i = 0; i < height; i++)
      p_png_rows[i] = data + (i * rowbytes);

    png_read_image(p_png_struct, p_png_rows);

    free(p_png_rows);

    png_read_end(p_png_struct, NULL);

    png_destroy_read_struct(&p_png_struct, &p_png_info, &p_png_end);

    fclose(p_file);

    return true;
  }

  bool decodePNG(unsigned char *data, unsigned int size, infoPNG *&s_png_info, Image *&img)
  {
    //Pointer to PNG structure
    png_structp p_png_struct;

    //Pointer to PNG's information structure
    png_infop p_png_info;

    assert(img);
    //    s_png_info=new infoPNG();
    if (s_png_info)
      s_png_info->img = img;
    else
    {
      s_png_info = new infoPNG();
      s_png_info->img = img;
    }
    //A new PNG structure is created
    p_png_struct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!p_png_struct)
    {
      return false;
    }

    //A new PNG info structure is created
    p_png_info = png_create_info_struct(p_png_struct);

    if (!p_png_info)
    {
      png_destroy_read_struct(&p_png_struct, NULL, NULL);
      return false;
    }

    if (setjmp(png_jmpbuf(p_png_struct)))
    {
      png_destroy_read_struct(&p_png_struct, &p_png_info, NULL);
      return false;
    }

    s_png_info->p_png_struct = p_png_struct;
    s_png_info->p_png_info = p_png_info;

    //Add the callback functions and feed the infoPNG structure
    png_set_progressive_read_fn(p_png_struct, s_png_info, png_info_clbk, png_row_clbk, png_end_clbk);

    //if(png_sig_cmp(data,0,8))
    //std::cerr<<"not a png file"<<std::endl;

    //Now the data in the buffer is decoded.
    png_decode_data(data, size, s_png_info);

    return true;
  }

  bool check_png_signature(unsigned char *data, unsigned int size)
  {
    if (size < 8 || data == NULL)
      return false;
    if (png_sig_cmp(data, 0, 8))
      return false;
    return true;
  }

  void png_info_clbk(png_structp p_png_struct, png_infop p_png_info)
  {
    //When this function is called, the image is full loaded
    //here we try to get the image information
    //and allocate the memory for the image's data.

    //First we get the infoPNG struct
    infoPNG *info;
    int color_t, depth;
    info = (infoPNG *)png_get_progressive_ptr(p_png_struct);

    assert(info->img);

    //Now we get the image information
    png_get_IHDR(p_png_struct, p_png_info, &info->png_width, &info->png_height, &depth, &color_t, NULL, NULL, NULL);

    //Transforming the image to RGB(A)

    //we need to expand the palette
    if (color_t == PNG_COLOR_TYPE_PALETTE)
      png_set_expand(p_png_struct);

    //Transfor grayscale images with less
    //than 8 bits to 8 bits.
    if (color_t == PNG_COLOR_TYPE_GRAY && depth < 8)
      png_set_expand_gray_1_2_4_to_8(p_png_struct);

    //Add a full alpha channel if there is
    //transparency information in the tRNS chunk
    if (png_get_valid(p_png_struct, p_png_info, PNG_INFO_tRNS))
      png_set_tRNS_to_alpha(p_png_struct);

    //we need to transform grayscale images
    //to rgb images.
    if (color_t == PNG_COLOR_TYPE_GRAY || color_t == PNG_COLOR_TYPE_GRAY_ALPHA)
      png_set_gray_to_rgb(p_png_struct);

    //Strip the pixels down to 8 bits
    if (depth == 16)
      png_set_strip_16(p_png_struct);

    //Now we update the png info structure
    png_read_update_info(p_png_struct, p_png_info);

    //Now that we have the information
    //we allocate the memory for the image.
    info->channels = png_get_channels(p_png_struct, p_png_info);
    Size2D size;
    size.w = (int)info->png_width;
    size.h = (int)info->png_height;
    info->rowbytes = png_get_rowbytes(p_png_struct, p_png_info);
    info->img->getData();

    if ((int)info->channels == 4)
      info->img->setSize(size, RGBA);
    else
      info->img->setSize(size, RGB);

    info->data = info->img->getData();
    info->p_png_rows = new png_bytep[info->png_height];

    if (info->p_png_rows == NULL)
    {
      png_destroy_read_struct(&p_png_struct, &p_png_info, NULL);
      return;
    }

    for (png_uint_32 i = info->png_height; i > 0; i--)
      info->p_png_rows[i - 1] = info->data + ((info->png_height - i) * info->rowbytes);
  }

  void png_row_clbk(png_structp p_png_struct, png_bytep row, png_uint_32 row_number, int pass)
  {
    infoPNG *info;
    info = (infoPNG *)png_get_progressive_ptr(p_png_struct);
    png_progressive_combine_row(p_png_struct, info->p_png_rows[info->png_height - row_number - 1], row);
    //png_progressive_combine_row(p_png_struct, info->p_png_rows[row_number], row);
    return;
  }

  void png_end_clbk(png_structp p_png_struct, png_infop p_png_info)
  {
    infoPNG *info;
    info = (infoPNG *)png_get_progressive_ptr(p_png_struct);
    info->done = true;
    png_destroy_read_struct(&p_png_struct, &p_png_info, NULL);
  }

  void png_decode_data(unsigned char *rawbuf, unsigned int size, infoPNG *info)
  {
    if (setjmp(png_jmpbuf(info->p_png_struct)))
    {
      png_destroy_read_struct(&info->p_png_struct, &info->p_png_info, (png_infopp)NULL);
      return;
    }
    info->empty = false;
    png_process_data(info->p_png_struct, info->p_png_info, rawbuf, (unsigned long)size);
  }
} // namespace FPV
