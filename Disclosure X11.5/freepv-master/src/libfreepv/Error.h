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

#ifndef FPV_ERROR_H
#define FPV_ERROR_H

#include <string>
#include <stdexcept>
#include "Image.h"

namespace FPV
{

class Error : public std::runtime_error{
 public:
   Error(const std::string &_arg) throw();
   virtual ~Error() throw();
};

class ImageError : public Error{
 private:
   std::string m_msg;
 public:
   ImageError(Image *_img) throw();
   virtual ~ImageError() throw();
   const char* what() const throw();
};

class ZeroDivision : public Error{
 public:
   ZeroDivision() throw();
   virtual ~ZeroDivision() throw();
};

}//namespace

#endif
