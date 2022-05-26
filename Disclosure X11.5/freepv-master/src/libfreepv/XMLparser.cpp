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

#include "XMLparser.h"
#include <iostream>
#include <cstring>

using namespace FPV;

XMLparser::XMLparser()
{
  m_doc=NULL;
}

XMLparser::~XMLparser()
{
}

bool XMLparser::validateElement(xmlNodePtr node, const char* name, const char* attr, const char* value)
{
  if(node==NULL)
      return false;
  if(name==NULL&&attr==NULL){
      return false;
  }
  else{
      if(name==NULL||cmp(name,node->name)){
	  if(attr!=NULL){
	      if(xmlHasProp(node,(const xmlChar*)attr)!=NULL){
		  if(value==NULL){
		      return true;
		  }else{
		      if(cmp(value,xmlGetProp(node,(const xmlChar*)attr))){
			  return true;
		      }else{
			  return false;
		      }
		  }
	      }else{
		  return false;
	      }
	  }
	  return true;
      }
  }
  return false;
}

int XMLparser::getInteger(unsigned char* value)
{
    if(value==NULL)
	return 0;
    return atoi((const char*) value);
}

double XMLparser::getDouble(unsigned char* value)
{
    if(value==NULL)
	return 0;
    return atof((const char*) value);
}

float XMLparser::getFloat(unsigned char* value)
{
  return (float)getDouble(value);
}

bool XMLparser::cmp(const char * strA, const unsigned char * strB)
{
    if(strcmp(strA, (const char*)strB)==0)
	return true;
    else
	return false;
}

bool XMLparser::getBool(const unsigned char* strA){
    if(cmp("true",strA)) return true;
    return false;
}

xmlNodePtr XMLparser::FindElement(xmlNodePtr node, const char *name, const char *attr, const char *value, search type)
{
    xmlNodePtr p_aux=NULL;
    if(node!=NULL){
	    if(type==DEPTH){
		if(validateElement(node,name,attr,value)){
		    p_aux=node;
		}else{
		    node=node->children;
		    while(p_aux==NULL&&node!=NULL){
			p_aux=FindElement(node, name, attr, value, type);
			node=node->next;
		    }
		}
	    }else if(type==WIDE){
		p_aux=node;
		while(p_aux!=NULL){
		    if(validateElement(p_aux, name,attr,value)){
			break;
		    }
		    p_aux=p_aux->next;
		}
		while(p_aux==NULL&&node!=NULL){
		    p_aux=FindElement(node->children, name, attr, value, type);
		    node=node->next;
		}
	    }
    }
    return p_aux;
}

bool XMLparser::parseURL(const char* url){
    m_doc=xmlParseFile(url);
    if(m_doc==NULL)
	return false;
    return true;
}

bool XMLparser::parseBuffer(const char* buffer, int size)
{
    m_doc=xmlParseMemory(buffer,size);
    if(m_doc==NULL)
	return false;
    return true;
}
