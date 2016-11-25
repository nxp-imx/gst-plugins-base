/*
 * GStreamer
 * Copyright (c) 2016, Freescale Semiconductor, Inc. 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundatdma; either
 * versdma 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundatdma, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstglfuncs.h"

#include <string.h>

#include <gst/allocators/gstdmabuf.h>
#include <gst/gl/gstglmemorydma.h>

#if GST_GL_HAVE_IONDMA
#include <gst/allocators/gstionmemory.h>
#endif

GST_DEBUG_CATEGORY_STATIC (GST_CAT_GL_DMA_MEMORY);
#define GST_CAT_DEFAULT GST_CAT_GL_DMA_MEMORY

#define parent_class gst_gl_memory_dma_allocator_parent_class
G_DEFINE_TYPE (GstGLMemoryDMAAllocator, gst_gl_memory_dma_allocator,
    GST_TYPE_GL_MEMORY_ALLOCATOR);

static void
gst_gl_memory_dma_init_instance (void)
{
  GstAllocator *ion_allocator = NULL;
  GstGLMemoryDMAAllocator *_gl_allocator;

  GST_DEBUG_CATEGORY_INIT (GST_CAT_GL_DMA_MEMORY, "glmemorydma", 0,
      "OpenGL dma memory");

#if GST_GL_HAVE_IONDMA
  ion_allocator = gst_ion_allocator_obtain ();
#endif

  if (!ion_allocator)
    return;

  gst_gl_memory_init_once ();

  _gl_allocator = (GstGLMemoryDMAAllocator *)
      g_object_new (GST_TYPE_GL_MEMORY_DMA_ALLOCATOR, NULL);
  _gl_allocator->ion_allocator = ion_allocator;

  gst_allocator_register (GST_GL_MEMORY_DMA_ALLOCATOR_NAME,
      gst_object_ref (_gl_allocator));
}

GstAllocator *
gst_gl_memory_dma_allocator_obtain (void)
{

  static GOnce once = G_ONCE_INIT;
  GstAllocator *allocator;

  g_once (&once, (GThreadFunc) gst_gl_memory_dma_init_instance, NULL);

  allocator = gst_allocator_find (GST_GL_MEMORY_DMA_ALLOCATOR_NAME);
  if (allocator == NULL)
    GST_WARNING ("No allocator named %s found",
        GST_GL_MEMORY_DMA_ALLOCATOR_NAME);

  return allocator;
}

static void
gst_gl_memory_dma_allocator_dispose (GObject * object)
{
  GstGLMemoryDMAAllocator *gl_dma_alloc = GST_GL_MEMORY_DMA_ALLOCATOR (object);

  if (gl_dma_alloc->ion_allocator) {
    GST_DEBUG ("free ion allocator");
    gst_object_unref (gl_dma_alloc->ion_allocator);
    gl_dma_alloc->ion_allocator = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gboolean
_gl_mem_create (GstGLMemoryDMA * gl_mem, GError ** error)
{
  GstGLContext *context = gl_mem->mem.mem.context;
  GstGLBaseMemoryAllocatorClass *alloc_class;
  guint dma_fd;

  alloc_class = GST_GL_BASE_MEMORY_ALLOCATOR_CLASS (parent_class);
  if (!alloc_class->create ((GstGLBaseMemory *) gl_mem, error))
    return FALSE;

  dma_fd = gst_dmabuf_memory_get_fd ((GstMemory *) gl_mem->dma);

  gl_mem->eglimage =
      gst_egl_image_from_dmabuf (context, dma_fd, &gl_mem->mem.info, 0, 0);

  if (!gl_mem->eglimage) {
    GST_CAT_ERROR (GST_CAT_GL_DMA_MEMORY, "Can't allocate eglimage memory");
    return FALSE;
  }

  const GstGLFuncs *gl = context->gl_vtable;

  gl->ActiveTexture (GL_TEXTURE0);
  gl->BindTexture (GL_TEXTURE_2D, gl_mem->mem.tex_id);
  gl->EGLImageTargetTexture2D (GL_TEXTURE_2D,
      gst_egl_image_get_image (gl_mem->eglimage));

  GST_CAT_DEBUG (GST_CAT_GL_DMA_MEMORY,
      "generated dma buffer %p fd %u texid %u", gl_mem, dma_fd,
      gl_mem->mem.tex_id);

  return TRUE;
}

static GstMemory *
_gl_mem_alloc (GstAllocator * allocator, gsize size,
    GstAllocationParams * params)
{
  g_warning ("Use gst_gl_base_memory_alloc () to allocate from this "
      "GstGLMemoryDMA allocator");

  return NULL;
}

static void
_gl_mem_destroy (GstGLMemoryDMA * gl_mem)
{
  GST_CAT_DEBUG (GST_CAT_GL_DMA_MEMORY, "destroy gl dma buffer %p", gl_mem);

  if (gl_mem->eglimage)
    gst_egl_image_unref (gl_mem->eglimage);
  gl_mem->eglimage = NULL;
  if (gl_mem->dma)
    gst_memory_unref (GST_MEMORY_CAST (gl_mem->dma));
  gl_mem->dma = NULL;

  GST_GL_BASE_MEMORY_ALLOCATOR_CLASS (parent_class)->destroy ((GstGLBaseMemory
          *) gl_mem);
}

static GstGLMemoryDMA *
_gl_mem_dma_alloc (GstGLBaseMemoryAllocator * allocator,
    GstGLVideoAllocationParams * params)
{
  GstGLMemoryDMA *mem;
  guint alloc_flags;
  gsize size;
  GstGLMemoryDMAAllocator *gl_dma_alloc =
      GST_GL_MEMORY_DMA_ALLOCATOR (allocator);

  alloc_flags = params->parent.alloc_flags;

  g_return_val_if_fail (alloc_flags & GST_GL_ALLOCATION_PARAMS_ALLOC_FLAG_VIDEO,
      NULL);

  mem = g_new0 (GstGLMemoryDMA, 1);

  mem->params = params->parent.alloc_params;

  size =
      gst_gl_get_plane_data_size (params->v_info, params->valign,
      params->plane);
  mem->dma =
      gst_allocator_alloc (gl_dma_alloc->ion_allocator, size, mem->params);

  if (!mem->dma) {
    GST_CAT_ERROR (GST_CAT_GL_DMA_MEMORY, "Can't allocate dma memory size %d",
        size);
    g_free (mem);
    return NULL;
  }

  gst_gl_memory_init (GST_GL_MEMORY_CAST (mem), GST_ALLOCATOR_CAST (allocator),
      NULL, params->parent.context, params->target, params->tex_format,
      params->parent.alloc_params, params->v_info, params->plane,
      params->valign, params->parent.user_data, params->parent.notify);

  return mem;
}

static void
gst_gl_memory_dma_allocator_class_init (GstGLMemoryDMAAllocatorClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstGLBaseMemoryAllocatorClass *gl_base;
  GstAllocatorClass *allocator_class;

  gl_base = (GstGLBaseMemoryAllocatorClass *) klass;
  allocator_class = (GstAllocatorClass *) klass;

  gl_base->alloc = (GstGLBaseMemoryAllocatorAllocFunction) _gl_mem_dma_alloc;
  gl_base->create = (GstGLBaseMemoryAllocatorCreateFunction) _gl_mem_create;
  gl_base->destroy = (GstGLBaseMemoryAllocatorDestroyFunction) _gl_mem_destroy;
  gobject_class->dispose =
      GST_DEBUG_FUNCPTR (gst_gl_memory_dma_allocator_dispose);

  allocator_class->alloc = _gl_mem_alloc;
}

static void
gst_gl_memory_dma_allocator_init (GstGLMemoryDMAAllocator * allocator)
{
  GstAllocator *alloc = GST_ALLOCATOR_CAST (allocator);

  alloc->mem_type = GST_GL_MEMORY_DMA_ALLOCATOR_NAME;

  GST_OBJECT_FLAG_SET (allocator, GST_ALLOCATOR_FLAG_CUSTOM_ALLOC);
}

gboolean
gst_is_gl_memory_dma (GstMemory * mem)
{
  return mem != NULL && mem->allocator != NULL
      && g_type_is_a (G_OBJECT_TYPE (mem->allocator),
      GST_TYPE_GL_MEMORY_DMA_ALLOCATOR);
}

static void
_finish_texture (GstGLContext * ctx, gpointer * data)
{
  GstGLFuncs *gl = ctx->gl_vtable;

  gl->Finish ();
}

GstBuffer *
gst_gl_memory_dma_buffer_to_gstbuffer (GstGLContext * ctx, GstVideoInfo * info,
    GstBuffer * glbuf)
{
  GstBuffer *buf;
  GstGLMemoryDMA *glmem;

  gst_gl_context_thread_add (ctx, (GstGLContextThreadFunc) _finish_texture,
      NULL);

  glmem = gst_buffer_peek_memory (glbuf, 0);

  buf = gst_buffer_new ();
  gst_buffer_append_memory (buf, (GstMemory *) glmem->dma);
  gst_memory_ref ((GstMemory *) glmem->dma);

  gst_buffer_add_video_meta_full (buf, 0,
      GST_VIDEO_INFO_FORMAT (info), GST_VIDEO_INFO_WIDTH (info),
      GST_VIDEO_INFO_HEIGHT (info), 1, info->offset, info->stride);
  GST_BUFFER_FLAGS (buf) = GST_BUFFER_FLAGS (glbuf);
  GST_BUFFER_PTS (buf) = GST_BUFFER_PTS (glbuf);
  GST_BUFFER_DTS (buf) = GST_BUFFER_DTS (glbuf);
  GST_BUFFER_DURATION (buf) = GST_BUFFER_DURATION (glbuf);

  return buf;
}
