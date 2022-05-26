/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: utils.cpp 61 2006-10-07 23:35:15Z dangelo $
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

#include <algorithm>
#include <cstdio>

#include "utils.h"

#ifdef unix
#include <sys/time.h>
#include <time.h>
#endif

std::string FPV::getExtension(const std::string &basename2)
{
    std::string::size_type idx = basename2.rfind('.');
    // check if the dot is not followed by a \ or a /
    // to avoid cutting pathes.
    if (idx == std::string::npos)
    {
        // no dot found
        return std::string("");
    }
    // check for slashes after dot
    std::string::size_type slashidx = basename2.find('/', idx);
    std::string::size_type backslashidx = basename2.find('\\', idx);
    if (slashidx == std::string::npos && backslashidx == std::string::npos)
    {
        return basename2.substr(idx + 1);
    }
    else
    {
        return std::string("");
    }
}

std::string FPV::removeWhitespace(const std::string &str)
{

    std::string::size_type begin, end;

    begin = str.find_first_not_of(" ");
    end = str.find_last_not_of(" ");
    if (begin == end)
    {
        return "";
    }
    return str.substr(begin, end - begin + 1);
}

std::string FPV::stripPath(const std::string &filename)
{
    std::string::size_type idx1 = filename.rfind('\\');
    std::string::size_type idx2 = filename.rfind('/');
    std::string::size_type idx;
    if (idx1 == std::string::npos)
    {
        idx = idx2;
    }
    else if (idx2 == std::string::npos)
    {
        idx = idx1;
    }
    else
    {
        idx = std::max(idx1, idx2);
    }
    if (idx != std::string::npos)
    {
        //        DEBUG_DEBUG("returning substring: " << filename.substr(idx + 1));
        return filename.substr(idx + 1);
    }
    else
    {
        return filename;
    }
}

std::string FPV::string2UPPER(const std::string &str)
{
    std::string ret = str;
    std::transform(ret.begin(), ret.end(), ret.begin(), toupper);
    return ret;
}

#ifdef unix
std::string FPV::CurrentTimeStr()
{
    char tmp[100];
    struct tm t;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &t);
    strftime(tmp, 99, "%H:%M:%S", &t);
    sprintf(tmp + 8, ".%06ld", tv.tv_usec);
    return tmp;
}
#else
std::string FPV::CurrentTimeStr()
{
    // FIXME implement for Win & Mac
    return "";
}
#endif
