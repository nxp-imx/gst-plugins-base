/*
 * Copyright 2020 NXP
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GST_DMABUFHEAPS_H__
#define __GST_DMABUFHEAPS_H__

#include <gst/gst.h>
#include <gst/allocators/gstdmabuf.h>
#include <gst/allocators/allocators-prelude.h>

G_BEGIN_DECLS

typedef struct _GstDMABUFHEAPSAllocator GstDMABUFHEAPSAllocator;
typedef struct _GstDMABUFHEAPSAllocatorClass GstDMABUFHEAPSAllocatorClass;
typedef struct _GstDMABUFHEAPSMemory GstDMABUFHEAPSMemory;

#define GST_ALLOCATOR_DMABUFHEAPS "dmabufheapsmem"

#define GST_TYPE_DMABUFHEAPS_ALLOCATOR gst_dmabufheaps_allocator_get_type ()
#define GST_IS_DMABUFHEAPS_ALLOCATOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
    GST_TYPE_DMABUFHEAPS_ALLOCATOR))
#define GST_DMABUFHEAPS_ALLOCATOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_DMABUFHEAPS_ALLOCATOR, GstDMABUFHEAPSAllocator))
#define GST_DMABUFHEAPS_ALLOCATOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_DMABUFHEAPS_ALLOCATOR, GstDMABUFHEAPSAllocatorClass))
#define GST_DMABUFHEAPS_ALLOCATOR_CAST(obj) ((GstDMABUFHEAPSAllocator *)(obj))

#define GST_DMABUFHEAPS_MEMORY_QUARK gst_dmabufheaps_memory_quark ()

struct _GstDMABUFHEAPSAllocator
{
  GstDmaBufAllocator parent;

  gint fd;
  guint fd_flags;
  guint heap_flags;
};

struct _GstDMABUFHEAPSAllocatorClass
{
  GstDmaBufAllocatorClass parent;
};

GST_ALLOCATORS_API
GType gst_dmabufheaps_allocator_get_type (void);

GST_ALLOCATORS_API
GstAllocator* gst_dmabufheaps_allocator_obtain (void);

G_END_DECLS

#endif /* __GST_DMABUFHEAPS_H__ */
