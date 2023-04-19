/* GStreamer
 * Copyright (C) <2009> Sebastian Dröge <sebastian.droege@collabora.co.uk>
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


#ifndef __GST_RAW_CAPS_H__
#define __GST_RAW_CAPS_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define DEFAULT_RAW_CAPS \
    "video/x-raw(ANY); " \
    "audio/x-raw(ANY); " \
    "text/x-raw(ANY); " \
    "subpicture/x-dvd; " \
    "subpicture/x-dvb; " \
    "subpicture/x-xsub; " \
    "subpicture/x-pgs; " \
    "closedcaption/x-cea-608; " \
    "closedcaption/x-cea-708; " \
    "application/x-onvif-metadata; "

#define AUDIO_PASSTHROUGH_CAPS \
    "audio/x-ac3, framed = (boolean) true;" \
    "audio/x-eac3, framed = (boolean) true, alignment = (string) iec61937; "\
    "audio/x-dts, framed = (boolean) true, " \
      "block-size = (int) { 512, 1024, 2048 }; " \
    "audio/mpeg, mpegversion = (int) 1, " \
      "mpegaudioversion = (int) [ 1, 3 ], parsed = (boolean) true;"

#define RAW_AND_PASSTHROUGH_CAPS \
        "audio/x-ac3, framed = (boolean) true;" \
        "audio/x-eac3, framed = (boolean) true, alignment = (string) iec61937; "\
        "audio/x-dts, framed = (boolean) true, " \
          "block-size = (int) { 512, 1024, 2048 }; " \
        "audio/mpeg, mpegversion = (int) 1, " \
          "mpegaudioversion = (int) [ 1, 3 ], parsed = (boolean) true; "\
        "video/x-raw(ANY); " \
        "audio/x-raw(ANY); " \
        "text/x-raw(ANY); " \
        "subpicture/x-dvd; " \
        "subpicture/x-dvb; " \
        "subpicture/x-xsub; " \
        "subpicture/x-pgs; " \
        "closedcaption/x-cea-608; " \
        "closedcaption/x-cea-708; " \
        "application/x-onvif-metadata; "

G_END_DECLS

#endif /* __GST_RAW_CAPS__ */
