/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: JpegReader.cpp 101 2006-12-01 23:42:33Z dangelo $
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


// jpeg memory data source..

#include <stdlib.h>
#include <stdio.h>

extern "C" {
#include <jpeglib.h>
#include <jerror.h>
}
#include <setjmp.h>

#include "JpegReader.h"


namespace FPV
{


/* Expanded data source object for stdio input */

typedef struct {
    struct jpeg_source_mgr pub;   /* public fields */

    JOCTET * buffer;         /* pointer to memory block (complete) */
    size_t   buffer_size;    /* size of compressed data */
      
} mem_source_mgr;

typedef mem_source_mgr * mem_src_ptr;


/*
 * Initialize source --- called by jpeg_read_header
 * before any data is actually read.
 */

static void
        init_source (j_decompress_ptr cinfo)
{
    mem_src_ptr src = (mem_src_ptr) cinfo->src;
    src->pub.next_input_byte = src->buffer;
    src->pub.bytes_in_buffer = src->buffer_size;
    
}


/*
 * Fill the input buffer --- called whenever buffer is emptied.
 *
 * In typical applications, this should read fresh data into the buffer
 * (ignoring the current state of next_input_byte & bytes_in_buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been reloaded.  It is not necessary to
 * fill the buffer entirely, only to obtain at least one more byte.
 *
 * There is no such thing as an EOF return.  If the end of the file has been
 * reached, the routine has a choice of ERREXIT() or inserting fake data into
 * the buffer.  In most cases, generating a warning message and inserting a
 * fake EOI marker is the best course of action --- this will allow the
 * decompressor to output however much of the image is there.  However,
 * the resulting error message is misleading if the real problem is an empty
 * input file, so we handle that case specially.
 *
 * In applications that need to be able to suspend compression due to input
 * not being available yet, a FALSE return indicates that no more data can be
 * obtained right now, but more may be forthcoming later.  In this situation,
 * the decompressor will return to its caller (with an indication of the
 * number of scanlines it has read, if any).  The application should resume
 * decompression after it has loaded more data into the input buffer.  Note
 * that there are substantial restrictions on the use of suspension --- see
 * the documentation.
 *
 * When suspending, the decompressor will back up to a convenient restart point
 * (typically the start of the current MCU). next_input_byte & bytes_in_buffer
 * indicate where the restart point will be if the current call returns FALSE.
 * Data beyond this point must be rescanned after resumption, so move it to
 * the front of the buffer rather than discarding it.
 */

static boolean
        fill_input_buffer (j_decompress_ptr cinfo)
{
    /* this is only called if the data inside the buffer is not complete. */
    mem_src_ptr src = (mem_src_ptr) cinfo->src;

    if (src->buffer_size < 2) /* Treat empty input file as fatal error */
        ERREXIT(cinfo, JERR_INPUT_EMPTY);
    WARNMS(cinfo, JWRN_JPEG_EOF);
    /* Insert a fake EOI marker */
    src->buffer[0] = (JOCTET) 0xFF;
    src->buffer[1] = (JOCTET) JPEG_EOI;
    src->pub.next_input_byte = src->buffer;
    src->pub.bytes_in_buffer = 2;
    return TRUE;
}


/*
 * Skip data --- used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 *
 * Writers of suspendable-input applications must note that skip_input_data
 * is not granted the right to give a suspension return.  If the skip extends
 * beyond the data currently in the buffer, the buffer can be marked empty so
 * that the next read will cause a fill_input_buffer call that can suspend.
 * Arranging for additional bytes to be discarded before reloading the input
 * buffer is the application writer's problem.
 */

static void
        skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
    mem_src_ptr src = (mem_src_ptr) cinfo->src;

    src->pub.next_input_byte += (size_t) num_bytes;
    src->pub.bytes_in_buffer -= (size_t) num_bytes;
}


/*
 * An additional method that can be provided by data source modules is the
 * resync_to_restart method for error recovery in the presence of RST markers.
 * For the moment, this source module just uses the default resync method
 * provided by the JPEG library.  That method assumes that no backtracking
 * is possible.
 */


/*
 * Terminate source --- called by jpeg_finish_decompress
 * after all data has been read.  Often a no-op.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */

static void
        term_source (j_decompress_ptr cinfo)
{
    /* no work necessary here */
}


/*
 * Prepare for input from a stdio stream.
 * The caller must have already opened the stream, and is responsible
 * for closing it after finishing decompression.
 */

static void
        jpeg_mem_src (j_decompress_ptr cinfo, JOCTET * data, size_t length)
{
    mem_src_ptr src;

  /* The source object and input buffer are made permanent so that a series
    * of JPEG images can be read from the same file by calling jpeg_stdio_src
    * only before the first one.  (If we discarded the buffer at the end of
    * one image, we'd likely lose the start of the next one.)
    * This makes it unsafe to use this manager and a different source
    * manager serially with the same JPEG object.  Caveat programmer.
  */
    if (cinfo->src == NULL) { /* first time for this JPEG object? */
        cinfo->src = (struct jpeg_source_mgr *)
                (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
        sizeof(mem_source_mgr));
        src = (mem_src_ptr) cinfo->src;
        src->buffer = data;
        src->pub.next_input_byte = data; /* until buffer loaded */
        src->buffer_size = length;
        src->pub.bytes_in_buffer = length;
    }

    src = (mem_src_ptr) cinfo->src;
    src->pub.init_source = init_source;
    src->pub.fill_input_buffer = fill_input_buffer;
    src->pub.skip_input_data = skip_input_data;
    src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
    src->pub.term_source = term_source;
}


struct my_error_mgr {
    struct jpeg_error_mgr pub;    /* "public" fields */

    jmp_buf setjmp_buffer;        /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

static void
        my_error_exit (j_common_ptr cinfo)
{
    /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
    my_error_ptr myerr = (my_error_ptr) cinfo->err;

    /* Always display the message. */
    /* We could postpone this until after returning, if we chose. */
    (*cinfo->err->output_message) (cinfo);

    /* Return control to the setjmp point */
    longjmp(myerr->setjmp_buffer, 1);
}




// read jpeg image from memory
bool
decodeJPEG(unsigned char * buffer, size_t buf_len, Image & image, bool rot90)
{
  /* This struct contains the JPEG decompression parameters and pointers to
    * working space (which is allocated as needed by the JPEG library).
  */
    struct jpeg_decompress_struct cinfo;
  /* We use our private extension JPEG error handler.
    * Note that this struct must live as long as the main JPEG parameter
    * struct, to avoid dangling-pointer problems.
  */
    struct my_error_mgr jerr;
    /* More stuff */
    int row_stride;       /* physical row width in output buffer */

    /* Step 1: allocate and initialize JPEG decompression object */

    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    /* Establish the setjmp return context for my_error_exit to use. */
    if (setjmp(jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
        * We need to clean up the JPEG object, close the input file, and return.
    */
        jpeg_destroy_decompress(&cinfo);
        return false;
    }
    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress(&cinfo);

    /* Step 2: specify data source (eg, a file) */
    jpeg_mem_src(&cinfo, buffer, buf_len);

    /* Step 3: read file parameters with jpeg_read_header() */

    (void) jpeg_read_header(&cinfo, TRUE);
  /* We can ignore the return value from jpeg_read_header since
    *   (a) suspension is not possible with the stdio data source, and
    *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
    * See libjpeg.doc for more info.
  */

    /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
    * jpeg_read_header(), so we do nothing here.
  */

    /* Step 5: Start decompressor */

    (void) jpeg_start_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
    * with the stdio data source.
  */

  /* We may need to do some setup of our own at this point before reading
    * the data.  After jpeg_start_decompress() we have the correct scaled
    * output image dimensions available, as well as the output colormap
    * if we asked for color quantization.
    * In this example, we need to make an output work buffer of the right size.
  */ 

    /* resize output image to correct size */
    if (rot90) {
        image.setSize(Size2D(cinfo.output_height, cinfo.output_width));
    } else {
        image.setSize(Size2D(cinfo.output_width, cinfo.output_height));
    }  
    /* JSAMPLEs per row in output buffer */
    row_stride = cinfo.output_width * cinfo.output_components;

    /* Step 6: while (scan lines remain to be read) */
    /*           jpeg_read_scanlines(...); */

  
    if (! rot90) {
        JOCTET * buffer = image.getData();
        /* Here we use the library's state variable cinfo.output_scanline as the
        * loop counter, so that we don't have to keep track ourselves.
        */
        while (cinfo.output_scanline < cinfo.output_height) {
        /* jpeg_read_scanlines expects an array of pointers to scanlines.
            * Here the array is only one element long, but you could ask for
            * more than one scanline at a time if that's more convenient.
        */
            (void) jpeg_read_scanlines(&cinfo, &buffer, 1);
            buffer += row_stride;
        }
    } else {
        // rotate image while reading
        // get pointer to upper right pixel
        JOCTET * buffer = image.getData() + 3*(image.size().w-1);
        // buffer for reading a scanline
        JOCTET * tmpBuffer = (JOCTET*) malloc(row_stride);
        if (tmpBuffer == 0) {
            jpeg_destroy_decompress(&cinfo);
            return false;
        }
        //int x = cinfo.output_height -1;
        while (cinfo.output_scanline < cinfo.output_height) {
            (void) jpeg_read_scanlines(&cinfo, &tmpBuffer, 1);
            JOCTET *srcPtr = tmpBuffer;
            JOCTET *destPtr = buffer;
            for (unsigned y=0; y < cinfo.output_width; y++) {
                destPtr[0] = *srcPtr++;
                destPtr[1] = *srcPtr++;
                destPtr[2] = *srcPtr++;
                destPtr += image.getRowStride();
            }
            // previous column
            buffer -=3;
        }
        // possible memory leak, if jpeg_read_scanlines failes
        free(tmpBuffer);
    }        

    /* Step 7: Finish decompression */

    (void) jpeg_finish_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
    * with the stdio data source.
  */

    /* Step 8: Release JPEG decompression object */

    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_decompress(&cinfo);

  /* At this point you may want to check to see whether any corrupt-data
    * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
  */

    /* And we're done! */
    return true;
}


// read jpeg image from memory
bool
decodeJPEG(FILE * infile, Image & image, bool rot90)
{
  /* This struct contains the JPEG decompression parameters and pointers to
    * working space (which is allocated as needed by the JPEG library).
  */
    struct jpeg_decompress_struct cinfo;
  /* We use our private extension JPEG error handler.
    * Note that this struct must live as long as the main JPEG parameter
    * struct, to avoid dangling-pointer problems.
  */
    struct my_error_mgr jerr;
    /* More stuff */
    int row_stride;       /* physical row width in output buffer */

    /* Step 1: allocate and initialize JPEG decompression object */

    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    /* Establish the setjmp return context for my_error_exit to use. */
    if (setjmp(jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
        * We need to clean up the JPEG object, close the input file, and return.
    */
        jpeg_destroy_decompress(&cinfo);
        return false;
    }
    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress(&cinfo);

    /* Step 2: specify data source (eg, a file) */
    jpeg_stdio_src(&cinfo, infile); 
    
    /* Step 3: read file parameters with jpeg_read_header() */

    (void) jpeg_read_header(&cinfo, TRUE);
  /* We can ignore the return value from jpeg_read_header since
    *   (a) suspension is not possible with the stdio data source, and
    *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
    * See libjpeg.doc for more info.
  */

    /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
    * jpeg_read_header(), so we do nothing here.
  */

    /* Step 5: Start decompressor */

    (void) jpeg_start_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
    * with the stdio data source.
  */

  /* We may need to do some setup of our own at this point before reading
    * the data.  After jpeg_start_decompress() we have the correct scaled
    * output image dimensions available, as well as the output colormap
    * if we asked for color quantization.
    * In this example, we need to make an output work buffer of the right size.
  */ 

    /* resize output image to correct size */
    if (rot90) {
        image.setSize(Size2D(cinfo.output_height, cinfo.output_width));
    } else {
        image.setSize(Size2D(cinfo.output_width, cinfo.output_height));
    }
  
    /* JSAMPLEs per row in output buffer */
    row_stride = cinfo.output_width * cinfo.output_components;

    /* Make a one-row-high sample array that will go away when done with image */

    if (! rot90) {
        JOCTET * buffer = image.getData();
        /* Here we use the library's state variable cinfo.output_scanline as the
         * loop counter, so that we don't have to keep track ourselves.
         */
        while (cinfo.output_scanline < cinfo.output_height) {
        /* jpeg_read_scanlines expects an array of pointers to scanlines.
         * Here the array is only one element long, but you could ask for
         * more than one scanline at a time if that's more convenient.
         */
            (void) jpeg_read_scanlines(&cinfo, &buffer, 1);
            buffer += row_stride;
        }
    } else {
        // rotate image while reading
        // get pointer to upper right pixel
        JOCTET * buffer = image.getData() + 3*(image.size().w-1);
        // buffer for reading a scanline
        JOCTET * tmpBuffer = (JOCTET*) malloc(row_stride);
        if (tmpBuffer == 0) {
            jpeg_destroy_decompress(&cinfo);
            return false;
        }
        //int x = cinfo.output_height -1;
        while (cinfo.output_scanline < cinfo.output_height) {
            (void) jpeg_read_scanlines(&cinfo, &tmpBuffer, 1);
            JOCTET *srcPtr = tmpBuffer;
            JOCTET *destPtr = buffer;
            for (unsigned y=0; y < cinfo.output_width; y++) {
                destPtr[0] = *srcPtr++;
                destPtr[1] = *srcPtr++;
                destPtr[2] = *srcPtr++;
                destPtr += image.getRowStride();
            }
            // previous column
            buffer -=3;
        }
        // possible memory leak, if jpeg_read_scanlines failes
        free(tmpBuffer);
    }        
    /* Step 7: Finish decompression */

    (void) jpeg_finish_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
    * with the stdio data source.
  */

    /* Step 8: Release JPEG decompression object */

    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_decompress(&cinfo);

  /* At this point you may want to check to see whether any corrupt-data
    * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
  */

    /* And we're done! */
    return true;
}


} // namespace
