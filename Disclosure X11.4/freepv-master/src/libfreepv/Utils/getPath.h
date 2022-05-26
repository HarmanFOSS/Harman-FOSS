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

#ifndef UTILS_GET_PATH_H
#define UTILS_GET_PATH_H

//get the path from a string
#include <string>

namespace FPV{
  namespace Utils{
    //!This function gets the path from a file.
    std::string getPath(const char* _str){
      std::string str=_str;
      std::string aux="";
      size_t pos;
      //it looks for the last '/'
      pos=str.rfind("/");
      if(pos!=std::string::npos){
	//gets the substring from the beginning
	//until the position of '/' character.
	aux=str.substr(0,pos+1);
      }
      return aux;
    }
  }
}

#endif
