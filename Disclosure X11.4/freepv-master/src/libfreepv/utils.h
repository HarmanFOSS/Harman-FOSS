/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: utils.h 79 2006-10-12 18:45:46Z dangelo $
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

#ifndef FPV_UTILS_H
#define FPV_UTILS_H

#include <string>
#include <iostream>
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979
#endif

namespace FPV
{

/** simple structure for a 2d point with integer coordinates. */
struct Point2D
{
    Point2D()
    { x=0; y=0; }
    
    Point2D(int x_, int y_)
    {
        x = x_;
        y = y_;
    }

    Point2D operator-(const Point2D & other) const
    {
        return Point2D(x-other.x, y-other.y);
    }

    bool operator==(const Point2D & other) const
    {
        return (other.x == x && other.y == y);
    }
    int x,y;
};

struct Size2D
{
    Size2D() { w=0; h=0; }
    Size2D(int w_, int h_)
    {
        w = w_;
        h = h_;
    }

    bool operator==(const Size2D & other) const
    {
        return (other.w == w && other.h == h);
    }

    Size2D operator-(const Size2D & other) const
    {
        return Size2D(w-other.w, h-other.h);
    }

    int w,h;
};

/** get the file extension */
std::string getExtension(const std::string & basename2);

/** remove the path of a filename (mainly useful for gui
 *  display of filenames)
 */
std::string stripPath(const std::string & filename);

/** remove leading and trailing white space */
std::string removeWhitespace(const std::string & str);

std::string string2UPPER(const std::string & str);

template <class T>
T r2d(T x)
{
    return static_cast<T>(x/M_PI*180.0);
}

template <class T>
T d2r(T x)
{
    return static_cast<T>(x/180.0*M_PI);
}


#define FPV_S2S(output, msg) { std::stringstream o; o << msg; output = o.str(); };

///////////////////////////////////////////
// misc functions for debug messages
//

std::string CurrentTimeStr();

#ifdef __GNUC__
// the full function name is too long..
//#define DEBUG_HEADER utils::CurrentTime() << " (" << __FILE__ << ":" << __LINE__ << ") " << __PRETTY_FUNCTION__ << "()" << std::endl << "    "
  #define DEBUG_HEADER FPV::CurrentTimeStr() <<" (" << FPV::stripPath(__FILE__) << ":" << __LINE__ << ") "  << __func__ << "(): "
#else
  #if _MSC_VER > 1300
    #define DEBUG_HEADER FPV::CurrentTimeStr() <<" (" << FPV::stripPath(__FILE__) << ":" << __LINE__ << ") "  << __FUNCTION__ << "(): "
  #else
    #define DEBUG_HEADER FPV::CurrentTimeStr() <<" (" << FPV::stripPath(__FILE__) << ":" << __LINE__ << ") "  << __func__ << "(): "
  #endif
#endif

#ifdef DEBUG
    // debug trace
    #define DEBUG_TRACE(msg) { std::cerr << "TRACE " << DEBUG_HEADER << msg << std::endl; }
    // low level debug info
    #define DEBUG_DEBUG(msg) { std::cerr << "DEBUG " << DEBUG_HEADER << msg << std::endl; }
    // informational debug message,
    #define DEBUG_INFO(msg) { std::cerr << "INFO " << DEBUG_HEADER << msg << std::endl; }
    // major change/operation should use this
    #define DEBUG_NOTICE(msg) { std::cerr << "NOTICE " << DEBUG_HEADER << msg << std::endl; }
#else
    #define DEBUG_TRACE(msg)
    #define DEBUG_DEBUG(msg)
    #define DEBUG_INFO(msg)
    #define DEBUG_NOTICE(msg)
#endif

 // when an error occured, but can be handled by the same function
#define DEBUG_WARN(msg) { std::cerr << "WARN: " << DEBUG_HEADER << msg << std::endl; }
 // an error occured, might be handled by a calling function
#define DEBUG_ERROR(msg) { std::cerr << "ERROR: " << DEBUG_HEADER << msg << std::endl; }
 // a fatal error occured. further program execution is unlikely
#define DEBUG_FATAL(msg) { std::cerr << "FATAL: " << DEBUG_HEADER << "(): " << msg << std::endl; }


}

#endif
