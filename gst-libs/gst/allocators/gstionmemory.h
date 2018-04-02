/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc. All rights reserved.
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GST_IONMEMORY_H__
#define __GST_IONMEMORY_H__

#include <gst/gst.h>
#include <gst/allocators/gstdmabuf.h>
#include <gst/allocators/allocators-prelude.h>

G_BEGIN_DECLS

typedef struct _GstIONAllocator GstIONAllocator;
typedef struct _GstIONAllocatorClass GstIONAllocatorClass;
typedef struct _GstIONMemory GstIONMemory;

#define GST_ALLOCATOR_ION "ionmem"

#define GST_TYPE_ION_ALLOCATOR gst_ion_allocator_get_type ()
#define GST_IS_ION_ALLOCATOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
    GST_TYPE_ION_ALLOCATOR))
#define GST_ION_ALLOCATOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_ION_ALLOCATOR, GstIONAllocator))
#define GST_ION_ALLOCATOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_ION_ALLOCATOR, GstIONAllocatorClass))
#define GST_ION_ALLOCATOR_CAST(obj) ((GstIONAllocator *)(obj))

#define GST_ION_MEMORY_QUARK gst_ion_memory_quark ()

struct _GstIONAllocator
{
  GstDmaBufAllocator parent;

  gint fd;
  guint heap_id;
  guint flags;
};

struct _GstIONAllocatorClass
{
  GstDmaBufAllocatorClass parent;
};

GST_ALLOCATORS_API
GType gst_ion_allocator_get_type (void);

GST_ALLOCATORS_API
GstAllocator* gst_ion_allocator_obtain (void);

G_END_DECLS

#endif /* __GST_IONMEMORY_H__ */
