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

#include "Error.h"

namespace FPV
{
Error::Error(const std::string &_arg) throw () : runtime_error("FPV::ERROR\a: "+_arg)
{
}
Error::~Error() throw ()
{
}

ImageError::ImageError(Image *_img) throw() : Error("Image-> ")
{
   if(_img == NULL)
      m_msg="Image null reference";
   else if(_img->getData() == NULL)
      m_msg="Image without data";
   else if(_img->size().w<=0)
      m_msg="Image with width=0";
   else if(_img->size().h==0)
      m_msg="Image with height=0";
}

const char* ImageError::what() const throw()
{
   return (Error::what()+m_msg).c_str();
}

ImageError::~ImageError() throw()
{

}

ZeroDivision::ZeroDivision() throw() : Error("Division by Zero is not defined")
{
}

ZeroDivision::~ZeroDivision() throw()
{
}

}//namespace
