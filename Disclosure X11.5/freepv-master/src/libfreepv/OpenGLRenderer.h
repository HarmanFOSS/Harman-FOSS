/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: OpenGLRenderer.h 150 2008-10-15 14:18:53Z leonox $
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

#ifndef FPV_OPENGLRENDERER_H
#define FPV_OPENGLRENDERER_H

#include "Renderer.h"

#if defined(_WIN32)
/* freepv now tries to avoid including <windows.h>
   to avoid name space pollution, but Win32's <GL/gl.h> 
   needs APIENTRY and WINGDIAPI defined properly. */
# if 0
   /* This would put tons of macros and crap in our clean name space. */
#  define  WIN32_LEAN_AND_MEAN
#  include <windows.h>
# else
   /* XXX This is from Win32's <windef.h> */
#  ifndef APIENTRY
#   define GLUT_APIENTRY_DEFINED
#   if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED) || defined(__BORLANDC__) || defined(__LCC__)
#    define APIENTRY    __stdcall
#   else
#    define APIENTRY
#   endif
#  endif
   /* XXX This is from Win32's <winnt.h> */
#  ifndef CALLBACK
#   if (defined(_M_MRX000) || defined(_M_IX86) || defined(_M_ALPHA) || defined(_M_PPC)) && !defined(MIDL_PASS) || defined(__LCC__)
#    define CALLBACK __stdcall
#   else
#    define CALLBACK
#   endif
#  endif
   /* XXX Hack for lcc compiler.  It doesn't support __declspec(dllimport), just __stdcall. */
#  if defined( __LCC__ )
#   undef WINGDIAPI
#   define WINGDIAPI __stdcall
#  else
   /* XXX This is from Win32's <wingdi.h> and <winnt.h> */
#   ifndef WINGDIAPI
#    define GLUT_WINGDIAPI_DEFINED
#    define WINGDIAPI __declspec(dllimport)
#   endif
#  endif
   /* XXX This is from Win32's <ctype.h> */
#  ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#   define _WCHAR_T_DEFINED
#  endif
# endif
#endif  /* _WIN32 */

#ifdef HAVE_OSXGLUT_H
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

typedef GLfloat Point3D[3];

/*
struct Point3D
{
    GLfloat         x,y,z;
};
*/

struct TextureUV
{
    GLfloat         u,v;
};



namespace FPV
{

class OpenGLRenderer : public Renderer
{
public:

    OpenGLRenderer(Platform * platform, size_t m_maxTexMem, RenderQuality q = RQ_HIGH);

    /** Prepare render for rendering
     *
     *  This should be called if all resources required by
     *  the renderer are available (for example: opengl context)
     */
    virtual void init();

    virtual void initElement(SceneElement & pano);
    virtual void render(Scene & scene);
    virtual void resize(Size2D sz);
    virtual void max_depth(float maxz);
    virtual void min_depth(float minz);
    std::list<Subject*>* getPointedSubjects(Scene &scene, Point2D);

private:
    void changeCurrentDepth(SceneElement &pano);
    RenderQuality m_quality;

    Size2D m_size;
    Platform * m_platform;
    size_t m_maxTexMem;
    float m_maxDepth;
    float m_minDepth;

    //This is the Depth in which 
    //an elment is rendered

    float m_currentDepth;
};

} // namespace
#endif
