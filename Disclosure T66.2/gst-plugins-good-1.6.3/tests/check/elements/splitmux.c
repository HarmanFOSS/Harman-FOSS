/* GStreamer unit test for splitmuxsrc/sink elements
 *
 * Copyright (C) 2007 David A. Schleef <ds@schleef.org>
 * Copyright (C) 2015 Jan Schmidt <jan@centricular.com>
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

#include <glib/gstdio.h>
#include <unistd.h>

#include <gst/check/gstcheck.h>
#include <stdlib.h>
#include <unistd.h>

gchar *tmpdir = NULL;

static void
tempdir_setup (void)
{
  const gchar *systmp = g_get_tmp_dir ();
  tmpdir = g_build_filename (systmp, "splitmux-test-XXXXXX", NULL);
  /* Rewrites tmpdir template input: */
  tmpdir = g_mkdtemp (tmpdir);
}

static void
tempdir_cleanup (void)
{
  GDir *d;
  const gchar *f;

  fail_if (tmpdir == NULL);

  d = g_dir_open (tmpdir, 0, NULL);
  fail_if (d == NULL);

  while ((f = g_dir_read_name (d)) != NULL) {
    gchar *fname = g_build_filename (tmpdir, f, NULL);
    fail_if (g_remove (fname) != 0, "Failed to remove tmp file %s", fname);
    g_free (fname);
  }
  g_dir_close (d);

  fail_if (g_remove (tmpdir) != 0, "Failed to delete tmpdir %s", tmpdir);

  g_free (tmpdir);
  tmpdir = NULL;
}

static guint
count_files (const gchar * target)
{
  GDir *d;
  const gchar *f;
  guint ret = 0;

  d = g_dir_open (target, 0, NULL);
  fail_if (d == NULL);

  while ((f = g_dir_read_name (d)) != NULL)
    ret++;
  g_dir_close (d);

  return ret;
}


static GstMessage *
run_pipeline (GstElement * pipeline)
{
  GstBus *bus = gst_element_get_bus (GST_ELEMENT (pipeline));
  GstMessage *msg;

  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  msg = gst_bus_poll (bus, GST_MESSAGE_EOS | GST_MESSAGE_ERROR, -1);
  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (bus);

  return msg;
}

static void
dump_error (GstMessage * msg)
{
  GError *err = NULL;
  gchar *dbg_info;

  fail_unless (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR);

  gst_message_parse_error (msg, &err, &dbg_info);

  g_printerr ("ERROR from element %s: %s\n",
      GST_OBJECT_NAME (msg->src), err->message);
  g_printerr ("Debugging info: %s\n", (dbg_info) ? dbg_info : "none");
  g_error_free (err);
  g_free (dbg_info);
}

static void
test_playback (const gchar * in_pattern)
{
  GstMessage *msg;
  GstElement *pipeline;
  GstElement *fakesink;
  gchar *uri;

  pipeline = gst_element_factory_make ("playbin", NULL);
  fail_if (pipeline == NULL);

  fakesink = gst_element_factory_make ("fakesink", NULL);
  fail_if (fakesink == NULL);
  g_object_set (G_OBJECT (pipeline), "video-sink", fakesink, NULL);

  uri = g_strdup_printf ("splitmux://%s", in_pattern);

  g_object_set (G_OBJECT (pipeline), "uri", uri, NULL);
  g_free (uri);

  msg = run_pipeline (pipeline);

  if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR)
    dump_error (msg);
  fail_unless (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_EOS);
  gst_message_unref (msg);
  gst_object_unref (pipeline);
}

GST_START_TEST (test_splitmuxsrc)
{
  gchar *in_pattern =
      g_build_filename (GST_TEST_FILES_PATH, "splitvideo*.ogg", NULL);
  test_playback (in_pattern);
  g_free (in_pattern);
}

GST_END_TEST;

GST_START_TEST (test_splitmuxsink)
{
  GstMessage *msg;
  GstElement *pipeline;
  GstElement *sink;
  GstPad *splitmux_sink_pad;
  GstPad *enc_src_pad;
  gchar *dest_pattern;
  guint count;
  gchar *in_pattern;

  /* This pipeline has a small time cutoff - it should start a new file
   * every GOP, ie 1 second */
  pipeline =
      gst_parse_launch
      ("videotestsrc num-buffers=15 ! video/x-raw,width=80,height=64,framerate=5/1 ! videoconvert !"
      " queue ! theoraenc keyframe-force=5 ! splitmuxsink name=splitsink "
      " max-size-time=1000000 max-size-bytes=1000000 muxer=oggmux", NULL);
  fail_if (pipeline == NULL);
  sink = gst_bin_get_by_name (GST_BIN (pipeline), "splitsink");
  fail_if (sink == NULL);
  dest_pattern = g_build_filename (tmpdir, "out%05d.ogg", NULL);
  g_object_set (G_OBJECT (sink), "location", dest_pattern, NULL);
  g_free (dest_pattern);
  g_object_unref (sink);

  msg = run_pipeline (pipeline);

  if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR)
    dump_error (msg);
  fail_unless (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_EOS);
  gst_message_unref (msg);

  /* unlink manually and relase request pad to ensure that we *can* do that
   * - https://bugzilla.gnome.org/show_bug.cgi?id=753622 */
  sink = gst_bin_get_by_name (GST_BIN (pipeline), "splitsink");
  fail_if (sink == NULL);
  splitmux_sink_pad = gst_element_get_static_pad (sink, "video");
  fail_if (splitmux_sink_pad == NULL);
  enc_src_pad = gst_pad_get_peer (splitmux_sink_pad);
  fail_if (enc_src_pad == NULL);
  fail_unless (gst_pad_unlink (enc_src_pad, splitmux_sink_pad));
  gst_object_unref (enc_src_pad);
  gst_element_release_request_pad (sink, splitmux_sink_pad);
  gst_object_unref (splitmux_sink_pad);
  /* at this point the pad must be releaased - try to find it again to verify */
  splitmux_sink_pad = gst_element_get_static_pad (sink, "video");
  fail_if (splitmux_sink_pad != NULL);
  g_object_unref (sink);

  gst_object_unref (pipeline);

  count = count_files (tmpdir);
  fail_unless (count == 3, "Expected 3 output files, got %d", count);

  in_pattern = g_build_filename (tmpdir, "out*.ogg", NULL);
  test_playback (in_pattern);
  g_free (in_pattern);
}

GST_END_TEST;

static Suite *
splitmux_suite (void)
{
  Suite *s = suite_create ("splitmux");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);

  tcase_add_checked_fixture (tc_chain, tempdir_setup, tempdir_cleanup);

  tcase_add_test (tc_chain, test_splitmuxsrc);
  tcase_add_test (tc_chain, test_splitmuxsink);

  return s;
}

GST_CHECK_MAIN (splitmux);
