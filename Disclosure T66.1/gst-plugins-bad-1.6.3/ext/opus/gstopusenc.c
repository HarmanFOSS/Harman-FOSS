/* GStreamer Opus Encoder
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2008> Sebastian Dröge <sebastian.droege@collabora.co.uk>
 * Copyright (C) <2011> Vincent Penquerc'h <vincent.penquerch@collabora.co.uk>
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

/*
 * Based on the speexenc element
 */

/**
 * SECTION:element-opusenc
 * @see_also: opusdec, oggmux
 *
 * This element encodes raw audio to OPUS.
 *
 * <refsect2>
 * <title>Example pipelines</title>
 * |[
 * gst-launch -v audiotestsrc wave=sine num-buffers=100 ! audioconvert ! opusenc ! oggmux ! filesink location=sine.ogg
 * ]| Encode a test sine signal to Ogg/OPUS.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <opus.h>

#include <gst/gsttagsetter.h>
#include <gst/audio/audio.h>
#include <gst/glib-compat-private.h>
#include "gstopusheader.h"
#include "gstopuscommon.h"
#include "gstopusenc.h"

GST_DEBUG_CATEGORY_STATIC (opusenc_debug);
#define GST_CAT_DEFAULT opusenc_debug

/* Some arbitrary bounds beyond which it really doesn't make sense.
   The spec mentions 6 kb/s to 510 kb/s, so 4000 and 650000 ought to be
   safe as property bounds. */
#define LOWEST_BITRATE 4000
#define HIGHEST_BITRATE 650000

#define GST_OPUS_ENC_TYPE_BANDWIDTH (gst_opus_enc_bandwidth_get_type())
static GType
gst_opus_enc_bandwidth_get_type (void)
{
  static const GEnumValue values[] = {
    {OPUS_BANDWIDTH_NARROWBAND, "Narrow band", "narrowband"},
    {OPUS_BANDWIDTH_MEDIUMBAND, "Medium band", "mediumband"},
    {OPUS_BANDWIDTH_WIDEBAND, "Wide band", "wideband"},
    {OPUS_BANDWIDTH_SUPERWIDEBAND, "Super wide band", "superwideband"},
    {OPUS_BANDWIDTH_FULLBAND, "Full band", "fullband"},
    {OPUS_AUTO, "Auto", "auto"},
    {0, NULL, NULL}
  };
  static volatile GType id = 0;

  if (g_once_init_enter ((gsize *) & id)) {
    GType _id;

    _id = g_enum_register_static ("GstOpusEncBandwidth", values);

    g_once_init_leave ((gsize *) & id, _id);
  }

  return id;
}

#define GST_OPUS_ENC_TYPE_FRAME_SIZE (gst_opus_enc_frame_size_get_type())
static GType
gst_opus_enc_frame_size_get_type (void)
{
  static const GEnumValue values[] = {
    {2, "2.5", "2.5"},
    {5, "5", "5"},
    {10, "10", "10"},
    {20, "20", "20"},
    {40, "40", "40"},
    {60, "60", "60"},
    {0, NULL, NULL}
  };
  static volatile GType id = 0;

  if (g_once_init_enter ((gsize *) & id)) {
    GType _id;

    _id = g_enum_register_static ("GstOpusEncFrameSize", values);

    g_once_init_leave ((gsize *) & id, _id);
  }

  return id;
}

#define GST_OPUS_ENC_TYPE_AUDIO_TYPE (gst_opus_enc_audio_type_get_type())
static GType
gst_opus_enc_audio_type_get_type (void)
{
  static const GEnumValue values[] = {
    {OPUS_APPLICATION_AUDIO, "Generic audio", "generic"},
    {OPUS_APPLICATION_VOIP, "Voice", "voice"},
    {0, NULL, NULL}
  };
  static volatile GType id = 0;

  if (g_once_init_enter ((gsize *) & id)) {
    GType _id;

    _id = g_enum_register_static ("GstOpusEncAudioType", values);

    g_once_init_leave ((gsize *) & id, _id);
  }

  return id;
}

#define GST_OPUS_ENC_TYPE_BITRATE_TYPE (gst_opus_enc_bitrate_type_get_type())
static GType
gst_opus_enc_bitrate_type_get_type (void)
{
  static const GEnumValue values[] = {
    {BITRATE_TYPE_CBR, "CBR", "cbr"},
    {BITRATE_TYPE_VBR, "VBR", "vbr"},
    {BITRATE_TYPE_CONSTRAINED_VBR, "Constrained VBR", "constrained-vbr"},
    {0, NULL, NULL}
  };
  static volatile GType id = 0;

  if (g_once_init_enter ((gsize *) & id)) {
    GType _id;

    _id = g_enum_register_static ("GstOpusEncBitrateType", values);

    g_once_init_leave ((gsize *) & id, _id);
  }

  return id;
}

#define FORMAT_STR GST_AUDIO_NE(S16)
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " FORMAT_STR ", "
        "layout = (string) interleaved, "
        "rate = (int) 48000, "
        "channels = (int) [ 1, 2 ]; "
        "audio/x-raw, "
        "format = (string) " FORMAT_STR ", "
        "layout = (string) interleaved, "
        "rate = (int) { 8000, 12000, 16000, 24000 }, "
        "channels = (int) [ 1, 2 ] ")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-opus")
    );

#define DEFAULT_AUDIO           TRUE
#define DEFAULT_AUDIO_TYPE      OPUS_APPLICATION_AUDIO
#define DEFAULT_BITRATE         64000
#define DEFAULT_BANDWIDTH       OPUS_BANDWIDTH_FULLBAND
#define DEFAULT_FRAMESIZE       20
#define DEFAULT_CBR             TRUE
#define DEFAULT_CONSTRAINED_VBR TRUE
#define DEFAULT_BITRATE_TYPE    BITRATE_TYPE_CBR
#define DEFAULT_COMPLEXITY      10
#define DEFAULT_INBAND_FEC      FALSE
#define DEFAULT_DTX             FALSE
#define DEFAULT_PACKET_LOSS_PERCENT 0
#define DEFAULT_MAX_PAYLOAD_SIZE 4000

enum
{
  PROP_0,
  PROP_AUDIO,
  PROP_AUDIO_TYPE,
  PROP_BITRATE,
  PROP_BANDWIDTH,
  PROP_FRAME_SIZE,
  PROP_CBR,
  PROP_CONSTRAINED_VBR,
  PROP_BITRATE_TYPE,
  PROP_COMPLEXITY,
  PROP_INBAND_FEC,
  PROP_DTX,
  PROP_PACKET_LOSS_PERCENT,
  PROP_MAX_PAYLOAD_SIZE
};

static void gst_opus_enc_finalize (GObject * object);

static gboolean gst_opus_enc_sink_event (GstAudioEncoder * benc,
    GstEvent * event);
static GstCaps *gst_opus_enc_sink_getcaps (GstAudioEncoder * benc,
    GstCaps * filter);
static gboolean gst_opus_enc_setup (GstOpusEnc * enc);

static void gst_opus_enc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_opus_enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);

static void gst_opus_enc_set_tags (GstOpusEnc * enc);
static gboolean gst_opus_enc_start (GstAudioEncoder * benc);
static gboolean gst_opus_enc_stop (GstAudioEncoder * benc);
static gboolean gst_opus_enc_set_format (GstAudioEncoder * benc,
    GstAudioInfo * info);
static GstFlowReturn gst_opus_enc_handle_frame (GstAudioEncoder * benc,
    GstBuffer * buf);
static gint64 gst_opus_enc_get_latency (GstOpusEnc * enc);

static GstFlowReturn gst_opus_enc_encode (GstOpusEnc * enc, GstBuffer * buffer);

#define gst_opus_enc_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstOpusEnc, gst_opus_enc, GST_TYPE_AUDIO_ENCODER,
    G_IMPLEMENT_INTERFACE (GST_TYPE_TAG_SETTER, NULL);
    G_IMPLEMENT_INTERFACE (GST_TYPE_PRESET, NULL));

static void
gst_opus_enc_set_tags (GstOpusEnc * enc)
{
  GstTagList *taglist;

  /* create a taglist and add a bitrate tag to it */
  taglist = gst_tag_list_new_empty ();
  gst_tag_list_add (taglist, GST_TAG_MERGE_REPLACE,
      GST_TAG_BITRATE, enc->bitrate, NULL);

  gst_audio_encoder_merge_tags (GST_AUDIO_ENCODER (enc), taglist,
      GST_TAG_MERGE_REPLACE);

  gst_tag_list_unref (taglist);
}

static void
gst_opus_enc_class_init (GstOpusEncClass * klass)
{
  GObjectClass *gobject_class;
  GstAudioEncoderClass *base_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  base_class = (GstAudioEncoderClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_opus_enc_set_property;
  gobject_class->get_property = gst_opus_enc_get_property;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
  gst_element_class_set_static_metadata (gstelement_class, "Opus audio encoder",
      "Codec/Encoder/Audio",
      "Encodes audio in Opus format",
      "Vincent Penquerc'h <vincent.penquerch@collabora.co.uk>");

  base_class->start = GST_DEBUG_FUNCPTR (gst_opus_enc_start);
  base_class->stop = GST_DEBUG_FUNCPTR (gst_opus_enc_stop);
  base_class->set_format = GST_DEBUG_FUNCPTR (gst_opus_enc_set_format);
  base_class->handle_frame = GST_DEBUG_FUNCPTR (gst_opus_enc_handle_frame);
  base_class->sink_event = GST_DEBUG_FUNCPTR (gst_opus_enc_sink_event);
  base_class->getcaps = GST_DEBUG_FUNCPTR (gst_opus_enc_sink_getcaps);

  g_object_class_install_property (gobject_class, PROP_AUDIO,
      g_param_spec_boolean ("audio",
          "Audio or voice",
          "Audio or voice (DEPRECATED: use audio-type)", DEFAULT_AUDIO,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_DEPRECATED));
  g_object_class_install_property (gobject_class, PROP_AUDIO_TYPE,
      g_param_spec_enum ("audio-type", "What type of audio to optimize for",
          "What type of audio to optimize for", GST_OPUS_ENC_TYPE_AUDIO_TYPE,
          DEFAULT_AUDIO_TYPE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_BITRATE,
      g_param_spec_int ("bitrate", "Encoding Bit-rate",
          "Specify an encoding bit-rate (in bps).", LOWEST_BITRATE,
          HIGHEST_BITRATE, DEFAULT_BITRATE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
          GST_PARAM_MUTABLE_PLAYING));
  g_object_class_install_property (gobject_class, PROP_BANDWIDTH,
      g_param_spec_enum ("bandwidth", "Band Width", "Audio Band Width",
          GST_OPUS_ENC_TYPE_BANDWIDTH, DEFAULT_BANDWIDTH,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
          GST_PARAM_MUTABLE_PLAYING));
  g_object_class_install_property (gobject_class, PROP_FRAME_SIZE,
      g_param_spec_enum ("frame-size", "Frame Size",
          "The duration of an audio frame, in ms", GST_OPUS_ENC_TYPE_FRAME_SIZE,
          DEFAULT_FRAMESIZE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
          GST_PARAM_MUTABLE_PLAYING));
  g_object_class_install_property (gobject_class, PROP_CBR,
      g_param_spec_boolean ("cbr", "Constant bit rate",
          "Constant bit rate (DEPRECATED: use bitrate-type)", DEFAULT_CBR,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING
          | G_PARAM_DEPRECATED));
  g_object_class_install_property (gobject_class, PROP_CONSTRAINED_VBR,
      g_param_spec_boolean ("constrained-vbr", "Constrained VBR",
          "Constrained VBR (DEPRECATED: use bitrate-type)",
          DEFAULT_CONSTRAINED_VBR,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING
          | G_PARAM_DEPRECATED));
  g_object_class_install_property (gobject_class, PROP_BITRATE_TYPE,
      g_param_spec_enum ("bitrate-type", "Bitrate type", "Bitrate type",
          GST_OPUS_ENC_TYPE_BITRATE_TYPE, DEFAULT_BITRATE_TYPE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
          GST_PARAM_MUTABLE_PLAYING));
  g_object_class_install_property (gobject_class, PROP_COMPLEXITY,
      g_param_spec_int ("complexity", "Complexity", "Complexity", 0, 10,
          DEFAULT_COMPLEXITY,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
          GST_PARAM_MUTABLE_PLAYING));
  g_object_class_install_property (gobject_class, PROP_INBAND_FEC,
      g_param_spec_boolean ("inband-fec", "In-band FEC",
          "Enable forward error correction", DEFAULT_INBAND_FEC,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
          GST_PARAM_MUTABLE_PLAYING));
  g_object_class_install_property (gobject_class, PROP_DTX,
      g_param_spec_boolean ("dtx", "DTX", "DTX", DEFAULT_DTX,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
          GST_PARAM_MUTABLE_PLAYING));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_PACKET_LOSS_PERCENT, g_param_spec_int ("packet-loss-percentage",
          "Loss percentage", "Packet loss percentage", 0, 100,
          DEFAULT_PACKET_LOSS_PERCENT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
          GST_PARAM_MUTABLE_PLAYING));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_MAX_PAYLOAD_SIZE, g_param_spec_uint ("max-payload-size",
          "Max payload size", "Maximum payload size in bytes", 2, 4000,
          DEFAULT_MAX_PAYLOAD_SIZE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
          GST_PARAM_MUTABLE_PLAYING));

  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_opus_enc_finalize);

  GST_DEBUG_CATEGORY_INIT (opusenc_debug, "opusenc", 0, "Opus encoder");
}

static void
gst_opus_enc_finalize (GObject * object)
{
  GstOpusEnc *enc;

  enc = GST_OPUS_ENC (object);

  g_mutex_clear (&enc->property_lock);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_opus_enc_init (GstOpusEnc * enc)
{
  GstAudioEncoder *benc = GST_AUDIO_ENCODER (enc);

  GST_DEBUG_OBJECT (enc, "init");

  GST_PAD_SET_ACCEPT_TEMPLATE (GST_AUDIO_ENCODER_SINK_PAD (enc));

  g_mutex_init (&enc->property_lock);

  enc->n_channels = -1;
  enc->sample_rate = -1;
  enc->frame_samples = 0;

  enc->bitrate = DEFAULT_BITRATE;
  enc->bandwidth = DEFAULT_BANDWIDTH;
  enc->frame_size = DEFAULT_FRAMESIZE;
  enc->bitrate_type = DEFAULT_BITRATE_TYPE;
  enc->complexity = DEFAULT_COMPLEXITY;
  enc->inband_fec = DEFAULT_INBAND_FEC;
  enc->dtx = DEFAULT_DTX;
  enc->packet_loss_percentage = DEFAULT_PACKET_LOSS_PERCENT;
  enc->max_payload_size = DEFAULT_MAX_PAYLOAD_SIZE;
  enc->audio_type = DEFAULT_AUDIO_TYPE;

  /* arrange granulepos marking (and required perfect ts) */
  gst_audio_encoder_set_mark_granule (benc, TRUE);
  gst_audio_encoder_set_perfect_timestamp (benc, TRUE);
}

static gboolean
gst_opus_enc_start (GstAudioEncoder * benc)
{
  GstOpusEnc *enc = GST_OPUS_ENC (benc);

  GST_DEBUG_OBJECT (enc, "start");
  enc->encoded_samples = 0;

  return TRUE;
}

static gboolean
gst_opus_enc_stop (GstAudioEncoder * benc)
{
  GstOpusEnc *enc = GST_OPUS_ENC (benc);

  GST_DEBUG_OBJECT (enc, "stop");
  if (enc->state) {
    opus_multistream_encoder_destroy (enc->state);
    enc->state = NULL;
  }
  gst_tag_setter_reset_tags (GST_TAG_SETTER (enc));

  return TRUE;
}

static gint64
gst_opus_enc_get_latency (GstOpusEnc * enc)
{
  gint64 latency = gst_util_uint64_scale (enc->frame_samples, GST_SECOND,
      enc->sample_rate);
  GST_DEBUG_OBJECT (enc, "Latency: %" GST_TIME_FORMAT, GST_TIME_ARGS (latency));
  return latency;
}

static void
gst_opus_enc_setup_base_class (GstOpusEnc * enc, GstAudioEncoder * benc)
{
  gst_audio_encoder_set_latency (benc,
      gst_opus_enc_get_latency (enc), gst_opus_enc_get_latency (enc));
  gst_audio_encoder_set_frame_samples_min (benc, enc->frame_samples);
  gst_audio_encoder_set_frame_samples_max (benc, enc->frame_samples);
  gst_audio_encoder_set_frame_max (benc, 1);
}

static gint
gst_opus_enc_get_frame_samples (GstOpusEnc * enc)
{
  gint frame_samples = 0;
  switch (enc->frame_size) {
    case 2:
      frame_samples = enc->sample_rate / 400;
      break;
    case 5:
      frame_samples = enc->sample_rate / 200;
      break;
    case 10:
      frame_samples = enc->sample_rate / 100;
      break;
    case 20:
      frame_samples = enc->sample_rate / 50;
      break;
    case 40:
      frame_samples = enc->sample_rate / 25;
      break;
    case 60:
      frame_samples = 3 * enc->sample_rate / 50;
      break;
    default:
      GST_WARNING_OBJECT (enc, "Unsupported frame size: %d", enc->frame_size);
      frame_samples = 0;
      break;
  }
  return frame_samples;
}

static void
gst_opus_enc_setup_trivial_mapping (GstOpusEnc * enc, guint8 mapping[256])
{
  int n;

  for (n = 0; n < 255; ++n)
    mapping[n] = n;
}

static int
gst_opus_enc_find_channel_position (GstOpusEnc * enc, const GstAudioInfo * info,
    GstAudioChannelPosition position)
{
  int n;
  for (n = 0; n < enc->n_channels; ++n) {
    if (GST_AUDIO_INFO_POSITION (info, n) == position) {
      return n;
    }
  }
  return -1;
}

static int
gst_opus_enc_find_channel_position_in_vorbis_order (GstOpusEnc * enc,
    GstAudioChannelPosition position)
{
  int c;

  for (c = 0; c < enc->n_channels; ++c) {
    if (gst_opus_channel_positions[enc->n_channels - 1][c] == position) {
      GST_INFO_OBJECT (enc,
          "Channel position %s maps to index %d in Vorbis order",
          gst_opus_channel_names[position], c);
      return c;
    }
  }
  GST_WARNING_OBJECT (enc,
      "Channel position %s is not representable in Vorbis order",
      gst_opus_channel_names[position]);
  return -1;
}

static void
gst_opus_enc_setup_channel_mappings (GstOpusEnc * enc,
    const GstAudioInfo * info)
{
#define MAPS(idx,pos) (GST_AUDIO_INFO_POSITION (info, (idx)) == GST_AUDIO_CHANNEL_POSITION_##pos)

  int n;

  GST_DEBUG_OBJECT (enc, "Setting up channel mapping for %d channels",
      enc->n_channels);

  /* Start by setting up a default trivial mapping */
  enc->n_stereo_streams = 0;
  gst_opus_enc_setup_trivial_mapping (enc, enc->encoding_channel_mapping);
  gst_opus_enc_setup_trivial_mapping (enc, enc->decoding_channel_mapping);

  /* For one channel, use the basic RTP mapping */
  if (enc->n_channels == 1) {
    GST_INFO_OBJECT (enc, "Mono, trivial RTP mapping");
    enc->channel_mapping_family = 0;
    /* implicit mapping for family 0 */
    return;
  }

  /* For two channels, use the basic RTP mapping if the channels are
     mapped as left/right. */
  if (enc->n_channels == 2) {
    if (MAPS (0, FRONT_LEFT) && MAPS (1, FRONT_RIGHT)) {
      GST_INFO_OBJECT (enc, "Stereo, canonical mapping");
      enc->channel_mapping_family = 0;
      enc->n_stereo_streams = 1;
      /* The channel mapping is implicit for family 0, that's why we do not
         attempt to create one for right/left - this will be mapped to the
         Vorbis mapping below. */
      return;
    } else {
      GST_DEBUG_OBJECT (enc, "Stereo, but not canonical mapping, continuing");
    }
  }

  /* For channels between 1 and 8, we use the Vorbis mapping if we can
     find a permutation that matches it. Mono will have been taken care
     of earlier, but this code also handles it. Same for left/right stereo.
     There are two mappings. One maps the input channels to an ordering
     which has the natural pairs first so they can benefit from the Opus
     stereo channel coupling, and the other maps this ordering to the
     Vorbis ordering. */
  if (enc->n_channels >= 1 && enc->n_channels <= 8) {
    int c0, c1, c0v, c1v;
    int mapped;
    gboolean positions_done[256];
    static const GstAudioChannelPosition pairs[][2] = {
      {GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
          GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT},
      {GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
          GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT},
      {GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER,
          GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER},
      {GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER,
          GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER},
      {GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
          GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT},
    };
    size_t pair;

    GST_DEBUG_OBJECT (enc,
        "In range for the Vorbis mapping, building channel mapping tables");

    enc->n_stereo_streams = 0;
    mapped = 0;
    for (n = 0; n < 256; ++n)
      positions_done[n] = FALSE;

    /* First, find any natural pairs, and move them to the front */
    for (pair = 0; pair < G_N_ELEMENTS (pairs); ++pair) {
      GstAudioChannelPosition p0 = pairs[pair][0];
      GstAudioChannelPosition p1 = pairs[pair][1];
      c0 = gst_opus_enc_find_channel_position (enc, info, p0);
      c1 = gst_opus_enc_find_channel_position (enc, info, p1);
      if (c0 >= 0 && c1 >= 0) {
        /* We found a natural pair */
        GST_DEBUG_OBJECT (enc, "Natural pair '%s/%s' found at %d %d",
            gst_opus_channel_names[p0], gst_opus_channel_names[p1], c0, c1);
        /* Find where they map in Vorbis order */
        c0v = gst_opus_enc_find_channel_position_in_vorbis_order (enc, p0);
        c1v = gst_opus_enc_find_channel_position_in_vorbis_order (enc, p1);
        if (c0v < 0 || c1v < 0) {
          GST_WARNING_OBJECT (enc,
              "Cannot map channel positions to Vorbis order, using unknown mapping");
          enc->channel_mapping_family = 255;
          enc->n_stereo_streams = 0;
          return;
        }

        enc->encoding_channel_mapping[mapped] = c0;
        enc->encoding_channel_mapping[mapped + 1] = c1;
        enc->decoding_channel_mapping[c0v] = mapped;
        enc->decoding_channel_mapping[c1v] = mapped + 1;
        enc->n_stereo_streams++;
        mapped += 2;
        positions_done[p0] = positions_done[p1] = TRUE;
      }
    }

    /* Now add all other input channels as mono streams */
    for (n = 0; n < enc->n_channels; ++n) {
      GstAudioChannelPosition position = GST_AUDIO_INFO_POSITION (info, n);

      /* if we already mapped it while searching for pairs, nothing else
         needs to be done */
      if (!positions_done[position]) {
        int cv;
        GST_DEBUG_OBJECT (enc, "Channel position %s is not mapped yet, adding",
            gst_opus_channel_names[position]);
        cv = gst_opus_enc_find_channel_position_in_vorbis_order (enc, position);
        if (cv < 0) {
          GST_WARNING_OBJECT (enc,
              "Cannot map channel positions to Vorbis order, using unknown mapping");
          enc->channel_mapping_family = 255;
          enc->n_stereo_streams = 0;
          return;
        }
        enc->encoding_channel_mapping[mapped] = n;
        enc->decoding_channel_mapping[cv] = mapped;
        mapped++;
      }
    }

#ifndef GST_DISABLE_GST_DEBUG
    GST_INFO_OBJECT (enc,
        "Mapping tables built: %d channels, %d stereo streams", enc->n_channels,
        enc->n_stereo_streams);
    gst_opus_common_log_channel_mapping_table (GST_ELEMENT (enc), opusenc_debug,
        "Encoding mapping table", enc->n_channels,
        enc->encoding_channel_mapping);
    gst_opus_common_log_channel_mapping_table (GST_ELEMENT (enc), opusenc_debug,
        "Decoding mapping table", enc->n_channels,
        enc->decoding_channel_mapping);
#endif

    enc->channel_mapping_family = 1;
    return;
  }

  /* More than 8 channels, if future mappings are added for those */

  /* For other cases, we use undefined, with the default trivial mapping
     and all mono streams */
  GST_WARNING_OBJECT (enc, "Unknown mapping");
  enc->channel_mapping_family = 255;
  enc->n_stereo_streams = 0;

#undef MAPS
}

static gboolean
gst_opus_enc_set_format (GstAudioEncoder * benc, GstAudioInfo * info)
{
  GstOpusEnc *enc;

  enc = GST_OPUS_ENC (benc);

  g_mutex_lock (&enc->property_lock);

  enc->n_channels = GST_AUDIO_INFO_CHANNELS (info);
  enc->sample_rate = GST_AUDIO_INFO_RATE (info);
  gst_opus_enc_setup_channel_mappings (enc, info);
  GST_DEBUG_OBJECT (benc, "Setup with %d channels, %d Hz", enc->n_channels,
      enc->sample_rate);

  /* handle reconfigure */
  if (enc->state) {
    opus_multistream_encoder_destroy (enc->state);
    enc->state = NULL;
  }
  if (!gst_opus_enc_setup (enc)) {
    g_mutex_unlock (&enc->property_lock);
    return FALSE;
  }

  /* update the tags */
  gst_opus_enc_set_tags (enc);

  enc->frame_samples = gst_opus_enc_get_frame_samples (enc);

  /* feedback to base class */
  gst_opus_enc_setup_base_class (enc, benc);

  g_mutex_unlock (&enc->property_lock);

  return TRUE;
}

static gboolean
gst_opus_enc_setup (GstOpusEnc * enc)
{
  int error = OPUS_OK;
  GstCaps *caps;
  gboolean ret;

#ifndef GST_DISABLE_GST_DEBUG
  GST_DEBUG_OBJECT (enc,
      "setup: %d Hz, %d channels, %d stereo streams, family %d",
      enc->sample_rate, enc->n_channels, enc->n_stereo_streams,
      enc->channel_mapping_family);
  GST_INFO_OBJECT (enc, "Mapping tables built: %d channels, %d stereo streams",
      enc->n_channels, enc->n_stereo_streams);
  gst_opus_common_log_channel_mapping_table (GST_ELEMENT (enc), opusenc_debug,
      "Encoding mapping table", enc->n_channels, enc->encoding_channel_mapping);
  gst_opus_common_log_channel_mapping_table (GST_ELEMENT (enc), opusenc_debug,
      "Decoding mapping table", enc->n_channels, enc->decoding_channel_mapping);
#endif

  enc->state = opus_multistream_encoder_create (enc->sample_rate,
      enc->n_channels, enc->n_channels - enc->n_stereo_streams,
      enc->n_stereo_streams, enc->encoding_channel_mapping,
      enc->audio_type, &error);
  if (!enc->state || error != OPUS_OK)
    goto encoder_creation_failed;

  opus_multistream_encoder_ctl (enc->state, OPUS_SET_BITRATE (enc->bitrate), 0);
  opus_multistream_encoder_ctl (enc->state, OPUS_SET_BANDWIDTH (enc->bandwidth),
      0);
  opus_multistream_encoder_ctl (enc->state,
      OPUS_SET_VBR (enc->bitrate_type != BITRATE_TYPE_CBR), 0);
  opus_multistream_encoder_ctl (enc->state,
      OPUS_SET_VBR_CONSTRAINT (enc->bitrate_type ==
          BITRATE_TYPE_CONSTRAINED_VBR), 0);
  opus_multistream_encoder_ctl (enc->state,
      OPUS_SET_COMPLEXITY (enc->complexity), 0);
  opus_multistream_encoder_ctl (enc->state,
      OPUS_SET_INBAND_FEC (enc->inband_fec), 0);
  opus_multistream_encoder_ctl (enc->state, OPUS_SET_DTX (enc->dtx), 0);
  opus_multistream_encoder_ctl (enc->state,
      OPUS_SET_PACKET_LOSS_PERC (enc->packet_loss_percentage), 0);

  GST_LOG_OBJECT (enc, "we have frame size %d", enc->frame_size);

  gst_opus_header_create_caps (&caps, NULL, enc->n_channels,
      enc->n_stereo_streams, enc->sample_rate, enc->channel_mapping_family,
      enc->decoding_channel_mapping,
      gst_tag_setter_get_tag_list (GST_TAG_SETTER (enc)));

  /* negotiate with these caps */
  GST_DEBUG_OBJECT (enc, "here are the caps: %" GST_PTR_FORMAT, caps);

  ret = gst_audio_encoder_set_output_format (GST_AUDIO_ENCODER (enc), caps);
  gst_caps_unref (caps);

  return ret;

encoder_creation_failed:
  GST_ERROR_OBJECT (enc, "Encoder creation failed");
  return FALSE;
}

static gboolean
gst_opus_enc_sink_event (GstAudioEncoder * benc, GstEvent * event)
{
  GstOpusEnc *enc;

  enc = GST_OPUS_ENC (benc);

  GST_DEBUG_OBJECT (enc, "sink event: %s", GST_EVENT_TYPE_NAME (event));
  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_TAG:
    {
      GstTagList *list;
      GstTagSetter *setter = GST_TAG_SETTER (enc);
      const GstTagMergeMode mode = gst_tag_setter_get_tag_merge_mode (setter);

      gst_event_parse_tag (event, &list);
      gst_tag_setter_merge_tags (setter, list, mode);
      break;
    }
    case GST_EVENT_SEGMENT:
      enc->encoded_samples = 0;
      break;

    default:
      break;
  }

  return GST_AUDIO_ENCODER_CLASS (parent_class)->sink_event (benc, event);
}

static GstCaps *
gst_opus_enc_sink_getcaps (GstAudioEncoder * benc, GstCaps * filter)
{
  GstOpusEnc *enc;
  GstCaps *caps;
  GstCaps *tcaps;
  GstCaps *peercaps = NULL;
  GstCaps *intersect = NULL;
  guint i;
  gboolean allow_multistream;

  enc = GST_OPUS_ENC (benc);

  GST_DEBUG_OBJECT (enc, "sink getcaps");

  peercaps = gst_pad_peer_query_caps (GST_AUDIO_ENCODER_SRC_PAD (benc), NULL);
  if (!peercaps) {
    GST_DEBUG_OBJECT (benc, "No peercaps, returning template sink caps");
    return gst_pad_get_pad_template_caps (GST_AUDIO_ENCODER_SINK_PAD (benc));
  }

  tcaps = gst_pad_get_pad_template_caps (GST_AUDIO_ENCODER_SRC_PAD (benc));
  intersect = gst_caps_intersect (peercaps, tcaps);
  gst_caps_unref (tcaps);
  gst_caps_unref (peercaps);

  if (gst_caps_is_empty (intersect))
    return intersect;

  allow_multistream = FALSE;
  for (i = 0; i < gst_caps_get_size (intersect); i++) {
    GstStructure *s = gst_caps_get_structure (intersect, i);
    gboolean multistream;
    if (gst_structure_get_boolean (s, "multistream", &multistream)) {
      if (multistream) {
        allow_multistream = TRUE;
      }
    } else {
      allow_multistream = TRUE;
    }
  }

  gst_caps_unref (intersect);

  caps = gst_pad_get_pad_template_caps (GST_AUDIO_ENCODER_SINK_PAD (benc));
  caps = gst_caps_make_writable (caps);
  if (!allow_multistream) {
    GValue range = { 0 };
    g_value_init (&range, GST_TYPE_INT_RANGE);
    gst_value_set_int_range (&range, 1, 2);
    for (i = 0; i < gst_caps_get_size (caps); i++) {
      GstStructure *s = gst_caps_get_structure (caps, i);
      gst_structure_set_value (s, "channels", &range);
    }
    g_value_unset (&range);
  }

  if (filter) {
    GstCaps *tmp = gst_caps_intersect_full (caps, filter,
        GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (caps);
    caps = tmp;
  }

  GST_DEBUG_OBJECT (enc, "Returning caps: %" GST_PTR_FORMAT, caps);
  return caps;
}

static GstFlowReturn
gst_opus_enc_encode (GstOpusEnc * enc, GstBuffer * buf)
{
  guint8 *bdata = NULL, *data, *mdata = NULL;
  gsize bsize, size;
  gsize bytes;
  gint ret = GST_FLOW_OK;
  GstMapInfo map;
  GstMapInfo omap;
  gint outsize;
  GstBuffer *outbuf;
  GstSegment *segment;
  GstClockTime duration;

  guint max_payload_size;
  gint frame_samples;

  g_mutex_lock (&enc->property_lock);

  bytes = enc->frame_samples * enc->n_channels * 2;
  max_payload_size = enc->max_payload_size;
  frame_samples = enc->frame_samples;

  g_mutex_unlock (&enc->property_lock);

  if (G_LIKELY (buf)) {
    gst_buffer_map (buf, &map, GST_MAP_READ);
    bdata = map.data;
    bsize = map.size;

    if (G_UNLIKELY (bsize % bytes)) {
      GST_DEBUG_OBJECT (enc, "draining; adding silence samples");

      /* If encoding part of a frame, and we have no set stop time on
       * the output segment, we update the segment stop time to reflect
       * the last sample. This will let oggmux set the last page's
       * granpos to tell a decoder the dummy samples should be clipped.
       */
      segment = &GST_AUDIO_ENCODER_OUTPUT_SEGMENT (enc);
      if (!GST_CLOCK_TIME_IS_VALID (segment->stop)) {
        int input_samples = bsize / (enc->n_channels * 2);
        GST_DEBUG_OBJECT (enc,
            "No stop time and partial frame, updating segment");
        duration =
            gst_util_uint64_scale (enc->encoded_samples + input_samples,
            GST_SECOND, enc->sample_rate);
        segment->stop = segment->start + duration;
        GST_DEBUG_OBJECT (enc, "new output segment %" GST_SEGMENT_FORMAT,
            segment);
        gst_pad_push_event (GST_AUDIO_ENCODER_SRC_PAD (enc),
            gst_event_new_segment (segment));
      }

      size = ((bsize / bytes) + 1) * bytes;
      mdata = g_malloc0 (size);
      memcpy (mdata, bdata, bsize);
      data = mdata;
    } else {
      data = bdata;
      size = bsize;
    }
  } else {
    GST_DEBUG_OBJECT (enc, "nothing to drain");
    goto done;
  }

  g_assert (size == bytes);

  outbuf =
      gst_audio_encoder_allocate_output_buffer (GST_AUDIO_ENCODER (enc),
      max_payload_size * enc->n_channels);
  if (!outbuf)
    goto done;

  GST_DEBUG_OBJECT (enc, "encoding %d samples (%d bytes)",
      frame_samples, (int) bytes);

  gst_buffer_map (outbuf, &omap, GST_MAP_WRITE);

  GST_DEBUG_OBJECT (enc, "encoding %d samples (%d bytes)",
      frame_samples, (int) bytes);

  outsize =
      opus_multistream_encode (enc->state, (const gint16 *) data,
      frame_samples, omap.data, max_payload_size * enc->n_channels);

  gst_buffer_unmap (outbuf, &omap);

  if (outsize < 0) {
    GST_ERROR_OBJECT (enc, "Encoding failed: %d", outsize);
    ret = GST_FLOW_ERROR;
    goto done;
  } else if (outsize > max_payload_size) {
    GST_WARNING_OBJECT (enc,
        "Encoded size %d is higher than max payload size (%d bytes)",
        outsize, max_payload_size);
    ret = GST_FLOW_ERROR;
    goto done;
  }

  GST_DEBUG_OBJECT (enc, "Output packet is %u bytes", outsize);
  gst_buffer_set_size (outbuf, outsize);

  ret =
      gst_audio_encoder_finish_frame (GST_AUDIO_ENCODER (enc), outbuf,
      frame_samples);
  enc->encoded_samples += frame_samples;

done:

  if (bdata)
    gst_buffer_unmap (buf, &map);

  if (mdata)
    g_free (mdata);

  return ret;
}

static GstFlowReturn
gst_opus_enc_handle_frame (GstAudioEncoder * benc, GstBuffer * buf)
{
  GstOpusEnc *enc;
  GstFlowReturn ret = GST_FLOW_OK;

  enc = GST_OPUS_ENC (benc);
  GST_DEBUG_OBJECT (enc, "handle_frame");
  GST_DEBUG_OBJECT (enc, "received buffer %p of %" G_GSIZE_FORMAT " bytes", buf,
      buf ? gst_buffer_get_size (buf) : 0);

  ret = gst_opus_enc_encode (enc, buf);

  return ret;
}

static void
gst_opus_enc_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstOpusEnc *enc;

  enc = GST_OPUS_ENC (object);

  g_mutex_lock (&enc->property_lock);

  switch (prop_id) {
    case PROP_AUDIO:
      g_value_set_boolean (value,
          enc->audio_type == OPUS_APPLICATION_AUDIO ? TRUE : FALSE);
      break;
    case PROP_AUDIO_TYPE:
      g_value_set_enum (value, enc->audio_type);
      break;
    case PROP_BITRATE:
      g_value_set_int (value, enc->bitrate);
      break;
    case PROP_BANDWIDTH:
      g_value_set_enum (value, enc->bandwidth);
      break;
    case PROP_FRAME_SIZE:
      g_value_set_enum (value, enc->frame_size);
      break;
    case PROP_CBR:
      GST_WARNING_OBJECT (enc,
          "cbr property is deprecated; use bitrate-type instead");
      g_value_set_boolean (value, enc->bitrate_type == BITRATE_TYPE_CBR);
      break;
    case PROP_CONSTRAINED_VBR:
      GST_WARNING_OBJECT (enc,
          "constrained-vbr property is deprecated; use bitrate-type instead");
      g_value_set_boolean (value,
          enc->bitrate_type == BITRATE_TYPE_CONSTRAINED_VBR);
      break;
    case PROP_BITRATE_TYPE:
      g_value_set_enum (value, enc->bitrate_type);
      break;
    case PROP_COMPLEXITY:
      g_value_set_int (value, enc->complexity);
      break;
    case PROP_INBAND_FEC:
      g_value_set_boolean (value, enc->inband_fec);
      break;
    case PROP_DTX:
      g_value_set_boolean (value, enc->dtx);
      break;
    case PROP_PACKET_LOSS_PERCENT:
      g_value_set_int (value, enc->packet_loss_percentage);
      break;
    case PROP_MAX_PAYLOAD_SIZE:
      g_value_set_uint (value, enc->max_payload_size);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

  g_mutex_unlock (&enc->property_lock);
}

static void
gst_opus_enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstOpusEnc *enc;

  enc = GST_OPUS_ENC (object);

#define GST_OPUS_UPDATE_PROPERTY(prop,type,ctl) do { \
  g_mutex_lock (&enc->property_lock); \
  enc->prop = g_value_get_##type (value); \
  if (enc->state) { \
    opus_multistream_encoder_ctl (enc->state, OPUS_SET_##ctl (enc->prop)); \
  } \
  g_mutex_unlock (&enc->property_lock); \
} while(0)

  switch (prop_id) {
    case PROP_AUDIO:
      enc->audio_type =
          g_value_get_boolean (value) ? OPUS_APPLICATION_AUDIO :
          OPUS_APPLICATION_VOIP;
      break;
    case PROP_AUDIO_TYPE:
      enc->audio_type = g_value_get_enum (value);
      break;
    case PROP_BITRATE:
      GST_OPUS_UPDATE_PROPERTY (bitrate, int, BITRATE);
      break;
    case PROP_BANDWIDTH:
      GST_OPUS_UPDATE_PROPERTY (bandwidth, enum, BANDWIDTH);
      break;
    case PROP_FRAME_SIZE:
      g_mutex_lock (&enc->property_lock);
      enc->frame_size = g_value_get_enum (value);
      enc->frame_samples = gst_opus_enc_get_frame_samples (enc);
      gst_opus_enc_setup_base_class (enc, GST_AUDIO_ENCODER (enc));
      g_mutex_unlock (&enc->property_lock);
      break;
    case PROP_CBR:
      GST_WARNING_OBJECT (enc,
          "cbr property is deprecated; use bitrate-type instead");
      g_warning ("cbr property is deprecated; use bitrate-type instead");
      g_mutex_lock (&enc->property_lock);
      enc->bitrate_type = BITRATE_TYPE_CBR;
      if (enc->state) {
        opus_multistream_encoder_ctl (enc->state, OPUS_SET_VBR (FALSE));
        opus_multistream_encoder_ctl (enc->state,
            OPUS_SET_VBR_CONSTRAINT (FALSE), 0);
      }
      g_mutex_unlock (&enc->property_lock);
      break;
    case PROP_CONSTRAINED_VBR:
      GST_WARNING_OBJECT (enc,
          "constrained-vbr property is deprecated; use bitrate-type instead");
      g_warning
          ("constrained-vbr property is deprecated; use bitrate-type instead");
      g_mutex_lock (&enc->property_lock);
      enc->bitrate_type = BITRATE_TYPE_CONSTRAINED_VBR;
      if (enc->state) {
        opus_multistream_encoder_ctl (enc->state, OPUS_SET_VBR (TRUE));
        opus_multistream_encoder_ctl (enc->state,
            OPUS_SET_VBR_CONSTRAINT (TRUE), 0);
      }
      g_mutex_unlock (&enc->property_lock);
      break;
    case PROP_BITRATE_TYPE:
      /* this one has an opposite meaning to the opus ctl... */
      g_mutex_lock (&enc->property_lock);
      enc->bitrate_type = g_value_get_enum (value);
      if (enc->state) {
        opus_multistream_encoder_ctl (enc->state,
            OPUS_SET_VBR (enc->bitrate_type != BITRATE_TYPE_CBR));
        opus_multistream_encoder_ctl (enc->state,
            OPUS_SET_VBR_CONSTRAINT (enc->bitrate_type ==
                BITRATE_TYPE_CONSTRAINED_VBR), 0);
      }
      g_mutex_unlock (&enc->property_lock);
      break;
    case PROP_COMPLEXITY:
      GST_OPUS_UPDATE_PROPERTY (complexity, int, COMPLEXITY);
      break;
    case PROP_INBAND_FEC:
      GST_OPUS_UPDATE_PROPERTY (inband_fec, boolean, INBAND_FEC);
      break;
    case PROP_DTX:
      GST_OPUS_UPDATE_PROPERTY (dtx, boolean, DTX);
      break;
    case PROP_PACKET_LOSS_PERCENT:
      GST_OPUS_UPDATE_PROPERTY (packet_loss_percentage, int, PACKET_LOSS_PERC);
      break;
    case PROP_MAX_PAYLOAD_SIZE:
      g_mutex_lock (&enc->property_lock);
      enc->max_payload_size = g_value_get_uint (value);
      g_mutex_unlock (&enc->property_lock);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

#undef GST_OPUS_UPDATE_PROPERTY

}
