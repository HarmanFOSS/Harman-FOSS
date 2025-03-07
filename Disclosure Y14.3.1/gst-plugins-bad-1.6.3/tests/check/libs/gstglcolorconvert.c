/* GStreamer
 *
 * Copyright (C) 2014 Matthew Waters <matthew@centricular.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/check/gstcheck.h>

#include <gst/gl/gl.h>

#include <stdio.h>

static GstGLDisplay *display;
static GstGLContext *context;
static GstGLWindow *window;
static GstGLColorConvert *convert;

typedef struct _TestFrame
{
  gint width;
  gint height;
  GstVideoFormat v_format;
  gchar *data[GST_VIDEO_MAX_PLANES];
} TestFrame;

#define IGNORE_MAGIC 0x05

static const gchar rgba_reorder_data[] = { 0x49, 0x24, 0x72, 0xff };
static const gchar rgbx_reorder_data[] = { 0x49, 0x24, 0x72, IGNORE_MAGIC };
static const gchar argb_reorder_data[] = { 0xff, 0x49, 0x24, 0x72 };
static const gchar xrgb_reorder_data[] = { IGNORE_MAGIC, 0x49, 0x24, 0x72 };
static const gchar rgb_reorder_data[] = { 0x49, 0x24, 0x72, IGNORE_MAGIC };
static const gchar bgr_reorder_data[] = { 0x72, 0x24, 0x49, IGNORE_MAGIC };
static const gchar bgra_reorder_data[] = { 0x72, 0x24, 0x49, 0xff };
static const gchar bgrx_reorder_data[] = { 0x72, 0x24, 0x49, IGNORE_MAGIC };
static const gchar abgr_reorder_data[] = { 0xff, 0x72, 0x24, 0x49 };
static const gchar xbgr_reorder_data[] = { IGNORE_MAGIC, 0x72, 0x24, 0x49 };

static TestFrame test_rgba_reorder[] = {
  {1, 1, GST_VIDEO_FORMAT_RGBA, {(gchar *) & rgba_reorder_data}},
  {1, 1, GST_VIDEO_FORMAT_RGBx, {(gchar *) & rgbx_reorder_data}},
  {1, 1, GST_VIDEO_FORMAT_ARGB, {(gchar *) & argb_reorder_data}},
  {1, 1, GST_VIDEO_FORMAT_xRGB, {(gchar *) & xrgb_reorder_data}},
  {1, 1, GST_VIDEO_FORMAT_BGRA, {(gchar *) & bgra_reorder_data}},
  {1, 1, GST_VIDEO_FORMAT_BGRx, {(gchar *) & bgrx_reorder_data}},
  {1, 1, GST_VIDEO_FORMAT_ABGR, {(gchar *) & abgr_reorder_data}},
  {1, 1, GST_VIDEO_FORMAT_xBGR, {(gchar *) & xbgr_reorder_data}},
  {1, 1, GST_VIDEO_FORMAT_RGB, {(gchar *) & rgb_reorder_data}},
  {1, 1, GST_VIDEO_FORMAT_BGR, {(gchar *) & bgr_reorder_data}},
};

static void
setup (void)
{
  GError *error = NULL;

  display = gst_gl_display_new ();
  context = gst_gl_context_new (display);

  gst_gl_context_create (context, 0, &error);
  window = gst_gl_context_get_window (context);

  fail_if (error != NULL, "Error creating context: %s\n",
      error ? error->message : "Unknown Error");

  convert = gst_gl_color_convert_new (context);
}

static void
teardown (void)
{
  GLuint error = context->gl_vtable->GetError ();
  fail_if (error != GL_NONE, "GL error 0x%x encountered during processing\n",
      error);

  gst_object_unref (convert);
  gst_object_unref (window);
  gst_object_unref (context);
  gst_object_unref (display);
}

static gsize
_video_info_plane_size (GstVideoInfo * info, guint plane)
{
  if (GST_VIDEO_INFO_N_PLANES (info) == plane + 1)
    return info->offset[0] + info->size - info->offset[plane];

  return info->offset[plane + 1] - info->offset[plane];
}

static void
_frame_unref (gpointer user_data)
{
  gint *ref_count = user_data;
  g_atomic_int_add (ref_count, -1);
}

static void
check_conversion (TestFrame * frames, guint size)
{
  gint i, j, k, l;
  gint ref_count = 0;

  for (i = 0; i < size; i++) {
    GstBuffer *inbuf;
    GstVideoInfo in_info;
    gint in_width = frames[i].width;
    gint in_height = frames[i].height;
    GstVideoFormat in_v_format = frames[i].v_format;
    gchar *in_data[GST_VIDEO_MAX_PLANES] = { 0 };
    GstGLMemory *in_mem[GST_VIDEO_MAX_PLANES] = { 0 };
    GstVideoFrame in_frame;
    GstCaps *in_caps;

    gst_video_info_set_format (&in_info, in_v_format, in_width, in_height);
    in_caps = gst_video_info_to_caps (&in_info);
    gst_caps_set_features (in_caps, 0,
        gst_caps_features_from_string (GST_CAPS_FEATURE_MEMORY_GL_MEMORY));

    for (j = 0; j < GST_VIDEO_INFO_N_PLANES (&in_info); j++) {
      in_data[j] = frames[i].data[j];
    }

    /* create GL buffer */
    ref_count += GST_VIDEO_INFO_N_PLANES (&in_info);
    inbuf = gst_buffer_new ();
    fail_unless (gst_gl_memory_setup_wrapped (context, &in_info, NULL,
            (gpointer *) in_data, in_mem, &ref_count, _frame_unref));

    for (j = 0; j < GST_VIDEO_INFO_N_PLANES (&in_info); j++) {
      gst_buffer_append_memory (inbuf, (GstMemory *) in_mem[j]);
    }

    fail_unless (gst_video_frame_map (&in_frame, &in_info, inbuf,
            GST_MAP_READ));

    /* sanity check that the correct values were wrapped */
    for (j = 0; j < GST_VIDEO_INFO_N_PLANES (&in_info); j++) {
      for (k = 0; k < _video_info_plane_size (&in_info, j); k++) {
        if (in_data[j][k] != IGNORE_MAGIC)
          fail_unless (((gchar *) in_frame.data[j])[k] == in_data[j][k]);
      }
    }

    for (j = 0; j < size; j++) {
      GstBuffer *outbuf;
      GstVideoInfo out_info;
      gint out_width = frames[j].width;
      gint out_height = frames[j].height;
      GstVideoFormat out_v_format = frames[j].v_format;
      gchar *out_data[GST_VIDEO_MAX_PLANES] = { 0 };
      GstVideoFrame out_frame;
      GstCaps *out_caps;

      gst_video_info_set_format (&out_info, out_v_format, out_width,
          out_height);
      out_caps = gst_video_info_to_caps (&out_info);
      gst_caps_set_features (out_caps, 0,
          gst_caps_features_from_string (GST_CAPS_FEATURE_MEMORY_GL_MEMORY));

      for (k = 0; k < GST_VIDEO_INFO_N_PLANES (&out_info); k++) {
        out_data[k] = frames[j].data[k];
      }

      gst_gl_color_convert_set_caps (convert, in_caps, out_caps);

      /* convert the data */
      outbuf = gst_gl_color_convert_perform (convert, inbuf);
      if (outbuf == NULL) {
        const gchar *in_str = gst_video_format_to_string (in_v_format);
        const gchar *out_str = gst_video_format_to_string (out_v_format);
        GST_WARNING ("failed to convert from %s to %s", in_str, out_str);
      }

      fail_unless (gst_video_frame_map (&out_frame, &out_info, outbuf,
              GST_MAP_READ));

      /* check that the converted values are correct */
      for (k = 0; k < GST_VIDEO_INFO_N_PLANES (&out_info); k++) {
        for (l = 0; l < _video_info_plane_size (&out_info, k); l++) {
          gchar out_pixel = ((gchar *) out_frame.data[k])[l];
          if (out_data[k][l] != IGNORE_MAGIC && out_pixel != IGNORE_MAGIC)
            fail_unless (out_pixel == out_data[k][l]);
          /* FIXME: check alpha clobbering */
        }
      }

      gst_caps_unref (out_caps);
      gst_video_frame_unmap (&out_frame);
      gst_buffer_unref (outbuf);
    }

    gst_caps_unref (in_caps);
    gst_video_frame_unmap (&in_frame);
    gst_buffer_unref (inbuf);

    fail_unless_equals_int (ref_count, 0);
  }
}

GST_START_TEST (test_reorder_buffer)
{
  guint size = G_N_ELEMENTS (test_rgba_reorder);

  /* gles can't download rgb24 textures */
  if (gst_gl_context_get_gl_api (context) & GST_GL_API_GLES2)
    size -= 2;

  check_conversion (test_rgba_reorder, size);
}

GST_END_TEST;

static Suite *
gst_gl_color_convert_suite (void)
{
  Suite *s = suite_create ("GstGLUpload");
  TCase *tc_chain = tcase_create ("upload");

  suite_add_tcase (s, tc_chain);
  tcase_add_checked_fixture (tc_chain, setup, teardown);
  tcase_add_test (tc_chain, test_reorder_buffer);
  /* FIXME add YUV <--> RGB conversion tests */

  return s;
}

GST_CHECK_MAIN (gst_gl_color_convert);
