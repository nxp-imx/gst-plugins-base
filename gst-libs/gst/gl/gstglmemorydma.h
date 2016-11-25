/*
 * GStreamer
 * Copyright (c) 2016, Freescale Semiconductor, Inc. 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundatdma; either
 * versdma 2 of the License, or (at your optdma) any later versdma.
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

#ifndef _GST_GL_MEMORY_DMA_H_
#define _GST_GL_MEMORY_DMA_H_

#include <gst/gst.h>
#include <gst/gstallocator.h>
#include <gst/gstmemory.h>
#include <gst/video/video.h>

#include <gst/gl/gl.h>
#include <gst/gl/egl/gstglcontext_egl.h>
#include <gst/gl/egl/gsteglimage.h>

#include <gst/gl/gstglmemory.h>

G_BEGIN_DECLS

#define GST_TYPE_GL_MEMORY_DMA_ALLOCATOR (gst_gl_memory_dma_allocator_get_type())
GST_GL_API
GType gst_gl_memory_dma_allocator_get_type(void);

#define GST_IS_GL_MEMORY_DMA_ALLOCATOR(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_GL_MEMORY_DMA_ALLOCATOR))
#define GST_IS_GL_MEMORY_DMA_ALLOCATOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_GL_MEMORY_DMA_ALLOCATOR))
#define GST_GL_MEMORY_DMA_ALLOCATOR_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_GL_MEMORY_DMA_ALLOCATOR, GstGLMemoryDMAAllocatorClass))
#define GST_GL_MEMORY_DMA_ALLOCATOR(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_GL_MEMORY_DMA_ALLOCATOR, GstGLMemoryDMAAllocator))
#define GST_GL_MEMORY_DMA_ALLOCATOR_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_GL_MEMORY_DMA_ALLOCATOR, GstGLAllocatorClass))
#define GST_GL_MEMORY_DMA_ALLOCATOR_CAST(obj)            ((GstGLMemoryDMAAllocator *)(obj))

struct _GstGLMemoryDMA
{
  GstGLMemory   mem;

  /* <private> */
  GstEGLImage	*eglimage;
  GstMemory     *dma;
  GstAllocationParams *params;
};

#define GST_GL_MEMORY_DMA_ALLOCATOR_NAME   "GLMemoryDMA"

struct _GstGLMemoryDMAAllocator
{
  GstGLMemoryAllocator parent;
  GstAllocator  *ion_allocator;
};

struct _GstGLMemoryDMAAllocatorClass
{
  GstGLMemoryAllocatorClass parent_class;
};

GST_GL_API
GstAllocator *gst_gl_memory_dma_allocator_obtain (void);

GST_GL_API
gboolean      gst_is_gl_memory_dma                      (GstMemory * mem);

GST_GL_API
GstBuffer *   gst_gl_memory_dma_buffer_to_gstbuffer     (GstGLContext * ctx, GstVideoInfo * info, GstBuffer * glbuf);

G_END_DECLS

#endif /* _GST_GL_MEMORY_DMA_H_ */
