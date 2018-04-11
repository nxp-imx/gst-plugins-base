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

#ifndef _GST_GL_PHY_MEMORY_H_
#define _GST_GL_PHY_MEMORY_H_

#include <gst/gst.h>
#include <gst/gstmemory.h>
#include <gst/video/video.h>
#include <gst/allocators/gstallocatorphymem.h>

#include <gst/gl/gl.h>

G_BEGIN_DECLS

#define GST_GL_PHY_MEM_ALLOCATOR "GLPhyMemory"

GST_GL_API
GstAllocator *gst_phy_mem_allocator_obtain (void);
GST_GL_API
gboolean gst_is_gl_physical_memory (GstMemory * mem);
GST_GL_API
gboolean gst_is_gl_physical_memory_supported_fmt (GstVideoInfo * info);
GST_GL_API
gboolean gst_gl_physical_memory_setup_buffer (GstAllocator * allocator, GstBuffer *buffer, GstGLVideoAllocationParams * params);
GST_GL_API
GstBuffer * gst_gl_phymem_buffer_to_gstbuffer (GstGLContext * ctx, GstVideoInfo * info, GstBuffer *glbuf);

G_END_DECLS

#endif /* _GST_GL_PHY_MEMORY_H_ */
