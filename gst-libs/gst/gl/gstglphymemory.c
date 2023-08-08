/*
 * GStreamer
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
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

#include "gstglphymemory.h"
#include "gstglfuncs.h"
#include <g2d.h>

GST_DEBUG_CATEGORY_STATIC (GST_CAT_GL_PHY_MEMORY);
#define GST_CAT_DEFAULT GST_CAT_GL_PHY_MEMORY

#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT                                             0x80E1
#endif
#ifndef GL_VIV_YV12
#define GL_VIV_YV12                                             0x8FC0
#endif
#ifndef GL_VIV_NV12
#define GL_VIV_NV12                                             0x8FC1
#endif
#ifndef GL_VIV_YUY2
#define GL_VIV_YUY2                                             0x8FC2
#endif
#ifndef GL_VIV_UYVY
#define GL_VIV_UYVY                                             0x8FC3
#endif
#ifndef GL_VIV_NV21
#define GL_VIV_NV21                                             0x8FC4
#endif
#ifndef GL_VIV_I420
#define GL_VIV_I420                                             0x8FC5
#endif

typedef void (*TexDirectVIVMap) (GLenum Target, GLsizei Width, GLsizei Height,
    GLenum Format, GLvoid ** Logical, const GLuint * Physical);
typedef void (*TexDirectInvalidateVIV) (GLenum Target);
static TexDirectVIVMap pTexDirectVIVMap = NULL;
static TexDirectInvalidateVIV pTexDirectInvalidateVIV = NULL;

typedef struct
{
  guint tex_id;
  guint w;
  guint h;
  guint fmt;
  void *vaddr;
  guint paddr;
  gboolean ret;
} DirectVIVData;

typedef struct _GstPhyMemAllocator GstPhyMemAllocator;
typedef struct _GstPhyMemAllocatorClass GstPhyMemAllocatorClass;

struct _GstPhyMemAllocator
{
  GstAllocatorPhyMem parent;
};

struct _GstPhyMemAllocatorClass
{
  GstAllocatorPhyMemClass parent_class;
};

GType gst_phy_mem_allocator_get_type (void);
G_DEFINE_TYPE (GstPhyMemAllocator, gst_phy_mem_allocator,
    GST_TYPE_ALLOCATOR_PHYMEM);

static int
alloc_phymem (GstAllocatorPhyMem * allocator, PhyMemBlock * memblk)
{
  struct g2d_buf *pbuf = NULL;

  memblk->size = PAGE_ALIGN (memblk->size);

  pbuf = g2d_alloc (memblk->size, 0);
  if (!pbuf) {
    GST_ERROR ("G2D allocate %u bytes memory failed: %s",
        memblk->size, strerror (errno));
    return -1;
  }

  memblk->vaddr = (guchar *) pbuf->buf_vaddr;
  memblk->paddr = (guchar *) pbuf->buf_paddr;
  memblk->user_data = (gpointer) pbuf;
  GST_DEBUG ("G2D allocated memory (%p)", memblk->paddr);

  return 1;
}

static int
free_phymem (GstAllocatorPhyMem * allocator, PhyMemBlock * memblk)
{
  GST_DEBUG ("G2D free memory (%p)", memblk->paddr);
  gint ret = g2d_free ((struct g2d_buf *) (memblk->user_data));
  memblk->user_data = NULL;
  memblk->vaddr = NULL;
  memblk->paddr = NULL;
  memblk->size = 0;

  return ret;
}

static void
gst_phy_mem_allocator_class_init (GstPhyMemAllocatorClass * klass)
{
  GstAllocatorPhyMemClass *phy_allocator_klass =
      (GstAllocatorPhyMemClass *) klass;

  phy_allocator_klass->alloc_phymem = alloc_phymem;
  phy_allocator_klass->free_phymem = free_phymem;
}

static void
gst_phy_mem_allocator_init (GstPhyMemAllocator * allocator)
{
  GstAllocator *alloc = GST_ALLOCATOR_CAST (allocator);

  alloc->mem_type = GST_GL_PHY_MEM_ALLOCATOR;
}


static gpointer
gst_phy_mem_allocator_init_instance (gpointer data)
{
  GstAllocator *allocator =
      g_object_new (gst_phy_mem_allocator_get_type (), NULL);

  GST_DEBUG_CATEGORY_INIT (GST_CAT_GL_PHY_MEMORY, "glphymemory", 0,
      "GLPhysical Memory");

  gst_allocator_register (GST_GL_PHY_MEM_ALLOCATOR, gst_object_ref (allocator));

  return allocator;
}

static void
_finish_texture (GstGLContext * ctx, gpointer * data)
{
  GstGLFuncs *gl = ctx->gl_vtable;

  gl->Finish ();
}

static void
_do_viv_direct_tex_bind_mem (GstGLContext * ctx, DirectVIVData * data)
{
  GstGLFuncs *gl = ctx->gl_vtable;

  GST_DEBUG ("viv direct bind, tex_id %d, fmt: %d, res: (%dx%d)", data->tex_id,
      data->fmt, data->w, data->h);
  GST_DEBUG ("Physical memory buffer, vaddr: %p, paddr: %p", data->vaddr,
      data->paddr);

  gl->BindTexture (GL_TEXTURE_2D, data->tex_id);
  pTexDirectVIVMap (GL_TEXTURE_2D, data->w, data->h, data->fmt, &data->vaddr,
      &data->paddr);
  pTexDirectInvalidateVIV (GL_TEXTURE_2D);
  data->ret = TRUE;
}

static GLenum
_directviv_video_format_to_gl_format (GstVideoFormat format)
{
  switch (format) {
    case GST_VIDEO_FORMAT_I420:
      return GL_VIV_I420;
    case GST_VIDEO_FORMAT_YV12:
      return GL_VIV_YV12;
    case GST_VIDEO_FORMAT_NV12:
      return GL_VIV_NV12;
    case GST_VIDEO_FORMAT_NV21:
      return GL_VIV_NV21;
    case GST_VIDEO_FORMAT_YUY2:
      return GL_VIV_YUY2;
    case GST_VIDEO_FORMAT_UYVY:
      return GL_VIV_UYVY;
    case GST_VIDEO_FORMAT_RGB16:
      return GL_RGB565;
    case GST_VIDEO_FORMAT_RGBA:
      return GL_RGBA;
    case GST_VIDEO_FORMAT_BGRA:
      return GL_BGRA_EXT;
    case GST_VIDEO_FORMAT_RGBx:
      return GL_RGBA;
    case GST_VIDEO_FORMAT_BGRx:
      return GL_BGRA_EXT;
    default:
      return 0;
  }
}

static void
gst_gl_phy_mem_destroy (GstMemory * mem)
{
  gst_memory_unref (mem);
}


GstAllocator *
gst_phy_mem_allocator_obtain (void)
{
  static GOnce once = G_ONCE_INIT;

  g_once (&once, gst_phy_mem_allocator_init_instance, NULL);

  g_return_val_if_fail (once.retval != NULL, NULL);

  return (GstAllocator *) (g_object_ref (once.retval));
}

gboolean
gst_is_gl_physical_memory (GstMemory * mem)
{
  GstGLBaseMemory *glmem;
  g_return_val_if_fail (gst_is_gl_memory (mem), FALSE);

  glmem = (GstGLBaseMemory *) mem;

  if (glmem->user_data
      && GST_IS_MINI_OBJECT_TYPE (glmem->user_data, GST_TYPE_MEMORY))
    return gst_memory_is_type ((GstMemory *) glmem->user_data,
        GST_GL_PHY_MEM_ALLOCATOR);
  else
    return FALSE;
}

gboolean
gst_is_gl_physical_memory_supported_fmt (GstVideoInfo * info)
{
  if (GST_VIDEO_INFO_IS_RGB (info)
      && _directviv_video_format_to_gl_format (GST_VIDEO_INFO_FORMAT (info))) {
    return TRUE;
  } else
    return FALSE;
}

gboolean
gst_gl_physical_memory_setup_buffer (GstAllocator * allocator,
    GstBuffer * buffer, GstGLVideoAllocationParams * params)
{
  GstGLBaseMemoryAllocator *gl_alloc;
  GstMemory *mem = NULL;
  PhyMemBlock *memblk = NULL;
  GstGLMemory *glmem = NULL;
  gsize size;

  GstVideoInfo *info = params->v_info;
  GstVideoAlignment *valign = params->valign;

  GST_DEBUG ("glphymemory setup buffer format %s",
      gst_video_format_to_string (GST_VIDEO_INFO_FORMAT (info)));

  if (!gst_is_gl_physical_memory_supported_fmt (info)) {
    GST_DEBUG ("Not support format.");
    return FALSE;
  }

  if (!pTexDirectVIVMap || !pTexDirectInvalidateVIV) {
    pTexDirectVIVMap =
        gst_gl_context_get_proc_address (params->parent.context,
        "glTexDirectVIVMap");
    pTexDirectInvalidateVIV =
        gst_gl_context_get_proc_address (params->parent.context,
        "glTexDirectInvalidateVIV");
  }

  if (!pTexDirectVIVMap || !pTexDirectInvalidateVIV) {
    GST_DEBUG ("Load directviv functions failed.");
    return FALSE;
  }

  size = gst_gl_get_plane_data_size (info, valign, 0);
  mem = gst_allocator_alloc (allocator, size, params->parent.alloc_params);
  if (!mem) {
    GST_DEBUG ("Can't allocate physical memory size %d", size);
    return FALSE;
  }

  memblk = gst_memory_query_phymem_block (mem);
  if (!memblk) {
    GST_ERROR ("Can't find physic memory block.");
    return FALSE;
  }

  gl_alloc =
      GST_GL_BASE_MEMORY_ALLOCATOR (gst_gl_memory_allocator_get_default
      (params->parent.context));

  params->plane = 0;
  params->parent.user_data = mem;
  params->parent.notify = gst_gl_phy_mem_destroy;
  params->tex_format =
      gst_gl_format_from_video_info (params->parent.context, info, 0);

  glmem =
      (GstGLMemory *) gst_gl_base_memory_alloc (gl_alloc,
      (GstGLAllocationParams *) params);
  gst_object_unref (gl_alloc);
  if (!glmem) {
    GST_ERROR ("Can't get gl memory.");
    return FALSE;
  }

  gst_buffer_append_memory (buffer, (GstMemory *) glmem);

  gst_buffer_add_video_meta_full (buffer, 0,
      GST_VIDEO_INFO_FORMAT (info), GST_VIDEO_INFO_WIDTH (info),
      GST_VIDEO_INFO_HEIGHT (info), 1, info->offset, info->stride);

  guint viv_fmt =
      _directviv_video_format_to_gl_format (GST_VIDEO_INFO_FORMAT (info));

  DirectVIVData directvivdata = {
    glmem->tex_id,
    GST_VIDEO_INFO_WIDTH (info),
    GST_VIDEO_INFO_HEIGHT (info),
    viv_fmt,
    memblk->vaddr,
    (guint)memblk->paddr,
    FALSE
  };

  gst_gl_context_thread_add (params->parent.context,
      _do_viv_direct_tex_bind_mem, &directvivdata);

  return directvivdata.ret;
}

GstBuffer *
gst_gl_phymem_buffer_to_gstbuffer (GstGLContext * ctx,
    GstVideoInfo * info, GstBuffer * glbuf)
{
  GstBuffer *buf;
  GstGLBaseMemory *glmem;

  gst_gl_context_thread_add (ctx, (GstGLContextThreadFunc) _finish_texture,
      NULL);

  glmem = gst_buffer_peek_memory (glbuf, 0);

  buf = gst_buffer_new ();
  gst_buffer_append_memory (buf, (GstMemory *) glmem->user_data);
  gst_memory_ref ((GstMemory *) glmem->user_data);

  gst_buffer_add_video_meta_full (buf, 0,
      GST_VIDEO_INFO_FORMAT (info), GST_VIDEO_INFO_WIDTH (info),
      GST_VIDEO_INFO_HEIGHT (info), 1, info->offset, info->stride);
  GST_BUFFER_FLAGS (buf) = GST_BUFFER_FLAGS (glbuf);
  GST_BUFFER_PTS (buf) = GST_BUFFER_PTS (glbuf);
  GST_BUFFER_DTS (buf) = GST_BUFFER_DTS (glbuf);
  GST_BUFFER_DURATION (buf) = GST_BUFFER_DURATION (glbuf);

  return buf;
}
