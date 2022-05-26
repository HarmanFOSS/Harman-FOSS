#ifndef  __FREEGLUT_FONT_COPY_H__
#define  __FREEGLUT_FONT_COPY_H__

/*
 * freeglut_std.h
 *
 * The GLUT-compatible part of the freeglut library include file
 *
 * Copyright (c) 1999-2000 Pawel W. Olszta. All Rights Reserved.
 * Written by Pawel W. Olszta, <olszta@sourceforge.net>
 * Creation date: Thu Dec 2 1999
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PAWEL W. OLSZTA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifdef __cplusplus
    extern "C" {
#endif

#ifdef _WIN32
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

#define FGAPIENTRY
#define FGAPI
/*
 * Always include OpenGL and GLU headers
 */

#ifdef HAVE_OSXGLUT_H
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

/*
 * GLUT API macro definitions -- fonts definitions
 *
 * Steve Baker suggested to make it binary compatible with GLUT:
 */
#   define  FPVGLUT_BITMAP_9_BY_15             2
#   define  FPVGLUT_BITMAP_8_BY_13             3
#   define  FPVGLUT_BITMAP_TIMES_ROMAN_10      4
#   define  FPVGLUT_BITMAP_TIMES_ROMAN_24      5
#   define  FPVGLUT_BITMAP_HELVETICA_10        6
#   define  FPVGLUT_BITMAP_HELVETICA_12        7
#   define  FPVGLUT_BITMAP_HELVETICA_18        8

#if 0
struct freeglutBitmapFont
{
    const char *name ;
    int num_chars ;
    int first ;
    void *ch ;
};


extern struct freeglutBitmapFont glutBitmap9By15 ;
extern struct freeglutBitmapFont glutBitmap8By13 ;
extern struct freeglutBitmapFont glutBitmapTimesRoman10 ;
extern struct freeglutBitmapFont glutBitmapTimesRoman24 ;
extern struct freeglutBitmapFont glutBitmapHelvetica10 ;
extern struct freeglutBitmapFont glutBitmapHelvetica12 ;
extern struct freeglutBitmapFont glutBitmapHelvetica18 ;

    /*
     * Those pointers will be used by following definitions:
     */
#   define  GLUT_BITMAP_9_BY_15             ((void *) &glutBitmap9By15)
#   define  GLUT_BITMAP_8_BY_13             ((void *) &glutBitmap8By13)
#   define  GLUT_BITMAP_TIMES_ROMAN_10      ((void *) &glutBitmapTimesRoman10)
#   define  GLUT_BITMAP_TIMES_ROMAN_24      ((void *) &glutBitmapTimesRoman24)
#   define  GLUT_BITMAP_HELVETICA_10        ((void *) &glutBitmapHelvetica10)
#   define  GLUT_BITMAP_HELVETICA_12        ((void *) &glutBitmapHelvetica12)
#   define  GLUT_BITMAP_HELVETICA_18        ((void *) &glutBitmapHelvetica18)
#endif

/*
 * Font stuff, see freeglut_font.c
 */
FGAPI void    FGAPIENTRY FPVglutBitmapCharacter( int font, int character );
FGAPI int     FGAPIENTRY FPVglutBitmapWidth( int font, int character );
FGAPI int     FGAPIENTRY FPVglutBitmapLength( int font, const unsigned char* string );
FGAPI void    FGAPIENTRY FPVglutBitmapString( int fontID, const unsigned char *string );



#ifdef __cplusplus
    }
#endif

/*** END OF FILE ***/

#endif /* __FREEGLUT_STD_H__ */

