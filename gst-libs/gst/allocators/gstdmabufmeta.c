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

/**
 * SECTION:gstdmabufmeta
 * @short_description: dmabuf metadata
 * @see_also: #GstDmaBufAllocator
 *
 * #GstDmabufMeta carries metadata that goes along with
 * dmabuf memory in the buffer, like drm modifier.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstdmabufmeta.h"

/**
 * gst_buffer_add_dmabuf_meta:
 * @buffer: a #GstBuffer
 * @modifier: the drm modifier
 *
 * Returns: (transfer none): the #GstDmabufMeta added to #GstBuffer
 *
 * Since: 1.12
 */
GstDmabufMeta *
gst_buffer_add_dmabuf_meta (GstBuffer * buffer, guint64 drm_modifier)
{
  GstDmabufMeta *meta;

  meta =
      (GstDmabufMeta *) gst_buffer_add_meta ((buffer),
      GST_DMABUF_META_INFO, NULL);

  if (!meta)
    return NULL;

  meta->drm_modifier = drm_modifier;

  return meta;
}

static gboolean
gst_dmabuf_meta_transform (GstBuffer * dest, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
  GstDmabufMeta *dmeta, *smeta;

  smeta = (GstDmabufMeta *) meta;

  if (GST_META_TRANSFORM_IS_COPY (type)) {
    GstMetaTransformCopy *copy = data;

    if (!copy->region) {
      /* only copy if the complete data is copied as well */
      dmeta = gst_buffer_add_dmabuf_meta (dest, smeta->drm_modifier);
      if (!dmeta)
        return FALSE;
    }
  } else {
    /* return FALSE, if transform type is not supported */
    return FALSE;
  }

  return TRUE;
}

static void
gst_dmabuf_meta_free (GstMeta * meta, GstBuffer * buffer)
{
  ((GstDmabufMeta *) meta)->drm_modifier = 0;

  return;
}

static gboolean
gst_dmabuf_meta_init (GstMeta * meta, gpointer params, GstBuffer * buffer)
{
  ((GstDmabufMeta *) meta)->drm_modifier = 0;

  return TRUE;
}

GType
gst_dmabuf_meta_api_get_type (void)
{
  static GType type = 0;
  static const gchar *tags[] = { NULL };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("GstDmabufMetaAPI", tags);
    g_once_init_leave (&type, _type);
  }

  return type;
}

const GstMetaInfo *
gst_dmabuf_meta_get_info (void)
{
  static const GstMetaInfo *meta_info = NULL;

  if (g_once_init_enter (&meta_info)) {
    const GstMetaInfo *meta = gst_meta_register (GST_DMABUF_META_API_TYPE,
        "GstDmabufMeta",
        sizeof (GstDmabufMeta), gst_dmabuf_meta_init,
        gst_dmabuf_meta_free,
        gst_dmabuf_meta_transform);
    g_once_init_leave (&meta_info, meta);
  }

  return meta_info;
}

void
gst_query_add_allocation_dmabuf_meta (GstQuery * query, guint64 drm_modifier)
{
  guint index;
  GstStructure *params;

  if (!gst_query_find_allocation_meta (query, GST_DMABUF_META_API_TYPE, &index)) {
    gchar *str =
        g_strdup_printf ("GstDmabufMeta, dmabuf.drm_modifier=(guint64){ %"
        G_GUINT64_FORMAT " };", drm_modifier);

    params = gst_structure_new_from_string (str);
    g_free (str);

    gst_query_add_allocation_meta (query, GST_DMABUF_META_API_TYPE, params);
    gst_structure_free (params);
  } else {
    GValue newlist = G_VALUE_INIT, drm_modifier_value = G_VALUE_INIT;

    gst_query_parse_nth_allocation_meta (query, index,
        (const GstStructure **) &params);
    g_value_init (&drm_modifier_value, G_TYPE_UINT64);
    g_value_set_uint64 (&drm_modifier_value, drm_modifier);
    gst_value_list_merge (&newlist, gst_structure_get_value (params,
            "dmabuf.drm_modifier"), &drm_modifier_value);
    gst_structure_take_value (params, "dmabuf.drm_modifier", &newlist);
  }
}
