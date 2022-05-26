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

#include "stringID.h"
#include <cstdlib>
namespace FPV{
namespace Utils{

stringID::stringID(){
   allocate();
}

void stringID::allocate(int length){
   m_length=length;
   if(m_string)
      free(m_string);
   m_string=new char[m_length+1];
   m_string[0]='@';
   for(int i=1; i<m_length+1; i++)
      m_string[i]=65;
   m_string[m_length]='\0';
}

stringID::~stringID(){

}

void stringID::Increment(int i){
   if(i>=m_length)
   {
      allocate(m_length+1);
      return;
   }
   if(m_string[i]<90)
      m_string[i]++;
   else
   {
      m_string[i]=65;
      Increment(i+1);
   }
}

std::string stringID::generate(){
   static stringID generator;
   std::string aux = generator.m_string;
   generator.Increment();
   return aux;
}

}//Utils
}//namespace
