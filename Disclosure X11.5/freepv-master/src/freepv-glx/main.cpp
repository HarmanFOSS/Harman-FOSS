/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Pablo d'Angelo <pablo.dangelo@web.de>
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
 
#include "glx_platform.h"

#include <libfreepv/PanoViewer.h>

using namespace FPV;

int main(int argc, char **argv)
{
    GLXPlatformStandalone platform(argc, argv);
    fprintf(stderr,"before processing events\n");

    // setup viewer
    PanoViewer* viewer=PanoViewer::Instance();

    FPV::Parameters para;
    para.parse(argc, argv);
    
    viewer->init(platform, para);
    viewer->start();
    // run event loop
    platform.ProcessEvents();
    return 0;
}
