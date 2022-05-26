/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Thomas Rauscher <t.rauscher@sinnfrei.at>
 *          Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: freepv-glut.cpp 155 2009-02-22 10:56:20Z brunopostle $
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

#include <config.h>

#ifdef _WIN32
#define FREEGLUT_STATIC
#endif

#if defined HAVE_OSXGLUT_H
#include <GLUT/glut.h>
#elif defined HAVE_FREEGLUT_H
#include <GL/freeglut.h>
#elif defined HAVE_GLUT_H
#include <GL/glut.h>
#else
#error "GLUT or freeglut not found, and configure (or cmake) checks failed"
#endif

#include <vector>
#include <string>
#include <fstream>

#include "glut_platform.h"
#include <libfreepv/PanoViewer.h>

#ifdef _WIN32
#include <shfolder.h>
#endif

GLUTPlatformStandalone * platformptr;
FPV::PanoViewer * gviewer=0;
static bool g_started = false;


#if 0

var reg :TRegistry;
    hs :string;
    Buffer: array[0..MAX_PATH] of Char;

begin
  result:=0;
  try
    reg:=TRegistry.Create(KEY_READ);
    reg.RootKey:=HKEY_LOCAL_MACHINE;
    reg.OpenKeyReadOnly('HARDWARE\DEVICEMAP\VIDEO');
    hs :=reg.ReadString('\Device\Video0');
    hs:=Copy(hs,length('\Registry\Hardware'),10000);
    reg.OpenKeyReadOnly(hs);
    reg.ReadBinaryData('HardwareInformation.MemorySize',Buffer, 4);
    result:=PDWORD(@Buffer)^;
//    DisplayMemoryLabel.Caption := Format('%.1f MB', [PDWORD(@Buffer)^ / OneMegabyte]);
  except
  end;
  reg.Free;
end;

#endif


void keyCallback(unsigned char key,int x, int y) {
	platformptr->glutKeyboardCallback(key,x,y, true);
}

void keyCallbackUp(unsigned char key,int x, int y) {
    platformptr->glutKeyboardCallback(key,x,y, false);
}

void keySpecialCallback(int key,int x, int y) {
    DEBUG_TRACE(key)
    platformptr->glutKeyboardSpecialCallback(key,x,y, true);
}

void keySpecialCallbackUp(int key,int x, int y) {
    DEBUG_TRACE(key);
    platformptr->glutKeyboardSpecialCallback(key,x,y, false);
}

void mouseCallback(int button, int state,int x, int y) {
	platformptr->glutMouseCallback(button,state,x,y);
}

void scrollWheelCallback(int button, int state,int x, int y) {
	platformptr->glutScrollWheelCallback(button,state,x,y);
}

void displayCallback() {
	platformptr->glutDisplayCallback();
    if (!g_started && gviewer) {
        gviewer->start();
        g_started=true;
    };
}

void mouseMotionCallback(int x, int y) {
	platformptr->glutMouseMotionCallback(x,y);
}

void timerCallback(int id) {
	platformptr->glutOnTimerCallback();
}

void reshapeCallback(int width, int height) {
    platformptr->glutReshapeCallback(width, height);
}

int main(int argc,char * argv[])
{
    GLUTPlatformStandalone platform=GLUTPlatformStandalone(argc,argv);
	platformptr=&platform;
    fprintf(stderr,"before processing events\n");


    // setup viewer
    PanoViewer* viewer=PanoViewer::Instance();

    FPV::Parameters para;
    para.parse(argc, argv);

    const char * home = 0; 
    // TODO: read preferences file and add to arguments
#ifdef _WIN32
    // TODO: use the registry here!
    // RegQueryValueEx

#if 0
    TCHAR szPath[MAX_PATH];

    // Default to My Pictures. First, get its path.
    if ( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_APPDATA,
        NULL, 0, szPath ) ) )
    {
        // Set lpstrInitialDir to the path that SHGetFolderPath obtains. 
        // This causes GetSaveFileName to point to the My Pictures folder.
        home = szPath;
    }
#endif
    home = 0;
#else
    home = getenv("HOME");
#endif
    if (home != 0) {
        std::string filename(home);
        filename.append("/.freepv");
        DEBUG_DEBUG("prefs file: " << filename);
        std::ifstream pstream(filename.c_str());
        if (pstream.is_open()) {
            while(pstream.good() && (!pstream.eof())) {
                std::string line;
                std::getline(pstream, line);
                if (line.length() == 0) 
                    continue;
                if (line[0] == '#')
                    continue;
                para.parse(line.c_str());
            }
        }
    }

    viewer->init(platform, para);
    gviewer = viewer;

    // disable keyboard repeat
    glutIgnoreKeyRepeat(1);

	glutDisplayFunc(displayCallback);
	glutKeyboardFunc(keyCallback);
    glutKeyboardUpFunc(keyCallbackUp);
    glutSpecialFunc(keySpecialCallback);
    glutSpecialUpFunc(keySpecialCallbackUp);
    glutMouseFunc(mouseCallback);
    glutPassiveMotionFunc(mouseMotionCallback);
    glutMotionFunc(mouseMotionCallback);
#ifdef HAVE_FREEGLUT_H
    glutMouseWheelFunc(scrollWheelCallback);
#endif
//	glutTimerFunc(10,timerCallback,0);
    glutReshapeFunc(reshapeCallback);

    // start viewer. This will immediately start downloading and
    // initializing the viewer, even if no window is shown (on unix)...
    //viewer.start();

	glutMainLoop();
	return 0;
}

