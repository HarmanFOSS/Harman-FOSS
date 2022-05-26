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

#include "imageReader.h"

namespace FPV
{
  bool decodeImage(FILE *p_file, Image *&img, std::string type)
  {
    unsigned char signature[8];
    if(img==NULL)
       img=new Image();
    if(type=="AUTO")
    {
       fread( signature, sizeof(unsigned char), 8, p_file);
       if(check_png_signature(signature, 8))
          type="PNG";
       else
          type="JPG";
       rewind(p_file);
    }
    if(type=="JPG")
      return decodeJPEG(p_file,*img);
    else if(type=="PNG")
    {
      return decodePNG(p_file,*img);
    }
    return false;
  }

  bool decodeImage(unsigned char* data, unsigned int size, Image *&img, std::string type)
  {
    //static infoPNG *info;
    
    if(type=="AUTO")
    {
      if(check_png_signature(data, size))
	type="PNG";
      else
	type="JPEG";
    }
    if(type=="JPG")
    {  
      if(img==NULL)
      	img=new Image();
      return decodeJPEG(data, size, *img);
    }
    else if(type=="PNG")
    {
      infoPNG *info=new infoPNG();
      if(img==NULL)
      	img=new Image();
      return decodePNG(data,size, info, img);
    }
    return false;
  }

}
