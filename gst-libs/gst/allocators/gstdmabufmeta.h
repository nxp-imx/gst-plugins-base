/*
 * GStreamer
 * Copyright (C) 2017 Intel Corporation
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

#ifndef __GST_DMABUF_META_H__
#define __GST_DMABUF_META_H__

#include <gst/gststructure.h>
#include <gst/gstmeta.h>
#include <gst/gstquery.h>
#include <gst/gstvalue.h>
#include <gst/allocators/allocators-prelude.h>

G_BEGIN_DECLS

#define GST_DMABUF_META_API_TYPE (gst_dmabuf_meta_api_get_type())
#define GST_DMABUF_META_INFO     (gst_dmabuf_meta_get_info())
typedef struct _GstDmabufMeta GstDmabufMeta;

/**
 * GstDmabufMeta:
 * @parent: the parent #GstMeta
 * @modifier: DRM modifier
 */
struct _GstDmabufMeta
{
  GstMeta parent;

  guint64 drm_modifier;
};

GST_ALLOCATORS_API
GType gst_dmabuf_meta_api_get_type (void);
GST_ALLOCATORS_API
const GstMetaInfo *gst_dmabuf_meta_get_info (void);

#define gst_buffer_get_dmabuf_meta(b) ((GstDmabufMeta*)gst_buffer_get_meta((b),GST_DMABUF_META_API_TYPE))

GST_ALLOCATORS_API
GstDmabufMeta * gst_buffer_add_dmabuf_meta (GstBuffer * buffer, guint64 drm_modifier);

GST_ALLOCATORS_API
void gst_query_add_allocation_dmabuf_meta (GstQuery * query, guint64 drm_modifier);

G_END_DECLS
#endif /* __GST_DMABUF_META_H__ */
