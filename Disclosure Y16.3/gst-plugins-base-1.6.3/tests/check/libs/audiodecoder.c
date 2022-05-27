/* GStreamer
 *
 * Copyright (C) 2014 Samsung Electronics. All rights reserved.
 *   Author: Thiago Santos <ts.santos@sisa.samsung.com>
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
#include "config.h"
#endif
#include <gst/gst.h>
#include <gst/check/gstcheck.h>
#include <gst/audio/audio.h>
#include <gst/app/app.h>

static GstPad *mysrcpad, *mysinkpad;
static GstElement *dec;
static GList *events = NULL;

#define TEST_MSECS_PER_SAMPLE 44100

#define RESTRICTED_CAPS_RATE 44100
#define RESTRICTED_CAPS_CHANNELS 6
static GstStaticPadTemplate sinktemplate_restricted =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, rate=(int)44100, channels=(int)6")
    );

static GstStaticPadTemplate sinktemplate_with_range =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, rate=(int)[1,44100], channels=(int)[1,6]")
    );

static GstStaticPadTemplate sinktemplate_default =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, format=(string)S32LE, "
        "rate=(int)[1, 320000], channels=(int)[1, 32],"
        "layout=(string)interleaved")
    );
static GstStaticPadTemplate srctemplate_default =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-test-custom")
    );

#define GST_AUDIO_DECODER_TESTER_TYPE gst_audio_decoder_tester_get_type()
static GType gst_audio_decoder_tester_get_type (void);

typedef struct _GstAudioDecoderTester GstAudioDecoderTester;
typedef struct _GstAudioDecoderTesterClass GstAudioDecoderTesterClass;

struct _GstAudioDecoderTester
{
  GstAudioDecoder parent;

  gboolean setoutputformat_on_decoding;
  gboolean output_too_many_frames;
};

struct _GstAudioDecoderTesterClass
{
  GstAudioDecoderClass parent_class;
};

G_DEFINE_TYPE (GstAudioDecoderTester, gst_audio_decoder_tester,
    GST_TYPE_AUDIO_DECODER);

static gboolean
gst_audio_decoder_tester_start (GstAudioDecoder * dec)
{
  return TRUE;
}

static gboolean
gst_audio_decoder_tester_stop (GstAudioDecoder * dec)
{
  return TRUE;
}

static void
gst_audio_decoder_tester_flush (GstAudioDecoder * dec, gboolean hard)
{
}

static gboolean
gst_audio_decoder_tester_set_format (GstAudioDecoder * dec, GstCaps * caps)
{
  GstAudioDecoderTester *tester = (GstAudioDecoderTester *) dec;
  GstAudioInfo info;

  if (!tester->setoutputformat_on_decoding) {
    caps = gst_caps_new_simple ("audio/x-raw", "format", G_TYPE_STRING, "S32LE",
        "channels", G_TYPE_INT, 2, "rate", G_TYPE_INT, 44100,
        "layout", G_TYPE_STRING, "interleaved", NULL);
    gst_audio_info_from_caps (&info, caps);
    gst_caps_unref (caps);

    gst_audio_decoder_set_output_format (dec, &info);
  }
  return TRUE;
}

static GstFlowReturn
gst_audio_decoder_tester_handle_frame (GstAudioDecoder * dec,
    GstBuffer * buffer)
{
  GstAudioDecoderTester *tester = (GstAudioDecoderTester *) dec;
  guint8 *data;
  gint size;
  GstMapInfo map;
  GstBuffer *output_buffer;

  if (buffer == NULL)
    return GST_FLOW_OK;

  if (tester->setoutputformat_on_decoding) {
    GstCaps *caps;
    GstAudioInfo info;

    caps = gst_caps_new_simple ("audio/x-raw", "format", G_TYPE_STRING, "S32LE",
        "channels", G_TYPE_INT, 2, "rate", G_TYPE_INT, 44100,
        "layout", G_TYPE_STRING, "interleaved", NULL);
    gst_audio_info_from_caps (&info, caps);
    gst_caps_unref (caps);

    gst_audio_decoder_set_output_format (dec, &info);
  }

  gst_buffer_map (buffer, &map, GST_MAP_READ);

  /* the output is SE32LE stereo 44100 Hz */
  size = 2 * 4;
  g_assert (size == sizeof (guint64));
  data = g_malloc0 (size);

  memcpy (data, map.data, sizeof (guint64));

  output_buffer = gst_buffer_new_wrapped (data, size);

  gst_buffer_unmap (buffer, &map);

  if (tester->output_too_many_frames) {
    return gst_audio_decoder_finish_frame (dec, output_buffer, 2);
  } else {
    return gst_audio_decoder_finish_frame (dec, output_buffer, 1);
  }
}

static void
gst_audio_decoder_tester_class_init (GstAudioDecoderTesterClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstAudioDecoderClass *audiosink_class = GST_AUDIO_DECODER_CLASS (klass);

  static GstStaticPadTemplate sink_templ = GST_STATIC_PAD_TEMPLATE ("sink",
      GST_PAD_SINK, GST_PAD_ALWAYS,
      GST_STATIC_CAPS ("audio/x-test-custom"));

  static GstStaticPadTemplate src_templ = GST_STATIC_PAD_TEMPLATE ("src",
      GST_PAD_SRC, GST_PAD_ALWAYS,
      GST_STATIC_CAPS ("audio/x-raw"));

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_templ));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_templ));

  gst_element_class_set_metadata (element_class,
      "AudioDecoderTester", "Decoder/Audio", "yep", "me");

  audiosink_class->start = gst_audio_decoder_tester_start;
  audiosink_class->stop = gst_audio_decoder_tester_stop;
  audiosink_class->flush = gst_audio_decoder_tester_flush;
  audiosink_class->handle_frame = gst_audio_decoder_tester_handle_frame;
  audiosink_class->set_format = gst_audio_decoder_tester_set_format;
}

static void
gst_audio_decoder_tester_init (GstAudioDecoderTester * tester)
{
}

static gboolean
_mysinkpad_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  events = g_list_append (events, event);
  return TRUE;
}

static void
setup_audiodecodertester (GstStaticPadTemplate * sinktemplate,
    GstStaticPadTemplate * srctemplate)
{
  if (sinktemplate == NULL)
    sinktemplate = &sinktemplate_default;
  if (srctemplate == NULL)
    srctemplate = &srctemplate_default;

  dec = g_object_new (GST_AUDIO_DECODER_TESTER_TYPE, NULL);
  mysrcpad = gst_check_setup_src_pad (dec, srctemplate);
  mysinkpad = gst_check_setup_sink_pad (dec, sinktemplate);

  gst_pad_set_event_function (mysinkpad, _mysinkpad_event);
}

static void
cleanup_audiodecodertest (void)
{
  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_src_pad (dec);
  gst_check_teardown_sink_pad (dec);
  gst_check_teardown_element (dec);
}

static GstBuffer *
create_test_buffer (guint64 num)
{
  GstBuffer *buffer;
  guint64 *data = g_malloc (sizeof (guint64));

  *data = num;

  buffer = gst_buffer_new_wrapped (data, sizeof (guint64));

  GST_BUFFER_PTS (buffer) =
      gst_util_uint64_scale_round (num, GST_SECOND, TEST_MSECS_PER_SAMPLE);
  GST_BUFFER_DURATION (buffer) =
      gst_util_uint64_scale_round (1, GST_SECOND, TEST_MSECS_PER_SAMPLE);

  return buffer;
}

static void
send_startup_events (void)
{
  GstCaps *caps;

  fail_unless (gst_pad_push_event (mysrcpad,
          gst_event_new_stream_start ("randomvalue")));

  /* push caps */
  caps =
      gst_caps_new_simple ("audio/x-test-custom", "channels", G_TYPE_INT, 2,
      "rate", G_TYPE_INT, 44100, NULL);
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_caps (caps)));
  gst_caps_unref (caps);
}

#define NUM_BUFFERS 1000
GST_START_TEST (audiodecoder_playback)
{
  GstSegment segment;
  GstBuffer *buffer;
  guint64 i;

  setup_audiodecodertester (NULL, NULL);

  gst_pad_set_active (mysrcpad, TRUE);
  gst_element_set_state (dec, GST_STATE_PLAYING);
  gst_pad_set_active (mysinkpad, TRUE);

  send_startup_events ();

  /* push a new segment */
  gst_segment_init (&segment, GST_FORMAT_TIME);
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_segment (&segment)));

  /* push buffers, the data is actually a number so we can track them */
  for (i = 0; i < NUM_BUFFERS; i++) {
    GstMapInfo map;
    guint64 num;

    buffer = create_test_buffer (i);

    fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);

    /* check that buffer was received by our source pad */
    buffer = buffers->data;

    gst_buffer_map (buffer, &map, GST_MAP_READ);

    num = *(guint64 *) map.data;
    fail_unless_equals_uint64 (i, num);
    fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer),
        gst_util_uint64_scale_round (i, GST_SECOND, TEST_MSECS_PER_SAMPLE));
    fail_unless_equals_uint64 (GST_BUFFER_DURATION (buffer),
        gst_util_uint64_scale_round (1, GST_SECOND, TEST_MSECS_PER_SAMPLE));

    gst_buffer_unmap (buffer, &map);

    gst_buffer_unref (buffer);
    buffers = g_list_delete_link (buffers, buffers);
  }

  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_eos ()));

  fail_unless (buffers == NULL);

  cleanup_audiodecodertest ();
}

GST_END_TEST;

static void
check_audiodecoder_negotiation (void)
{
  gboolean received_caps = FALSE;
  GList *iter;

  for (iter = events; iter; iter = g_list_next (iter)) {
    GstEvent *event = iter->data;

    if (GST_EVENT_TYPE (event) == GST_EVENT_CAPS) {
      GstCaps *caps;
      GstStructure *structure;
      gint channels;
      gint rate;

      gst_event_parse_caps (event, &caps);
      structure = gst_caps_get_structure (caps, 0);

      fail_unless (gst_structure_get_int (structure, "rate", &rate));
      fail_unless (gst_structure_get_int (structure, "channels", &channels));

      fail_unless (rate == 44100, "%d != %d", rate, 44100);
      fail_unless (channels == 2, "%d != %d", channels, 2);

      received_caps = TRUE;
      break;
    }
  }
  fail_unless (received_caps);
}

GST_START_TEST (audiodecoder_negotiation_with_buffer)
{
  GstSegment segment;
  GstBuffer *buffer;

  setup_audiodecodertester (NULL, NULL);

  gst_pad_set_active (mysrcpad, TRUE);
  gst_element_set_state (dec, GST_STATE_PLAYING);
  gst_pad_set_active (mysinkpad, TRUE);

  send_startup_events ();

  /* push a new segment */
  gst_segment_init (&segment, GST_FORMAT_TIME);
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_segment (&segment)));

  /* push a buffer event to force audiodecoder to push a caps event */
  buffer = create_test_buffer (0);
  fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);

  check_audiodecoder_negotiation ();

  cleanup_audiodecodertest ();
  g_list_free_full (buffers, (GDestroyNotify) gst_buffer_unref);
}

GST_END_TEST;


GST_START_TEST (audiodecoder_negotiation_with_gap_event)
{
  GstSegment segment;

  setup_audiodecodertester (NULL, NULL);

  gst_pad_set_active (mysrcpad, TRUE);
  gst_element_set_state (dec, GST_STATE_PLAYING);
  gst_pad_set_active (mysinkpad, TRUE);

  send_startup_events ();

  /* push a new segment */
  gst_segment_init (&segment, GST_FORMAT_TIME);
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_segment (&segment)));

  /* push a gap event to force audiodecoder to push a caps event */
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_gap (0,
              GST_SECOND)));
  fail_unless (buffers == NULL);

  check_audiodecoder_negotiation ();

  cleanup_audiodecodertest ();
}

GST_END_TEST;


GST_START_TEST (audiodecoder_delayed_negotiation_with_gap_event)
{
  GstSegment segment;

  setup_audiodecodertester (NULL, NULL);

  ((GstAudioDecoderTester *) dec)->setoutputformat_on_decoding = TRUE;

  gst_pad_set_active (mysrcpad, TRUE);
  gst_element_set_state (dec, GST_STATE_PLAYING);
  gst_pad_set_active (mysinkpad, TRUE);

  send_startup_events ();

  /* push a new segment */
  gst_segment_init (&segment, GST_FORMAT_TIME);
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_segment (&segment)));

  /* push a gap event to force audiodecoder to push a caps event */
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_gap (0,
              GST_SECOND)));
  fail_unless (buffers == NULL);

  check_audiodecoder_negotiation ();

  cleanup_audiodecodertest ();
}

GST_END_TEST;


/* make sure that the segment event is pushed before the gap */
GST_START_TEST (audiodecoder_first_data_is_gap)
{
  GstSegment segment;
  GList *events_iter;

  setup_audiodecodertester (NULL, NULL);

  gst_pad_set_active (mysrcpad, TRUE);
  gst_element_set_state (dec, GST_STATE_PLAYING);
  gst_pad_set_active (mysinkpad, TRUE);

  send_startup_events ();

  /* push a new segment */
  gst_segment_init (&segment, GST_FORMAT_TIME);
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_segment (&segment)));

  /* push a gap */
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_gap (0,
              GST_SECOND)));
  events_iter = events;
  /* make sure the usual events have been received */
  {
    GstEvent *sstart = events_iter->data;
    fail_unless (GST_EVENT_TYPE (sstart) == GST_EVENT_STREAM_START);
    events_iter = g_list_next (events_iter);
  }
  {
    GstEvent *caps_event = events_iter->data;
    fail_unless (GST_EVENT_TYPE (caps_event) == GST_EVENT_CAPS);
    events_iter = g_list_next (events_iter);
  }
  {
    GstEvent *segment_event = events_iter->data;
    fail_unless (GST_EVENT_TYPE (segment_event) == GST_EVENT_SEGMENT);
    events_iter = g_list_next (events_iter);
  }

  /* Make sure the gap was pushed */
  {
    GstEvent *gap = events_iter->data;
    fail_unless (GST_EVENT_TYPE (gap) == GST_EVENT_GAP);
    events_iter = g_list_next (events_iter);
  }
  fail_unless (events_iter == NULL);

  cleanup_audiodecodertest ();
}

GST_END_TEST;


static void
_audiodecoder_flush_events (gboolean send_buffers)
{
  GstSegment segment;
  GstBuffer *buffer;
  guint i;
  GList *events_iter;
  GstMessage *msg;

  setup_audiodecodertester (NULL, NULL);

  gst_pad_set_active (mysrcpad, TRUE);
  gst_element_set_state (dec, GST_STATE_PLAYING);
  gst_pad_set_active (mysinkpad, TRUE);

  send_startup_events ();

  /* push a new segment */
  gst_segment_init (&segment, GST_FORMAT_TIME);
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_segment (&segment)));

  if (send_buffers) {
    /* push buffers, the data is actually a number so we can track them */
    for (i = 0; i < NUM_BUFFERS; i++) {
      if (i % 10 == 0) {
        GstTagList *tags;

        tags = gst_tag_list_new (GST_TAG_TRACK_NUMBER, i, NULL);
        fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_tag (tags)));
      } else {
        buffer = create_test_buffer (i);

        fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);
      }
    }
  } else {
    /* push sticky event */
    GstTagList *tags;
    tags = gst_tag_list_new (GST_TAG_TRACK_NUMBER, 0, NULL);
    fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_tag (tags)));
  }

  msg =
      gst_message_new_element (GST_OBJECT (mysrcpad),
      gst_structure_new_empty ("test"));
  fail_unless (gst_pad_push_event (mysrcpad,
          gst_event_new_sink_message ("test", msg)));
  gst_message_unref (msg);

  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_eos ()));

  events_iter = events;
  /* make sure the usual events have been received */
  {
    GstEvent *sstart = events_iter->data;
    fail_unless (GST_EVENT_TYPE (sstart) == GST_EVENT_STREAM_START);
    events_iter = g_list_next (events_iter);
  }
  if (send_buffers) {
    {
      GstEvent *caps_event = events_iter->data;
      fail_unless (GST_EVENT_TYPE (caps_event) == GST_EVENT_CAPS);
      events_iter = g_list_next (events_iter);
    }
    {
      GstEvent *segment_event = events_iter->data;
      fail_unless (GST_EVENT_TYPE (segment_event) == GST_EVENT_SEGMENT);
      events_iter = g_list_next (events_iter);
    }
    for (int i = 0; i < NUM_BUFFERS / 10; i++) {
      GstEvent *tag_event = events_iter->data;
      fail_unless (GST_EVENT_TYPE (tag_event) == GST_EVENT_TAG);
      events_iter = g_list_next (events_iter);
    }
  }
  {
    GstEvent *eos_event = g_list_last (events_iter)->data;

    fail_unless (GST_EVENT_TYPE (eos_event) == GST_EVENT_EOS);
    events_iter = g_list_next (events_iter);
  }

  /* check that EOS was received */
  fail_unless (GST_PAD_IS_EOS (mysrcpad));
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_flush_start ()));
  fail_unless (GST_PAD_IS_EOS (mysrcpad));

  /* Check that we have tags */
  {
    GstEvent *tags = gst_pad_get_sticky_event (mysrcpad, GST_EVENT_TAG, 0);

    fail_unless (tags != NULL);
    gst_event_unref (tags);
  }

  /* Check that we still have a segment set */
  {
    GstEvent *segment =
        gst_pad_get_sticky_event (mysrcpad, GST_EVENT_SEGMENT, 0);

    fail_unless (segment != NULL);
    gst_event_unref (segment);
  }

  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_flush_stop (TRUE)));
  fail_if (GST_PAD_IS_EOS (mysrcpad));

  /* Check that the segment was flushed on FLUSH_STOP */
  {
    GstEvent *segment =
        gst_pad_get_sticky_event (mysrcpad, GST_EVENT_SEGMENT, 0);

    fail_unless (segment == NULL);
  }

  /* Check the tags were not lost on FLUSH_STOP */
  {
    GstEvent *tags = gst_pad_get_sticky_event (mysrcpad, GST_EVENT_TAG, 0);

    fail_unless (tags != NULL);
    gst_event_unref (tags);

  }

  g_list_free_full (events, (GDestroyNotify) gst_event_unref);
  events = NULL;

  g_list_free_full (buffers, (GDestroyNotify) gst_buffer_unref);
  buffers = NULL;

  gst_element_set_state (dec, GST_STATE_NULL);
  cleanup_audiodecodertest ();
}

/* An element should always push its segment before sending EOS */
GST_START_TEST (audiodecoder_eos_events_no_buffers)
{
  GstSegment segment;
  setup_audiodecodertester (NULL, NULL);

  gst_pad_set_active (mysrcpad, TRUE);
  gst_element_set_state (dec, GST_STATE_PLAYING);
  gst_pad_set_active (mysinkpad, TRUE);
  send_startup_events ();

  gst_segment_init (&segment, GST_FORMAT_TIME);
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_segment (&segment)));
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_eos ()));

  fail_unless (GST_PAD_IS_EOS (mysinkpad));

  {
    GstEvent *segment_event =
        gst_pad_get_sticky_event (mysinkpad, GST_EVENT_SEGMENT, 0);
    fail_unless (segment_event != NULL);
    gst_event_unref (segment_event);
  }

  gst_element_set_state (dec, GST_STATE_NULL);
  cleanup_audiodecodertest ();
}

GST_END_TEST;

GST_START_TEST (audiodecoder_flush_events_no_buffers)
{
  _audiodecoder_flush_events (FALSE);
}

GST_END_TEST;

GST_START_TEST (audiodecoder_flush_events)
{
  _audiodecoder_flush_events (TRUE);
}

GST_END_TEST;


GST_START_TEST (audiodecoder_buffer_after_segment)
{
  GstSegment segment;
  GstBuffer *buffer;
  guint64 i;
  GstClockTime pos;

  setup_audiodecodertester (NULL, NULL);

  gst_pad_set_active (mysrcpad, TRUE);
  gst_element_set_state (dec, GST_STATE_PLAYING);
  gst_pad_set_active (mysinkpad, TRUE);

  send_startup_events ();

  /* push a new segment */
  gst_segment_init (&segment, GST_FORMAT_TIME);
  segment.stop = GST_SECOND;
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_segment (&segment)));

  /* push buffers, the data is actually a number so we can track them */
  i = 0;
  pos = 0;
  while (pos < GST_SECOND) {
    GstMapInfo map;
    guint64 num;

    buffer = create_test_buffer (i);
    pos = GST_BUFFER_TIMESTAMP (buffer) + GST_BUFFER_DURATION (buffer);

    fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);

    /* check that buffer was received by our source pad */
    buffer = buffers->data;

    gst_buffer_map (buffer, &map, GST_MAP_READ);

    num = *(guint64 *) map.data;
    fail_unless_equals_uint64 (i, num);
    fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer),
        gst_util_uint64_scale_round (i, GST_SECOND, TEST_MSECS_PER_SAMPLE));
    fail_unless_equals_uint64 (GST_BUFFER_DURATION (buffer),
        gst_util_uint64_scale_round (1, GST_SECOND, TEST_MSECS_PER_SAMPLE));

    gst_buffer_unmap (buffer, &map);

    gst_buffer_unref (buffer);
    buffers = g_list_delete_link (buffers, buffers);
    i++;
  }

  /* this buffer is after the segment */
  buffer = create_test_buffer (i++);
  fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_EOS);

  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_eos ()));

  fail_unless (buffers == NULL);

  cleanup_audiodecodertest ();
}

GST_END_TEST;

GST_START_TEST (audiodecoder_output_too_many_frames)
{
  GstSegment segment;
  GstBuffer *buffer;
  guint64 i;

  setup_audiodecodertester (NULL, NULL);

  ((GstAudioDecoderTester *) dec)->output_too_many_frames = TRUE;

  gst_pad_set_active (mysrcpad, TRUE);
  gst_element_set_state (dec, GST_STATE_PLAYING);
  gst_pad_set_active (mysinkpad, TRUE);

  send_startup_events ();

  /* push a new segment */
  gst_segment_init (&segment, GST_FORMAT_TIME);
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_segment (&segment)));

  /* push buffers, the data is actually a number so we can track them */
  for (i = 0; i < 3; i++) {
    GstMapInfo map;
    guint64 num;

    buffer = create_test_buffer (i);

    fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);

    /* check that buffer was received by our source pad */
    buffer = buffers->data;

    gst_buffer_map (buffer, &map, GST_MAP_READ);

    num = *(guint64 *) map.data;
    fail_unless_equals_uint64 (i, num);
    fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer),
        gst_util_uint64_scale_round (i, GST_SECOND, TEST_MSECS_PER_SAMPLE));
    fail_unless_equals_uint64 (GST_BUFFER_DURATION (buffer),
        gst_util_uint64_scale_round (1, GST_SECOND, TEST_MSECS_PER_SAMPLE));

    gst_buffer_unmap (buffer, &map);

    gst_buffer_unref (buffer);
    buffers = g_list_delete_link (buffers, buffers);
  }

  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_eos ()));

  fail_unless (buffers == NULL);

  cleanup_audiodecodertest ();
}

GST_END_TEST;

GST_START_TEST (audiodecoder_query_caps_with_fixed_caps_peer)
{
  GstCaps *caps;
  GstCaps *filter;
  GstStructure *structure;
  gint rate, channels;

  setup_audiodecodertester (&sinktemplate_restricted, NULL);

  gst_pad_set_active (mysrcpad, TRUE);
  gst_element_set_state (dec, GST_STATE_PLAYING);
  gst_pad_set_active (mysinkpad, TRUE);

  caps = gst_pad_peer_query_caps (mysrcpad, NULL);
  fail_unless (caps != NULL);

  structure = gst_caps_get_structure (caps, 0);
  fail_unless (gst_structure_get_int (structure, "rate", &rate));
  fail_unless (gst_structure_get_int (structure, "channels", &channels));

  /* match our restricted caps values */
  fail_unless (channels == RESTRICTED_CAPS_CHANNELS);
  fail_unless (rate == RESTRICTED_CAPS_RATE);
  gst_caps_unref (caps);

  filter = gst_caps_new_simple ("audio/x-custom-test", "rate", G_TYPE_INT,
      10000, "channels", G_TYPE_INT, 12, NULL);
  caps = gst_pad_peer_query_caps (mysrcpad, filter);
  fail_unless (caps != NULL);
  fail_unless (gst_caps_is_empty (caps));
  gst_caps_unref (caps);
  gst_caps_unref (filter);

  cleanup_audiodecodertest ();
}

GST_END_TEST;

static void
_get_int_range (GstStructure * s, const gchar * field, gint * min_v,
    gint * max_v)
{
  const GValue *value;

  value = gst_structure_get_value (s, field);
  fail_unless (value != NULL);
  fail_unless (GST_VALUE_HOLDS_INT_RANGE (value));

  *min_v = gst_value_get_int_range_min (value);
  *max_v = gst_value_get_int_range_max (value);
}

GST_START_TEST (audiodecoder_query_caps_with_range_caps_peer)
{
  GstCaps *caps;
  GstCaps *filter;
  GstStructure *structure;
  gint rate, channels;
  gint rate_min, channels_min;
  gint rate_max, channels_max;

  setup_audiodecodertester (&sinktemplate_with_range, NULL);

  gst_pad_set_active (mysrcpad, TRUE);
  gst_element_set_state (dec, GST_STATE_PLAYING);
  gst_pad_set_active (mysinkpad, TRUE);

  caps = gst_pad_peer_query_caps (mysrcpad, NULL);
  fail_unless (caps != NULL);

  structure = gst_caps_get_structure (caps, 0);
  _get_int_range (structure, "rate", &rate_min, &rate_max);
  _get_int_range (structure, "channels", &channels_min, &channels_max);
  fail_unless (rate_min == 1);
  fail_unless (rate_max == RESTRICTED_CAPS_RATE);
  fail_unless (channels_min == 1);
  fail_unless (channels_max == RESTRICTED_CAPS_CHANNELS);
  gst_caps_unref (caps);

  /* query with a fixed filter */
  filter = gst_caps_new_simple ("audio/x-test-custom", "rate", G_TYPE_INT,
      RESTRICTED_CAPS_RATE, "channels", G_TYPE_INT, RESTRICTED_CAPS_CHANNELS,
      NULL);
  caps = gst_pad_peer_query_caps (mysrcpad, filter);
  fail_unless (caps != NULL);
  structure = gst_caps_get_structure (caps, 0);
  fail_unless (gst_structure_get_int (structure, "rate", &rate));
  fail_unless (gst_structure_get_int (structure, "channels", &channels));
  fail_unless (rate == RESTRICTED_CAPS_RATE);
  fail_unless (channels == RESTRICTED_CAPS_CHANNELS);
  gst_caps_unref (caps);
  gst_caps_unref (filter);

  /* query with a fixed filter that will lead to empty result */
  filter = gst_caps_new_simple ("audio/x-test-custom", "rate", G_TYPE_INT,
      10000, "channels", G_TYPE_INT, 12, NULL);
  caps = gst_pad_peer_query_caps (mysrcpad, filter);
  fail_unless (caps != NULL);
  fail_unless (gst_caps_is_empty (caps));
  gst_caps_unref (caps);
  gst_caps_unref (filter);

  cleanup_audiodecodertest ();
}

GST_END_TEST;

#define GETCAPS_CAPS_STR "audio/x-test-custom, somefield=(string)getcaps"
static GstCaps *
_custom_audio_decoder_getcaps (GstAudioDecoder * dec, GstCaps * filter)
{
  return gst_caps_from_string (GETCAPS_CAPS_STR);
}

GST_START_TEST (audiodecoder_query_caps_with_custom_getcaps)
{
  GstCaps *caps;
  GstAudioDecoderClass *klass;
  GstCaps *expected_caps;

  setup_audiodecodertester (&sinktemplate_restricted, NULL);

  klass = GST_AUDIO_DECODER_CLASS (GST_AUDIO_DECODER_GET_CLASS (dec));
  klass->getcaps = _custom_audio_decoder_getcaps;

  gst_pad_set_active (mysrcpad, TRUE);
  gst_element_set_state (dec, GST_STATE_PLAYING);
  gst_pad_set_active (mysinkpad, TRUE);

  caps = gst_pad_peer_query_caps (mysrcpad, NULL);
  fail_unless (caps != NULL);

  expected_caps = gst_caps_from_string (GETCAPS_CAPS_STR);
  fail_unless (gst_caps_is_equal (expected_caps, caps));
  gst_caps_unref (expected_caps);
  gst_caps_unref (caps);

  cleanup_audiodecodertest ();
}

GST_END_TEST;

static GstTagList *
pad_get_sticky_tags (GstPad * pad, GstTagScope scope)
{
  GstTagList *tags = NULL;
  GstEvent *event;
  guint i = 0;

  do {
    event = gst_pad_get_sticky_event (pad, GST_EVENT_TAG, i++);
    if (event == NULL)
      break;
    gst_event_parse_tag (event, &tags);
    if (scope == gst_tag_list_get_scope (tags))
      tags = gst_tag_list_ref (tags);
    else
      tags = NULL;
    gst_event_unref (event);
  }
  while (tags == NULL);

  return tags;
}

#define tag_list_peek_string(list,tag,p_s) \
    gst_tag_list_peek_string_index(list,tag,0,p_s)

/* Check tag transformations and updates */
GST_START_TEST (audiodecoder_tag_handling)
{
  GstTagList *global_tags;
  GstTagList *tags;
  GstSegment segment;
  const gchar *s = NULL;
  guint u = 0;

  setup_audiodecodertester (NULL, NULL);

  gst_pad_set_active (mysrcpad, TRUE);
  gst_element_set_state (dec, GST_STATE_PLAYING);
  gst_pad_set_active (mysinkpad, TRUE);

  send_startup_events ();

  /* push a new segment */
  gst_segment_init (&segment, GST_FORMAT_TIME);
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_segment (&segment)));

  /* =======================================================================
   * SCENARIO 0: global tags passthrough; check upstream/decoder tag merging
   * ======================================================================= */

  /* push some global tags (these should be passed through and not messed with) */
  global_tags = gst_tag_list_new (GST_TAG_TITLE, "Global", NULL);
  gst_tag_list_set_scope (global_tags, GST_TAG_SCOPE_GLOBAL);
  fail_unless (gst_pad_push_event (mysrcpad,
          gst_event_new_tag (gst_tag_list_ref (global_tags))));

  /* create some (upstream) stream tags */
  tags = gst_tag_list_new (GST_TAG_AUDIO_CODEC, "Upstream Codec",
      GST_TAG_DESCRIPTION, "Upstream Description", NULL);
  gst_tag_list_set_scope (tags, GST_TAG_SCOPE_STREAM);
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_tag (tags)));
  tags = NULL;

  /* decoder tags: override/add AUDIO_CODEC, BITRATE and MAXIMUM_BITRATE */
  {
    GstTagList *decoder_tags;

    decoder_tags = gst_tag_list_new (GST_TAG_AUDIO_CODEC, "Decoder Codec",
        GST_TAG_BITRATE, 250000, GST_TAG_MAXIMUM_BITRATE, 255000, NULL);
    gst_audio_decoder_merge_tags (GST_AUDIO_DECODER (dec),
        decoder_tags, GST_TAG_MERGE_REPLACE);
    gst_tag_list_unref (decoder_tags);
  }

  /* push buffer (this will call gst_audio_decoder_merge_tags with the above) */
  fail_unless (gst_pad_push (mysrcpad, create_test_buffer (0)) == GST_FLOW_OK);
  gst_buffer_unref (buffers->data);
  buffers = g_list_delete_link (buffers, buffers);

  /* check global tags: should not have been tampered with */
  tags = pad_get_sticky_tags (mysinkpad, GST_TAG_SCOPE_GLOBAL);
  fail_unless (tags != NULL);
  GST_INFO ("global tags: %" GST_PTR_FORMAT, tags);
  fail_unless (gst_tag_list_is_equal (tags, global_tags));

  /* check merged stream tags */
  tags = pad_get_sticky_tags (mysinkpad, GST_TAG_SCOPE_STREAM);
  fail_unless (tags != NULL);
  GST_INFO ("stream tags: %" GST_PTR_FORMAT, tags);
  /* upstream audio codec should've been replaced with audiodecoder one */
  fail_unless (tag_list_peek_string (tags, GST_TAG_AUDIO_CODEC, &s));
  fail_unless_equals_string (s, "Decoder Codec");
  /* no upstream bitrate, so audiodecoder one should've been added */
  fail_unless (gst_tag_list_get_uint (tags, GST_TAG_BITRATE, &u));
  fail_unless_equals_int (u, 250000);
  /* no upstream maximum-bitrate, so audiodecoder one should've been added */
  fail_unless (gst_tag_list_get_uint (tags, GST_TAG_MAXIMUM_BITRATE, &u));
  fail_unless_equals_int (u, 255000);
  fail_unless (gst_tag_list_get_tag_size (tags, GST_TAG_AUDIO_CODEC) == 1);
  fail_unless (gst_tag_list_get_tag_size (tags, GST_TAG_BITRATE) == 1);
  fail_unless (gst_tag_list_get_tag_size (tags, GST_TAG_MAXIMUM_BITRATE) == 1);
  /* upstream description should've been maintained */
  fail_unless (gst_tag_list_get_tag_size (tags, GST_TAG_DESCRIPTION) == 1);
  /* and that should be all: AUDIO_CODEC, DESCRIPTION, BITRATE, MAX BITRATE */
  fail_unless_equals_int (gst_tag_list_n_tags (tags), 4);
  gst_tag_list_unref (tags);
  s = NULL;

  /* ===================================================================
   * SCENARIO 1: upstream sends updated tags, decoder tags stay the same
   * =================================================================== */

  /* push same upstream stream tags again */
  tags = gst_tag_list_new (GST_TAG_AUDIO_CODEC, "Upstream Codec",
      GST_TAG_DESCRIPTION, "Upstream Description", NULL);
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_tag (tags)));
  tags = NULL;

  /* decoder tags are still:
   * audio-codec = "Decoder Codec", bitrate=250000, maximum-bitrate=255000 */

  /* check possibly updated merged stream tags, should be same as before */
  tags = pad_get_sticky_tags (mysinkpad, GST_TAG_SCOPE_STREAM);
  fail_unless (tags != NULL);
  GST_INFO ("stream tags: %" GST_PTR_FORMAT, tags);
  /* upstream audio codec still be the one merge-replaced by the subclass */
  fail_unless (tag_list_peek_string (tags, GST_TAG_AUDIO_CODEC, &s));
  fail_unless_equals_string (s, "Decoder Codec");
  /* no upstream bitrate, so audiodecoder one should've been added */
  fail_unless (gst_tag_list_get_uint (tags, GST_TAG_BITRATE, &u));
  fail_unless_equals_int (u, 250000);
  fail_unless (gst_tag_list_get_tag_size (tags, GST_TAG_AUDIO_CODEC) == 1);
  fail_unless (gst_tag_list_get_tag_size (tags, GST_TAG_BITRATE) == 1);
  fail_unless (gst_tag_list_get_tag_size (tags, GST_TAG_MAXIMUM_BITRATE) == 1);
  /* upstream description should've been maintained */
  fail_unless (gst_tag_list_get_tag_size (tags, GST_TAG_DESCRIPTION) == 1);
  /* and that should be all: AUDIO_CODEC, DESCRIPTION, BITRATE, MAX BITRATE */
  fail_unless_equals_int (gst_tag_list_n_tags (tags), 4);
  gst_tag_list_unref (tags);
  s = NULL;

  /* =============================================================
   * SCENARIO 2: decoder updates tags, upstream tags stay the same
   * ============================================================= */

  /* new decoder tags: override AUDIO_CODEC, update/add BITRATE,
   * no MAXIMUM_BITRATE this time (which means it should not appear
   * any longer in the output tags now) (bitrate is a different value now) */
  {
    GstTagList *decoder_tags;

    decoder_tags = gst_tag_list_new (GST_TAG_AUDIO_CODEC, "Decoder Codec",
        GST_TAG_BITRATE, 275000, NULL);
    gst_audio_decoder_merge_tags (GST_AUDIO_DECODER (dec),
        decoder_tags, GST_TAG_MERGE_REPLACE);
    gst_tag_list_unref (decoder_tags);
  }

  /* push another buffer to make decoder update tags */
  fail_unless (gst_pad_push (mysrcpad, create_test_buffer (2)) == GST_FLOW_OK);
  gst_buffer_unref (buffers->data);
  buffers = g_list_delete_link (buffers, buffers);

  /* check updated merged stream tags, the decoder bits should be different */
  tags = pad_get_sticky_tags (mysinkpad, GST_TAG_SCOPE_STREAM);
  fail_unless (tags != NULL);
  GST_INFO ("stream tags: %" GST_PTR_FORMAT, tags);
  /* upstream audio codec still replaced by the subclass's (wasn't updated) */
  fail_unless (tag_list_peek_string (tags, GST_TAG_AUDIO_CODEC, &s));
  fail_unless_equals_string (s, "Decoder Codec");
  /* no upstream bitrate, so audiodecoder one should've been added, was updated */
  fail_unless (gst_tag_list_get_uint (tags, GST_TAG_BITRATE, &u));
  fail_unless_equals_int (u, 275000);
  /* no upstream maximum-bitrate, and audiodecoder removed it now */
  fail_unless (!gst_tag_list_get_uint (tags, GST_TAG_MAXIMUM_BITRATE, &u));
  fail_unless (gst_tag_list_get_tag_size (tags, GST_TAG_AUDIO_CODEC) == 1);
  fail_unless (gst_tag_list_get_tag_size (tags, GST_TAG_BITRATE) == 1);
  /* upstream description should've been maintained */
  fail_unless (gst_tag_list_get_tag_size (tags, GST_TAG_DESCRIPTION) == 1);
  /* and that should be all, just AUDIO_CODEC, DESCRIPTION, BITRATE */
  fail_unless_equals_int (gst_tag_list_n_tags (tags), 3);
  gst_tag_list_unref (tags);
  s = NULL;

  /* =================================================================
   * SCENARIO 3: stream-start event should clear upstream tags
   * ================================================================= */

  /* also tests if the stream-start event clears the upstream tags */
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_stream_start ("x")));

  /* push another buffer to make decoder update tags */
  fail_unless (gst_pad_push (mysrcpad, create_test_buffer (3)) == GST_FLOW_OK);
  gst_buffer_unref (buffers->data);
  buffers = g_list_delete_link (buffers, buffers);

  /* check updated merged stream tags, should be just decoder tags now */
  tags = pad_get_sticky_tags (mysinkpad, GST_TAG_SCOPE_STREAM);
  fail_unless (tags != NULL);
  GST_INFO ("stream tags: %" GST_PTR_FORMAT, tags);
  fail_unless (tag_list_peek_string (tags, GST_TAG_AUDIO_CODEC, &s));
  fail_unless_equals_string (s, "Decoder Codec");
  fail_unless (gst_tag_list_get_uint (tags, GST_TAG_BITRATE, &u));
  fail_unless_equals_int (u, 275000);
  /* no upstream maximum-bitrate, and audiodecoder removed it now */
  fail_unless (!gst_tag_list_get_uint (tags, GST_TAG_MAXIMUM_BITRATE, &u));
  fail_unless (gst_tag_list_get_tag_size (tags, GST_TAG_AUDIO_CODEC) == 1);
  fail_unless (gst_tag_list_get_tag_size (tags, GST_TAG_BITRATE) == 1);
  /* no more description tag since no more upstream tags */
  fail_unless (gst_tag_list_get_tag_size (tags, GST_TAG_DESCRIPTION) == 0);
  /* and that should be all, just AUDIO_CODEC, BITRATE */
  fail_unless_equals_int (gst_tag_list_n_tags (tags), 2);
  gst_tag_list_unref (tags);
  s = NULL;

  /* clean up */
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_eos ()));
  fail_unless (buffers == NULL);

  cleanup_audiodecodertest ();
  gst_tag_list_unref (global_tags);
}

GST_END_TEST;

static Suite *
gst_audiodecoder_suite (void)
{
  Suite *s = suite_create ("GstAudioDecoder");
  TCase *tc = tcase_create ("general");

  suite_add_tcase (s, tc);
  tcase_add_test (tc, audiodecoder_playback);
  tcase_add_test (tc, audiodecoder_flush_events_no_buffers);
  tcase_add_test (tc, audiodecoder_eos_events_no_buffers);
  tcase_add_test (tc, audiodecoder_flush_events);
  tcase_add_test (tc, audiodecoder_negotiation_with_buffer);
  tcase_add_test (tc, audiodecoder_negotiation_with_gap_event);
  tcase_add_test (tc, audiodecoder_delayed_negotiation_with_gap_event);
  tcase_add_test (tc, audiodecoder_first_data_is_gap);
  tcase_add_test (tc, audiodecoder_buffer_after_segment);
  tcase_add_test (tc, audiodecoder_output_too_many_frames);
  tcase_add_test (tc, audiodecoder_tag_handling);

  tcase_add_test (tc, audiodecoder_query_caps_with_fixed_caps_peer);
  tcase_add_test (tc, audiodecoder_query_caps_with_range_caps_peer);
  tcase_add_test (tc, audiodecoder_query_caps_with_custom_getcaps);

  return s;
}

GST_CHECK_MAIN (gst_audiodecoder);
