/* GStreamer GL Library
 * Copyright (C) 2018 GStreamer developers
 *
 * gl-prelude.h: prelude include header for gst-gl library
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

#ifndef __GST_GL_PRELUDE_H__
#define __GST_GL_PRELUDE_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>

#ifdef BUILDING_GST_GL
#define GST_GL_API GST_API_EXPORT         /* from config.h */
#else
#define GST_GL_API GST_API_IMPORT
#endif

#ifndef GST_DISABLE_DEPRECATED
#define GST_GL_DEPRECATED GST_GL_API
#define GST_GL_DEPRECATED_FOR(f) GST_GL_API
#else
#define GST_GL_DEPRECATED G_DEPRECATED GST_GL_API
#define GST_GL_DEPRECATED_FOR(f) G_DEPRECATED_FOR(f) GST_GL_API
#endif

#endif /* __GST_GL_PRELUDE_H__ */
