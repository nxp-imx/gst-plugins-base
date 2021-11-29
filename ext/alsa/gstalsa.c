/* Copyright (C) 2006 Tim-Philipp Müller <tim centricular net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "gstalsa.h"

#include <gst/audio/audio.h>

static GstCaps *
gst_alsa_detect_rates (GstObject * obj, snd_pcm_hw_params_t * hw_params,
    GstCaps * in_caps)
{
  GstCaps *caps;
  guint min, max;
  gint err, dir, min_rate, max_rate, i;

  GST_LOG_OBJECT (obj, "probing sample rates ...");

  if ((err = snd_pcm_hw_params_get_rate_min (hw_params, &min, &dir)) < 0)
    goto min_rate_err;

  if ((err = snd_pcm_hw_params_get_rate_max (hw_params, &max, &dir)) < 0)
    goto max_rate_err;

  min_rate = min;
  max_rate = max;

  if (min_rate < 4000)
    min_rate = 4000;            /* random 'sensible minimum' */

  if (max_rate <= 0)
    max_rate = G_MAXINT;        /* or maybe just use 192400 or so? */
  else if (max_rate > 0 && max_rate < 4000)
    max_rate = MAX (4000, min_rate);

  GST_DEBUG_OBJECT (obj, "Min. rate = %u (%d)", min_rate, min);
  GST_DEBUG_OBJECT (obj, "Max. rate = %u (%d)", max_rate, max);

  caps = gst_caps_make_writable (in_caps);

  for (i = 0; i < gst_caps_get_size (caps); ++i) {
    GstStructure *s;

    s = gst_caps_get_structure (caps, i);
    if (min_rate == max_rate) {
      gst_structure_set (s, "rate", G_TYPE_INT, min_rate, NULL);
    } else {
      gst_structure_set (s, "rate", GST_TYPE_INT_RANGE,
          min_rate, max_rate, NULL);
    }
  }

  return caps;

  /* ERRORS */
min_rate_err:
  {
    GST_ERROR_OBJECT (obj, "failed to query minimum sample rate: %s",
        snd_strerror (err));
    gst_caps_unref (in_caps);
    return NULL;
  }
max_rate_err:
  {
    GST_ERROR_OBJECT (obj, "failed to query maximum sample rate: %s",
        snd_strerror (err));
    gst_caps_unref (in_caps);
    return NULL;
  }
}

static snd_pcm_format_t
gst_alsa_get_pcm_format (GstAudioFormat fmt)
{
  switch (fmt) {
    case GST_AUDIO_FORMAT_S8:
      return SND_PCM_FORMAT_S8;
    case GST_AUDIO_FORMAT_U8:
      return SND_PCM_FORMAT_U8;
      /* 16 bit */
    case GST_AUDIO_FORMAT_S16LE:
      return SND_PCM_FORMAT_S16_LE;
    case GST_AUDIO_FORMAT_S16BE:
      return SND_PCM_FORMAT_S16_BE;
    case GST_AUDIO_FORMAT_U16LE:
      return SND_PCM_FORMAT_U16_LE;
    case GST_AUDIO_FORMAT_U16BE:
      return SND_PCM_FORMAT_U16_BE;
      /* 24 bit in low 3 bytes of 32 bits */
    case GST_AUDIO_FORMAT_S24_32LE:
      return SND_PCM_FORMAT_S24_LE;
    case GST_AUDIO_FORMAT_S24_32BE:
      return SND_PCM_FORMAT_S24_BE;
    case GST_AUDIO_FORMAT_U24_32LE:
      return SND_PCM_FORMAT_U24_LE;
    case GST_AUDIO_FORMAT_U24_32BE:
      return SND_PCM_FORMAT_U24_BE;
      /* 24 bit in 3 bytes */
    case GST_AUDIO_FORMAT_S24LE:
      return SND_PCM_FORMAT_S24_3LE;
    case GST_AUDIO_FORMAT_S24BE:
      return SND_PCM_FORMAT_S24_3BE;
    case GST_AUDIO_FORMAT_U24LE:
      return SND_PCM_FORMAT_U24_3LE;
    case GST_AUDIO_FORMAT_U24BE:
      return SND_PCM_FORMAT_U24_3BE;
      /* 32 bit */
    case GST_AUDIO_FORMAT_S32LE:
      return SND_PCM_FORMAT_S32_LE;
    case GST_AUDIO_FORMAT_S32BE:
      return SND_PCM_FORMAT_S32_BE;
    case GST_AUDIO_FORMAT_U32LE:
      return SND_PCM_FORMAT_U32_LE;
    case GST_AUDIO_FORMAT_U32BE:
      return SND_PCM_FORMAT_U32_BE;
    case GST_AUDIO_FORMAT_F32LE:
      return SND_PCM_FORMAT_FLOAT_LE;
    case GST_AUDIO_FORMAT_F32BE:
      return SND_PCM_FORMAT_FLOAT_BE;
    case GST_AUDIO_FORMAT_F64LE:
      return SND_PCM_FORMAT_FLOAT64_LE;
    case GST_AUDIO_FORMAT_F64BE:
      return SND_PCM_FORMAT_FLOAT64_BE;
    case GST_AUDIO_FORMAT_IEC958_SUBFRAME_LE:
      return SND_PCM_FORMAT_IEC958_SUBFRAME_LE;
    default:
      break;
  }
  return SND_PCM_FORMAT_UNKNOWN;
}

static gboolean
format_supported (const GValue * format_val, snd_pcm_format_mask_t * mask,
    int endianness)
{
  const GstAudioFormatInfo *finfo;
  snd_pcm_format_t pcm_format;
  GstAudioFormat format;

  if (!G_VALUE_HOLDS_STRING (format_val))
    return FALSE;

  format = gst_audio_format_from_string (g_value_get_string (format_val));
  if (format == GST_AUDIO_FORMAT_UNKNOWN)
    return FALSE;

  finfo = gst_audio_format_get_info (format);

  if (GST_AUDIO_FORMAT_INFO_ENDIANNESS (finfo) != endianness
      && GST_AUDIO_FORMAT_INFO_ENDIANNESS (finfo) != 0)
    return FALSE;

  pcm_format = gst_alsa_get_pcm_format (format);
  if (pcm_format == SND_PCM_FORMAT_UNKNOWN)
    return FALSE;

  return snd_pcm_format_mask_test (mask, pcm_format);
}

static GstCaps *
gst_alsa_detect_formats (GstObject * obj, snd_pcm_hw_params_t * hw_params,
    GstCaps * in_caps, int endianness)
{
  snd_pcm_format_mask_t *mask;
  GstStructure *s;
  GstCaps *caps;
  gint i;

  snd_pcm_format_mask_malloc (&mask);
  snd_pcm_hw_params_get_format_mask (hw_params, mask);

  caps = NULL;

  for (i = 0; i < gst_caps_get_size (in_caps); ++i) {
    const GValue *format;
    GValue list = G_VALUE_INIT;

    gboolean is_iec958_format = FALSE;
    s = gst_caps_get_structure (in_caps, i);
    if (!gst_structure_has_name (s, "audio/x-raw")) {
      if (gst_structure_has_name (s, "audio/x-iec958")) {
        is_iec958_format = TRUE;
        GST_DEBUG_OBJECT (obj, "audio/x-iec958");
      } else {
        GST_DEBUG_OBJECT (obj, "skipping non-raw format or iec958 format");
        continue;
      }
    }

    format = gst_structure_get_value (s, "format");
    if (format == NULL)
      continue;

    g_value_init (&list, GST_TYPE_LIST);

    if (GST_VALUE_HOLDS_LIST (format)) {
      gint i, len;

      len = gst_value_list_get_size (format);
      for (i = 0; i < len; i++) {
        const GValue *val;

        val = gst_value_list_get_value (format, i);
        if (format_supported (val, mask, endianness)) {
          gst_value_list_append_value (&list, val);
        } else {
          if (is_iec958_format) {
            if (snd_pcm_format_mask_test (mask,
                    SND_PCM_FORMAT_IEC958_SUBFRAME_LE)) {
              gst_value_list_append_value (&list, val);
            }
          }
        }
      }
    } else if (G_VALUE_HOLDS_STRING (format)) {
      if (format_supported (format, mask, endianness)) {
        gst_value_list_append_value (&list, format);
      } else {
        if (is_iec958_format) {
          if (snd_pcm_format_mask_test (mask,
                  SND_PCM_FORMAT_IEC958_SUBFRAME_LE)) {
            gst_value_list_append_value (&list, format);
          }
        }
      }
    }

    if (gst_value_list_get_size (&list) > 1) {
      if (caps == NULL)
        caps = gst_caps_new_empty ();
      s = gst_structure_copy (s);
      gst_structure_take_value (s, "format", &list);
      gst_caps_append_structure (caps, s);
    } else if (gst_value_list_get_size (&list) == 1) {
      if (caps == NULL)
        caps = gst_caps_new_empty ();
      format = gst_value_list_get_value (&list, 0);
      s = gst_structure_copy (s);
      gst_structure_set_value (s, "format", format);
      gst_caps_append_structure (caps, s);
      g_value_unset (&list);
    } else {
      g_value_unset (&list);
    }
  }

  snd_pcm_format_mask_free (mask);
  gst_caps_unref (in_caps);
  return caps;
}

/* we don't have channel mappings for more than this many channels */
#define GST_ALSA_MAX_CHANNELS 8

static GstStructure *
get_channel_free_structure (const GstStructure * in_structure)
{
  GstStructure *s = gst_structure_copy (in_structure);

  gst_structure_remove_field (s, "channels");
  return s;
}

#define ONE_64 G_GUINT64_CONSTANT (1)
#define CHANNEL_MASK_STEREO ((ONE_64<<GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT) | (ONE_64<<GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT))
#define CHANNEL_MASK_2_1    (CHANNEL_MASK_STEREO | (ONE_64<<GST_AUDIO_CHANNEL_POSITION_LFE1))
#define CHANNEL_MASK_4_0    (CHANNEL_MASK_STEREO | (ONE_64<<GST_AUDIO_CHANNEL_POSITION_REAR_LEFT) | (ONE_64<<GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT))
#define CHANNEL_MASK_5_1    (CHANNEL_MASK_4_0 | (ONE_64<<GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER) | (ONE_64<<GST_AUDIO_CHANNEL_POSITION_LFE1))
#define CHANNEL_MASK_7_1    (CHANNEL_MASK_5_1 | (ONE_64<<GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT) | (ONE_64<<GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT))

static GstCaps *
caps_add_channel_configuration (GstCaps * caps,
    const GstStructure * in_structure, gint min_chans, gint max_chans)
{
  GstStructure *s = NULL;
  gint c;

  if (min_chans == max_chans && max_chans == 1) {
    s = get_channel_free_structure (in_structure);
    gst_structure_set (s, "channels", G_TYPE_INT, 1, NULL);
    caps = gst_caps_merge_structure (caps, s);
    return caps;
  }

  g_assert (min_chans >= 1);

  /* mono and stereo don't need channel configurations */
  if (min_chans == 2) {
    s = get_channel_free_structure (in_structure);
    gst_structure_set (s, "channels", G_TYPE_INT, 2, "channel-mask",
        GST_TYPE_BITMASK, CHANNEL_MASK_STEREO, NULL);
    caps = gst_caps_merge_structure (caps, s);
  } else if (min_chans == 1 && max_chans >= 2) {
    s = get_channel_free_structure (in_structure);
    gst_structure_set (s, "channels", G_TYPE_INT, 2, "channel-mask",
        GST_TYPE_BITMASK, CHANNEL_MASK_STEREO, NULL);
    caps = gst_caps_merge_structure (caps, s);
    s = get_channel_free_structure (in_structure);
    gst_structure_set (s, "channels", G_TYPE_INT, 1, NULL);
    caps = gst_caps_merge_structure (caps, s);
  }

  /* don't know whether to use 2.1 or 3.0 here - but I suspect
   * alsa might work around that/fix it somehow. Can we tell alsa
   * what our channel layout is like? */
  if (max_chans >= 3 && min_chans <= 3) {
    s = get_channel_free_structure (in_structure);
    gst_structure_set (s, "channels", G_TYPE_INT, 3, "channel-mask",
        GST_TYPE_BITMASK, CHANNEL_MASK_2_1, NULL);
    caps = gst_caps_merge_structure (caps, s);
  }

  /* everything else (4, 6, 8 channels) needs a channel layout */
  for (c = MAX (4, min_chans); c <= 8; c += 2) {
    if (max_chans >= c) {
      guint64 channel_mask;

      s = get_channel_free_structure (in_structure);
      switch (c) {
        case 4:
          channel_mask = CHANNEL_MASK_4_0;
          break;
        case 6:
          channel_mask = CHANNEL_MASK_5_1;
          break;
        case 8:
          channel_mask = CHANNEL_MASK_7_1;
          break;
        default:
          channel_mask = 0;
          g_assert_not_reached ();
          break;
      }
      gst_structure_set (s, "channels", G_TYPE_INT, c, "channel-mask",
          GST_TYPE_BITMASK, channel_mask, NULL);
      caps = gst_caps_merge_structure (caps, s);
    }
  }

  /* NONE layouts for everything else */
  for (c = MAX (9, min_chans); c <= max_chans; ++c) {
    s = get_channel_free_structure (in_structure);
    gst_structure_set (s, "channels", G_TYPE_INT, c, "channel-mask",
        GST_TYPE_BITMASK, G_GUINT64_CONSTANT (0), NULL);
    caps = gst_caps_merge_structure (caps, s);
  }
  return caps;
}

static GstCaps *
gst_alsa_detect_channels (GstObject * obj, snd_pcm_hw_params_t * hw_params,
    GstCaps * in_caps)
{
  GstCaps *caps;
  guint min, max;
  gint min_chans, max_chans;
  gint err, i;

  GST_LOG_OBJECT (obj, "probing channels ...");

  if ((err = snd_pcm_hw_params_get_channels_min (hw_params, &min)) < 0)
    goto min_chan_error;

  if ((err = snd_pcm_hw_params_get_channels_max (hw_params, &max)) < 0)
    goto max_chan_error;

  /* note: the above functions may return (guint) -1 */
  min_chans = min;
  max_chans = max;

  if (min_chans < 0) {
    min_chans = 1;
    max_chans = GST_ALSA_MAX_CHANNELS;
  } else if (max_chans < 0) {
    max_chans = GST_ALSA_MAX_CHANNELS;
  }

  if (min_chans > max_chans) {
    gint temp;

    GST_WARNING_OBJECT (obj, "minimum channels > maximum channels (%d > %d), "
        "please fix your soundcard drivers", min, max);
    temp = min_chans;
    min_chans = max_chans;
    max_chans = temp;
  }

  /* pro cards seem to return large numbers for min_channels */
  if (min_chans > GST_ALSA_MAX_CHANNELS) {
    GST_DEBUG_OBJECT (obj, "min_chans = %u, looks like a pro card", min_chans);
    if (max_chans < min_chans) {
      max_chans = min_chans;
    } else {
      /* only support [max_chans; max_chans] for these cards for now
       * to avoid inflating the source caps with loads of structures ... */
      min_chans = max_chans;
    }
  } else {
    min_chans = MAX (min_chans, 1);
    max_chans = MIN (GST_ALSA_MAX_CHANNELS, max_chans);
  }

  GST_DEBUG_OBJECT (obj, "Min. channels = %d (%d)", min_chans, min);
  GST_DEBUG_OBJECT (obj, "Max. channels = %d (%d)", max_chans, max);

  caps = gst_caps_new_empty ();

  for (i = 0; i < gst_caps_get_size (in_caps); ++i) {
    GstStructure *s;
    GType field_type;
    gint c_min = min_chans;
    gint c_max = max_chans;

    s = gst_caps_get_structure (in_caps, i);
    /* the template caps might limit the number of channels (like alsasrc),
     * in which case we don't want to return a superset, so hack around this
     * for the two common cases where the channels are either a fixed number
     * or a min/max range). Example: alsasrc template has channels = [1,2] and 
     * the detection will claim to support 8 channels for device 'plughw:0' */
    field_type = gst_structure_get_field_type (s, "channels");
    if (field_type == G_TYPE_INT) {
      gst_structure_get_int (s, "channels", &c_min);
      gst_structure_get_int (s, "channels", &c_max);
    } else if (field_type == GST_TYPE_INT_RANGE) {
      const GValue *val;

      val = gst_structure_get_value (s, "channels");
      c_min = CLAMP (gst_value_get_int_range_min (val), min_chans, max_chans);
      c_max = CLAMP (gst_value_get_int_range_max (val), min_chans, max_chans);
    } else {
      c_min = min_chans;
      c_max = max_chans;
    }

    caps = caps_add_channel_configuration (caps, s, c_min, c_max);
  }

  gst_caps_unref (in_caps);

  return caps;

  /* ERRORS */
min_chan_error:
  {
    GST_ERROR_OBJECT (obj, "failed to query minimum channel count: %s",
        snd_strerror (err));
    return NULL;
  }
max_chan_error:
  {
    GST_ERROR_OBJECT (obj, "failed to query maximum channel count: %s",
        snd_strerror (err));
    return NULL;
  }
}

snd_pcm_t *
gst_alsa_open_iec958_pcm (GstObject * obj, gchar * device)
{
  char *iec958_pcm_name = NULL;
  snd_pcm_t *pcm = NULL;
  int res;
  char devstr[256];             /* Storage for local 'default' device string */

  /*
   * Try and open our default iec958 device. Fall back to searching on card x
   * if this fails, which should only happen on older alsa setups
   */

  /* The string will be one of these:
   * SPDIF_CON: Non-audio flag not set:
   *    spdif:{AES0 0x0 AES1 0x82 AES2 0x0 AES3 0x2}
   * SPDIF_CON: Non-audio flag set:
   *    spdif:{AES0 0x2 AES1 0x82 AES2 0x0 AES3 0x2}
   */
/*  sprintf (devstr,
      "%s:{AES0 0x%02x AES1 0x%02x AES2 0x%02x AES3 0x%02x}",
      device,
      IEC958_AES0_CON_EMPHASIS_NONE | IEC958_AES0_NONAUDIO,
      IEC958_AES1_CON_ORIGINAL | IEC958_AES1_CON_PCM_CODER,
      0, IEC958_AES3_CON_FS_48000);*/

  sprintf (devstr, "%s", device);

  GST_DEBUG_OBJECT (obj, "Generated device string \"%s\"", devstr);
  iec958_pcm_name = devstr;

  res = snd_pcm_open (&pcm, iec958_pcm_name, SND_PCM_STREAM_PLAYBACK, 0);
  if (G_UNLIKELY (res < 0)) {
    GST_DEBUG_OBJECT (obj, "failed opening IEC958 device: %s",
        snd_strerror (res));
    pcm = NULL;
  }

  return pcm;
}


/*
 * gst_alsa_probe_supported_formats:
 *
 * Takes the template caps and returns the subset which is actually
 * supported by this device.
 *
 */

GstCaps *
gst_alsa_probe_supported_formats (GstObject * obj, gchar * device,
    snd_pcm_t * handle, const GstCaps * template_caps)
{
  snd_pcm_hw_params_t *hw_params;
  GstCaps *caps;
  gint err;

  snd_pcm_hw_params_malloc (&hw_params);
  if ((err = snd_pcm_hw_params_any (handle, hw_params)) < 0)
    goto error;

  caps = gst_alsa_detect_formats (obj, hw_params,
      gst_caps_copy (template_caps), G_BYTE_ORDER);

  /* if there are no formats in native endianness, try non-native as well */
  if (caps == NULL) {
    GST_INFO_OBJECT (obj, "no formats in native endianness detected");

    caps = gst_alsa_detect_formats (obj, hw_params,
        gst_caps_copy (template_caps),
        (G_BYTE_ORDER == G_LITTLE_ENDIAN) ? G_BIG_ENDIAN : G_LITTLE_ENDIAN);

    if (caps == NULL)
      goto subroutine_error;
  }

  if (!(caps = gst_alsa_detect_rates (obj, hw_params, caps)))
    goto subroutine_error;

  if (!(caps = gst_alsa_detect_channels (obj, hw_params, caps)))
    goto subroutine_error;

  snd_pcm_hw_params_free (hw_params);
  return caps;

  /* ERRORS */
error:
  {
    GST_ERROR_OBJECT (obj, "failed to query formats: %s", snd_strerror (err));
    snd_pcm_hw_params_free (hw_params);
    return NULL;
  }
subroutine_error:
  {
    GST_ERROR_OBJECT (obj, "failed to query formats");
    snd_pcm_hw_params_free (hw_params);
    gst_caps_replace (&caps, NULL);
    return NULL;
  }
}

/*
 * gst_alsa_probe_supported_formats:
 *
 * Takes the template caps and returns the subset which is actually
 * supported by this device.
 */
gboolean gst_alsa_iec958_formats_supported (GstObject * obj, gchar * device,
    snd_pcm_t ** handle)
{
  snd_pcm_stream_t stream_type;
  gboolean supported = FALSE;
  snd_pcm_t *pcm;
  gint err;

  /* Try opening IEC958 device to see if we can support that format (playback
   * only for now but we could add SPDIF capture later) */
  stream_type = snd_pcm_stream (*handle);
  if (stream_type == SND_PCM_STREAM_PLAYBACK) {
    /* Close the exiting device to re-open it as iec958 device*/
    err = snd_pcm_close(*handle);
    if (G_UNLIKELY (err < 0)) {
      GST_DEBUG_OBJECT (obj, " failed closing alsa device: %s",
        snd_strerror (err));
      return FALSE;
    }

    pcm = gst_alsa_open_iec958_pcm(GST_OBJECT (obj), device);
    if (G_LIKELY (pcm)) {
      supported = TRUE;
      snd_pcm_close (pcm);
    }

    /* Reopen device in raw audio mode*/
    err = snd_pcm_open (handle, device, SND_PCM_STREAM_PLAYBACK,
        SND_PCM_NONBLOCK);
    if (G_UNLIKELY (err < 0)) {
       GST_DEBUG_OBJECT (obj, "failed re-opening alsa device: %s",
           snd_strerror (err));
    }
    return supported;
  }

  GST_DEBUG_OBJECT (obj, "only supported for playback");
  return FALSE;

}

/* returns the card name when the device number is unknown or -1 */
static gchar *
gst_alsa_find_device_name_no_handle (GstObject * obj, const gchar * devcard,
    gint device_num, snd_pcm_stream_t stream)
{
  snd_ctl_card_info_t *info = NULL;
  snd_ctl_t *ctl = NULL;
  gchar *ret = NULL;
  gint dev = -1;

  GST_LOG_OBJECT (obj, "[%s] device=%d", devcard, device_num);

  if (snd_ctl_open (&ctl, devcard, 0) < 0)
    return NULL;

  snd_ctl_card_info_malloc (&info);
  if (snd_ctl_card_info (ctl, info) < 0)
    goto done;

  if (device_num != -1) {
    while (snd_ctl_pcm_next_device (ctl, &dev) == 0 && dev >= 0) {
      if (dev == device_num) {
        snd_pcm_info_t *pcminfo;

        snd_pcm_info_malloc (&pcminfo);
        snd_pcm_info_set_device (pcminfo, dev);
        snd_pcm_info_set_subdevice (pcminfo, 0);
        snd_pcm_info_set_stream (pcminfo, stream);
        if (snd_ctl_pcm_info (ctl, pcminfo) < 0) {
          snd_pcm_info_free (pcminfo);
          break;
        }

        ret = (gchar *) snd_pcm_info_get_name (pcminfo);
        if (ret) {
          ret = g_strdup (ret);
          GST_LOG_OBJECT (obj, "name from pcminfo: %s", ret);
        }
        snd_pcm_info_free (pcminfo);
        if (ret)
          break;
      }
    }
  }

  if (ret == NULL) {
    char *name = NULL;
    gint card;

    GST_LOG_OBJECT (obj, "trying card name");
    card = snd_ctl_card_info_get_card (info);
    snd_card_get_name (card, &name);
    ret = g_strdup (name);
    free (name);
  }

done:
  snd_ctl_card_info_free (info);
  snd_ctl_close (ctl);

  return ret;
}

gchar *
gst_alsa_find_card_name (GstObject * obj, const gchar * devcard,
    snd_pcm_stream_t stream)
{
  return gst_alsa_find_device_name_no_handle (obj, devcard, -1, stream);
}

gchar *
gst_alsa_find_device_name (GstObject * obj, const gchar * device,
    snd_pcm_t * handle, snd_pcm_stream_t stream)
{
  gchar *ret = NULL;

  if (device != NULL) {
    gchar *dev, *comma;
    gint devnum;

    GST_LOG_OBJECT (obj, "Trying to get device name from string '%s'", device);

    /* only want name:card bit, but not devices and subdevices */
    dev = g_strdup (device);
    if ((comma = strchr (dev, ','))) {
      *comma = '\0';
      devnum = atoi (comma + 1);
      ret = gst_alsa_find_device_name_no_handle (obj, dev, devnum, stream);
    }
    g_free (dev);
  }

  if (ret == NULL && handle != NULL) {
    snd_pcm_info_t *info;

    GST_LOG_OBJECT (obj, "Trying to get device name from open handle");
    snd_pcm_info_malloc (&info);
    snd_pcm_info (handle, info);
    ret = g_strdup (snd_pcm_info_get_name (info));
    snd_pcm_info_free (info);
  }

  GST_LOG_OBJECT (obj, "Device name for device '%s': %s",
      GST_STR_NULL (device), GST_STR_NULL (ret));

  return ret;
}

/* ALSA channel positions */
const GstAudioChannelPosition alsa_position[][8] = {
  {
      GST_AUDIO_CHANNEL_POSITION_MONO},
  {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
      GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT},
  {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
      GST_AUDIO_CHANNEL_POSITION_LFE1},
  {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
      GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT},
  {
        GST_AUDIO_CHANNEL_POSITION_INVALID,
        GST_AUDIO_CHANNEL_POSITION_INVALID,
        GST_AUDIO_CHANNEL_POSITION_INVALID,
        GST_AUDIO_CHANNEL_POSITION_INVALID,
        GST_AUDIO_CHANNEL_POSITION_INVALID,
        GST_AUDIO_CHANNEL_POSITION_INVALID,
        GST_AUDIO_CHANNEL_POSITION_INVALID,
      GST_AUDIO_CHANNEL_POSITION_INVALID},
  {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
      GST_AUDIO_CHANNEL_POSITION_LFE1},
  {
        GST_AUDIO_CHANNEL_POSITION_INVALID,
        GST_AUDIO_CHANNEL_POSITION_INVALID,
        GST_AUDIO_CHANNEL_POSITION_INVALID,
        GST_AUDIO_CHANNEL_POSITION_INVALID,
        GST_AUDIO_CHANNEL_POSITION_INVALID,
        GST_AUDIO_CHANNEL_POSITION_INVALID,
        GST_AUDIO_CHANNEL_POSITION_INVALID,
      GST_AUDIO_CHANNEL_POSITION_INVALID},
  {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_LFE1,
        GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
      GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT}
};

#ifdef SND_CHMAP_API_VERSION
/* +1 is to make zero as holes */
#define ITEM(x, y) \
  [SND_CHMAP_ ## x] = GST_AUDIO_CHANNEL_POSITION_ ## y + 1

static const GstAudioChannelPosition gst_pos[SND_CHMAP_LAST + 1] = {
  ITEM (MONO, MONO),
  ITEM (FL, FRONT_LEFT),
  ITEM (FR, FRONT_RIGHT),
  ITEM (FC, FRONT_CENTER),
  ITEM (RL, REAR_LEFT),
  ITEM (RR, REAR_RIGHT),
  ITEM (RC, REAR_CENTER),
  ITEM (LFE, LFE1),
  ITEM (SL, SIDE_LEFT),
  ITEM (SR, SIDE_RIGHT),
  ITEM (FLC, FRONT_LEFT_OF_CENTER),
  ITEM (FRC, FRONT_RIGHT_OF_CENTER),
  ITEM (FLW, WIDE_LEFT),
  ITEM (FRW, WIDE_RIGHT),
  ITEM (TC, TOP_CENTER),
  ITEM (TFL, TOP_FRONT_LEFT),
  ITEM (TFR, TOP_FRONT_RIGHT),
  ITEM (TFC, TOP_FRONT_CENTER),
  ITEM (TRL, TOP_REAR_LEFT),
  ITEM (TRR, TOP_REAR_RIGHT),
  ITEM (TRC, TOP_REAR_CENTER),
  ITEM (LLFE, LFE1),
  ITEM (RLFE, LFE2),
  ITEM (BC, BOTTOM_FRONT_CENTER),
  ITEM (BLC, BOTTOM_FRONT_LEFT),
  ITEM (BRC, BOTTOM_FRONT_LEFT),
};

#undef ITEM

gboolean
alsa_chmap_to_channel_positions (const snd_pcm_chmap_t * chmap,
    GstAudioChannelPosition * pos)
{
  int c;
  gboolean all_mono = TRUE;

  for (c = 0; c < chmap->channels; c++) {
    if (chmap->pos[c] > SND_CHMAP_LAST)
      return FALSE;
    pos[c] = gst_pos[chmap->pos[c]];
    if (!pos[c])
      return FALSE;
    pos[c]--;

    if (pos[c] != GST_AUDIO_CHANNEL_POSITION_MONO)
      all_mono = FALSE;
  }

  if (all_mono && chmap->channels > 1) {
    /* GST_AUDIO_CHANNEL_POSITION_MONO can only be used with 1 channel and
     * GST_AUDIO_CHANNEL_POSITION_NONE is meant to be used for position-less
     * multi channels.
     * Converting as ALSA can only express such configuration by using an array
     * full of SND_CHMAP_MONO.
     */
    for (c = 0; c < chmap->channels; c++)
      pos[c] = GST_AUDIO_CHANNEL_POSITION_NONE;
  }

  return TRUE;
}

void
alsa_detect_channels_mapping (GstObject * obj, snd_pcm_t * handle,
    GstAudioRingBufferSpec * spec, guint channels, GstAudioRingBuffer * buf)
{
  snd_pcm_chmap_t *chmap;
  GstAudioChannelPosition pos[8];

  if (spec->type != GST_AUDIO_RING_BUFFER_FORMAT_TYPE_RAW || channels >= 9)
    return;

  chmap = snd_pcm_get_chmap (handle);
  if (!chmap) {
    GST_LOG_OBJECT (obj, "ALSA driver does not implement channels mapping API");
    return;
  }

  if (chmap->channels != channels) {
    GST_LOG_OBJECT (obj,
        "got channels mapping for %u channels but stream has %u channels; ignoring",
        chmap->channels, channels);
    goto out;
  }

  if (alsa_chmap_to_channel_positions (chmap, pos)) {
#ifndef GST_DISABLE_GST_DEBUG
    {
      gchar *tmp = gst_audio_channel_positions_to_string (pos, channels);

      GST_LOG_OBJECT (obj, "got channels mapping %s", tmp);
      g_free (tmp);
    }
#endif /* GST_DISABLE_GST_DEBUG */

    gst_audio_ring_buffer_set_channel_positions (buf, pos);
  } else {
    GST_LOG_OBJECT (obj, "failed to convert ALSA channels mapping");
  }

out:
  free (chmap);
}
#endif /* SND_CHMAP_API_VERSION */
