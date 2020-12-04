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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <gst/allocators/gstdmabuf.h>
#include "gstphysmemory.h"
#include "gstdmabufheaps.h"

GST_DEBUG_CATEGORY_STATIC (dmabufheaps_allocator_debug);
#define GST_CAT_DEFAULT dmabufheaps_allocator_debug

#define gst_dmabufheaps_allocator_parent_class parent_class

#define DEFAULT_FD_FLAGS  (O_RDWR | O_CLOEXEC)
#define DEFAULT_HEAP_FLAGS     DMA_HEAP_VALID_HEAP_FLAGS

enum
{
  PROP_0,
  PROP_FD_FLAGS,
  PROP_HEAP_FLAGS,
  PROP_LAST
};

static guintptr
gst_dmabufheaps_allocator_get_phys_addr (GstPhysMemoryAllocator * allocator,
    GstMemory * mem)
{
  struct dma_buf_phys dma_phys;
  gint ret, fd;

  if (!gst_is_dmabuf_memory (mem)) {
    GST_ERROR ("isn't dmabuf memory");
    return 0;
  }

  fd = gst_dmabuf_memory_get_fd (mem);
  if (fd < 0) {
    GST_ERROR ("dmabuf memory get fd failed");
    return 0;
  }

  GST_DEBUG ("dmabufheaps DMA FD: %d", fd);

  ret = ioctl (fd, DMA_BUF_IOCTL_PHYS, &dma_phys);
  if (ret < 0)
    return 0;

  return dma_phys.phys;
}

static void
gst_dmabufheaps_allocator_iface_init (gpointer g_iface)
{
  GstPhysMemoryAllocatorInterface *iface = g_iface;
  iface->get_phys_addr = gst_dmabufheaps_allocator_get_phys_addr;
}

G_DEFINE_TYPE_WITH_CODE (GstDMABUFHEAPSAllocator, gst_dmabufheaps_allocator,
    GST_TYPE_DMABUF_ALLOCATOR,
    G_IMPLEMENT_INTERFACE (GST_TYPE_PHYS_MEMORY_ALLOCATOR,
        gst_dmabufheaps_allocator_iface_init));

static void
gst_dmabufheaps_allocator_mem_init (void)
{
  GstAllocator *allocator =
      g_object_new (gst_dmabufheaps_allocator_get_type (), NULL);
  GstDMABUFHEAPSAllocator *self = GST_DMABUFHEAPS_ALLOCATOR (allocator);
  gint fd;

  fd = open ("/dev/dma_heap/linux,cma", O_RDWR);
  if (fd < 0) {
    GST_WARNING ("Could not open dmabufheaps driver");
    g_object_unref (self);
    return;
  }

  self->fd = fd;

  gst_allocator_register (GST_ALLOCATOR_DMABUFHEAPS, allocator);
}

GstAllocator *
gst_dmabufheaps_allocator_obtain (void)
{
  static GOnce dmabufheaps_allocator_once = G_ONCE_INIT;
  GstAllocator *allocator;

  g_once (&dmabufheaps_allocator_once,
      (GThreadFunc) gst_dmabufheaps_allocator_mem_init, NULL);

  allocator = gst_allocator_find (GST_ALLOCATOR_DMABUFHEAPS);
  if (allocator == NULL)
    GST_WARNING ("No allocator named %s found", GST_ALLOCATOR_DMABUFHEAPS);

  return allocator;
}

static GstMemory *
gst_dmabufheaps_allocator_alloc (GstAllocator * allocator, gsize size,
    GstAllocationParams * params)
{
  GstDMABUFHEAPSAllocator *self = GST_DMABUFHEAPS_ALLOCATOR (allocator);
  struct dma_heap_allocation_data data = { 0 };
  GstMemory *mem;
  gsize dmabufheaps_size;
  gint dma_fd = -1;
  gint ret;

  if (self->fd < 0) {
    GST_WARNING ("don't open dmabufheaps driver");
    return NULL;
  }

  dmabufheaps_size = size + params->prefix + params->padding;
  data.len = dmabufheaps_size,
      data.fd_flags = self->fd_flags,
      data.heap_flags = self->heap_flags,
      ret = ioctl (self->fd, DMA_HEAP_IOCTL_ALLOC, &data);
  if (ret < 0) {
    GST_ERROR ("dmabufheaps allocate failed.");
    return NULL;
  }
  dma_fd = data.fd;

  mem =
      gst_dmabuf_allocator_alloc_with_flags (allocator, dma_fd, size,
      GST_FD_MEMORY_FLAG_KEEP_MAPPED);

  GST_DEBUG ("dmabufheaps allocated size: %" G_GSIZE_FORMAT "DMA FD: %d",
      dmabufheaps_size, dma_fd);

  return mem;
}

static void
gst_dmabufheaps_allocator_dispose (GObject * object)
{
  GstDMABUFHEAPSAllocator *self = GST_DMABUFHEAPS_ALLOCATOR (object);

  if (self->fd > 0) {
    close (self->fd);
    self->fd = -1;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_dmabufheaps_allocator_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDMABUFHEAPSAllocator *self = GST_DMABUFHEAPS_ALLOCATOR (object);

  switch (prop_id) {
    case PROP_FD_FLAGS:
      self->fd_flags = g_value_get_uint (value);
      break;
    case PROP_HEAP_FLAGS:
      self->heap_flags = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_dmabufheaps_allocator_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstDMABUFHEAPSAllocator *self = GST_DMABUFHEAPS_ALLOCATOR (object);

  switch (prop_id) {
    case PROP_FD_FLAGS:
      g_value_set_uint (value, self->fd_flags);
      break;
    case PROP_HEAP_FLAGS:
      g_value_set_uint (value, self->heap_flags);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_dmabufheaps_allocator_class_init (GstDMABUFHEAPSAllocatorClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstAllocatorClass *allocator_class = GST_ALLOCATOR_CLASS (klass);

  gobject_class->dispose =
      GST_DEBUG_FUNCPTR (gst_dmabufheaps_allocator_dispose);
  gobject_class->set_property = gst_dmabufheaps_allocator_set_property;
  gobject_class->get_property = gst_dmabufheaps_allocator_get_property;

  g_object_class_install_property (gobject_class, PROP_FD_FLAGS,
      g_param_spec_uint ("fd-flags", "FD Flags",
          "DMABUFHEAPS fd flags", 0, G_MAXUINT32, DEFAULT_FD_FLAGS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_HEAP_FLAGS,
      g_param_spec_uint ("heap-flags", "Heap Flags",
          "DMABUFHEAPS heap flags", 0, G_MAXUINT32, DEFAULT_HEAP_FLAGS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  allocator_class->alloc = GST_DEBUG_FUNCPTR (gst_dmabufheaps_allocator_alloc);

  GST_DEBUG_CATEGORY_INIT (dmabufheaps_allocator_debug, "dmabufheapsmemory", 0,
      "DMA FD memory allocator based on dma-buf heaps");
}

static void
gst_dmabufheaps_allocator_init (GstDMABUFHEAPSAllocator * self)
{
  GstAllocator *allocator = GST_ALLOCATOR (self);

  allocator->mem_type = GST_ALLOCATOR_DMABUFHEAPS;

  self->fd_flags = DEFAULT_FD_FLAGS;
  self->heap_flags = DEFAULT_HEAP_FLAGS;
}
