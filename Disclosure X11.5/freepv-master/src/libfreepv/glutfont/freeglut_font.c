/*
 * freeglut_font.c
 *
 * Bitmap and stroke fonts displaying.
 *
 * Copyright (c) 1999-2000 Pawel W. Olszta. All Rights Reserved.
 * Written by Pawel W. Olszta, <olszta@sourceforge.net>
 * Creation date: Thu Dec 16 1999
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

#include <config.h>

#include "freeglut_font_copy.h"

#include <assert.h>
#include <stdio.h>

/* The bitmap font structure */
typedef struct tagFPVSFG_Font FPVSFG_Font;
struct tagFPVSFG_Font
{
    char*           Name;         /* The source font name             */
    int             Quantity;     /* Number of chars in font          */
    int             Height;       /* Height of the characters         */
    const GLubyte** Characters;   /* The characters mapping           */

    float           xorig, yorig; /* Relative origin of the character */
};

/*
 * TODO BEFORE THE STABLE RELEASE:
 *
 *  Test things out ...
 */

/* -- IMPORT DECLARATIONS -------------------------------------------------- */

/*
 * These are the font faces defined in freeglut_font_data.c file:
 */
extern FPVSFG_Font FPVfgFontFixed8x13;
extern FPVSFG_Font FPVfgFontFixed9x15;
extern FPVSFG_Font FPVfgFontHelvetica10;
extern FPVSFG_Font FPVfgFontHelvetica12;
extern FPVSFG_Font FPVfgFontHelvetica18;
extern FPVSFG_Font FPVfgFontTimesRoman10;
extern FPVSFG_Font FPVfgFontTimesRoman24;
//extern FPVSFG_StrokeFont fgStrokeRoman;
//extern FPVSFG_StrokeFont fgStrokeMonoRoman;


/* -- PRIVATE FUNCTIONS ---------------------------------------------------- */

/*
 * Matches a font ID with a FPVSFG_Font structure pointer.
 * This was changed to match the GLUT header style.
 */
static FPVSFG_Font* FPVfghFontByID( int font )
{
    if( font == FPVGLUT_BITMAP_8_BY_13        )
        return &FPVfgFontFixed8x13;
    if( font == FPVGLUT_BITMAP_9_BY_15        )
        return &FPVfgFontFixed9x15;
    if( font == FPVGLUT_BITMAP_HELVETICA_10   )
        return &FPVfgFontHelvetica10;
    if( font == FPVGLUT_BITMAP_HELVETICA_12   )
        return &FPVfgFontHelvetica12;
    if( font == FPVGLUT_BITMAP_HELVETICA_18   )
        return &FPVfgFontHelvetica18;
    if( font == FPVGLUT_BITMAP_TIMES_ROMAN_10 )
        return &FPVfgFontTimesRoman10;
    if( font == FPVGLUT_BITMAP_TIMES_ROMAN_24 )
        return &FPVfgFontTimesRoman24;

    return 0;
}


/* -- INTERFACE FUNCTIONS -------------------------------------------------- */

/*
 * Draw a bitmap character
 */
void FGAPIENTRY FPVglutBitmapCharacter( int fontID, int character )
{
    const GLubyte* face;
    FPVSFG_Font* font;
    font = FPVfghFontByID( fontID );
    assert (( character >= 1 )&&( character < 256 ) );
    assert ( font );

    /*
     * Find the character we want to draw (???)
     */
    face = font->Characters[ character ];

    glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT );
    glPixelStorei( GL_UNPACK_SWAP_BYTES,  GL_FALSE );
    glPixelStorei( GL_UNPACK_LSB_FIRST,   GL_FALSE );
    glPixelStorei( GL_UNPACK_ROW_LENGTH,  0        );
    glPixelStorei( GL_UNPACK_SKIP_ROWS,   0        );
    glPixelStorei( GL_UNPACK_SKIP_PIXELS, 0        );
    glPixelStorei( GL_UNPACK_ALIGNMENT,   1        );
    glBitmap(
        face[ 0 ], font->Height,      /* The bitmap's width and height  */
        font->xorig, font->yorig,     /* The origin in the font glyph   */
        ( float )( face[ 0 ] ), 0.0,  /* The raster advance -- inc. x,y */
        ( face + 1 )                  /* The packed bitmap data...      */
    );
    glPopClientAttrib( );
}

void FGAPIENTRY FPVglutBitmapString( int fontID, const unsigned char *string )
{
    unsigned char c;
    float x = 0.0f ;
    FPVSFG_Font* font;
    font = FPVfghFontByID( fontID );
    assert( font );
    if ( !string || ! *string )
        return;

    glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT );
    glPixelStorei( GL_UNPACK_SWAP_BYTES,  GL_FALSE );
    glPixelStorei( GL_UNPACK_LSB_FIRST,   GL_FALSE );
    glPixelStorei( GL_UNPACK_ROW_LENGTH,  0        );
    glPixelStorei( GL_UNPACK_SKIP_ROWS,   0        );
    glPixelStorei( GL_UNPACK_SKIP_PIXELS, 0        );
    glPixelStorei( GL_UNPACK_ALIGNMENT,   1        );

    /*
     * Step through the string, drawing each character.
     * A newline will simply translate the next character's insertion
     * point back to the start of the line and down one line.
     */
    while( ( c = *string++) )
        if( c == '\n' )
        {
            glBitmap ( 0, 0, 0, 0, -x, (float) -font->Height, NULL );
            x = 0.0f;
        }
        else  /* Not an EOL, draw the bitmap character */
        {
            const GLubyte* face = font->Characters[ c ];

            glBitmap(
                face[ 0 ], font->Height,     /* Bitmap's width and height    */
                font->xorig, font->yorig,    /* The origin in the font glyph */
                ( float )( face[ 0 ] ), 0.0, /* The raster advance; inc. x,y */
                ( face + 1 )                 /* The packed bitmap data...    */
            );

            x += ( float )( face[ 0 ] );
        }

    glPopClientAttrib( );
}

/*
 * Returns the width in pixels of a font's character
 */
int FGAPIENTRY FPVglutBitmapWidth( int fontID, int character )
{
    FPVSFG_Font* font;
    font = FPVfghFontByID( fontID );
    assert( character > 0 && character < 256);
    assert( font);
    return *( font->Characters[ character ] );
}

/*
 * Return the width of a string drawn using a bitmap font
 */
int FGAPIENTRY FPVglutBitmapLength( int fontID, const unsigned char* string )
{
    unsigned char c;
    int length = 0, this_line_length = 0;
    FPVSFG_Font* font;
    font = FPVfghFontByID( fontID );
    assert( font );
    if ( !string || ! *string )
        return 0;

    while( ( c = *string++) )
    {
        if( c != '\n' )/* Not an EOL, increment length of line */
            this_line_length += *( font->Characters[ c ]);
        else  /* EOL; reset the length of this line */
        {
            if( length < this_line_length )
                length = this_line_length;
            this_line_length = 0;
        }
    }
    if ( length < this_line_length )
        length = this_line_length;

    return length;
}

/*
 * Returns the height of a bitmap font
 */
int FGAPIENTRY FPVglutBitmapHeight( int fontID )
{
    FPVSFG_Font* font;
    font = FPVfghFontByID( fontID );
    assert( font);
    return font->Height;
}


/*** END OF FILE ***/
