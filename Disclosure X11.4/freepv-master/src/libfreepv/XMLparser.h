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

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <string>

namespace FPV
{
  enum search{DEPTH,WIDE};

  /*! XMLparser is a base class for parsing 
      XML documents, it uses libxml2 library */
  class XMLparser{
  public:

    //!Constructor
    XMLparser();
    //!Destructor
    virtual ~XMLparser()=0;
    //! Method to parse a XML documents file.
    bool parseURL(const char*);
    //! Method to parse documents from a buffer.
    bool parseBuffer(const char* buffer, int size);

    xmlDocPtr m_doc;

    /*!Method to convert a string to an integer value.*/
    int getInteger(unsigned char* value);

    /*!Method to convert a string to a double value.*/
    double getDouble(unsigned char* value);

    /*!Method to convert a string to a float value.*/
    float getFloat(unsigned char* value);

    /*!Method to convert a string to a boolean value.*/ 
    bool getBool(const unsigned char* strA);

    /*!Method to compare two different strings.
    /return true if the strings are equal.*/
    bool cmp(const char* srcA, const unsigned char*);

    /*!FindElement is an useful method to find elements based in
      their name, a particular attribute or a particular value.
      \note node must be different of NULL and at least a name or attribute must
      be specified, otherwise the function returns a NULL pointer.
      \param if the node is a NULL pointer, the method will return a NULL pointer.
      \param if the name is a NULL pointer, the method ignores the name of the node.
      \param if the attr is a NULL pointer, the method ignores the attributes.
      \param if the value is a NULL pointer, the method ignores the value of the attribute.
      \param to specify the type of search algorithm that will be used. [DEPTH|WIDE}]
      \return pointer to the node that has been found.
      \todo creat new types of search algorithms or improve the ones that already exist.
    */ 
    xmlNodePtr FindElement(xmlNodePtr node, const char *name, const char *attr, const char *value, search type);

    /*!validateElement is an useful to verify if an elements fulfil
      with a name, a particular attribute or a particular value.
      \note node must be different of NULL and at least a name or attribute must
      be specified, otherwise the function returns false.
      \param if the node is a NULL pointer, the method will return false.
      \param if the name is a NULL pointer, the method ignores the name of the node.
      \param if the attr is a NULL pointer, the method ignores the attributes.
      \param if the value is a NULL pointer, the method ignores the value of the attribute.
      \return boolean value true if the node fulfils.
      \todo creat a similar method that can accept more than one attribute and/or values.
    */
    bool validateElement(xmlNodePtr node, const char* name, const char* attr, const char* value);
  };
}
