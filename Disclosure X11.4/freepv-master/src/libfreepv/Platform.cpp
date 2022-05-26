/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer
 *
 *  Author: Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: Platform.cpp 41 2006-09-29 23:55:56Z dangelo $
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

#include "Platform.h"

using namespace FPV;

Platform::Platform()
    : m_eventListener(0)
{
    
}

void Platform::setListener(PlatformEventListener * listener)
{
    m_eventListener = listener;
}

