/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc. All rights reserved.
 * Copyright 2017, 2019 NXP
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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <linux/ion.h>
#include <linux/dma-buf.h>
#include <linux/version.h>


#include <gst/allocators/gstdmabuf.h>
#include "gstphysmemory.h"
#include "gstionmemory.h"

GST_DEBUG_CATEGORY_STATIC (ion_allocator_debug);
#define GST_CAT_DEFAULT ion_allocator_debug

#define gst_ion_allocator_parent_class parent_class

#define DEFAULT_HEAP_ID  0
#define DEFAULT_FLAG     0

enum
{
  PROP_0,
  PROP_HEAP_ID,
  PROP_FLAG,
  PROP_LAST
};

static guintptr
gst_ion_allocator_get_phys_addr (GstPhysMemoryAllocator * allocator,
    GstMemory * mem)
{
  GstIONAllocator *self = GST_ION_ALLOCATOR (allocator);
  gint ret, fd;

  if (self->fd < 0 || !mem) {
    GST_ERROR ("ion get phys param wrong");
    return 0;
  }

  if (!gst_is_dmabuf_memory (mem)) {
    GST_ERROR ("isn't dmabuf memory");
    return 0;
  }

  fd = gst_dmabuf_memory_get_fd (mem);
  if (fd < 0) {
    GST_ERROR ("dmabuf memory get fd failed");
    return 0;
  }

  GST_DEBUG ("ion DMA FD: %d", fd);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
  struct ion_phys_dma_data data = {
    .phys = 0,
    .size = 0,
    .dmafd = fd,
  };
  struct ion_custom_data custom = {
    .cmd = ION_IOC_PHYS_DMA,
    .arg = (unsigned long) &data,
  };

  ret = ioctl (self->fd, ION_IOC_CUSTOM, &custom);
  if (ret < 0)
    return 0;

  return data.phys;
#else
  struct dma_buf_phys dma_phys;

  ret = ioctl (fd, DMA_BUF_IOCTL_PHYS, &dma_phys);
  if (ret < 0)
    return 0;

  return dma_phys.phys;
#endif
}

static void
gst_ion_allocator_iface_init (gpointer g_iface)
{
  GstPhysMemoryAllocatorInterface *iface = g_iface;
  iface->get_phys_addr = gst_ion_allocator_get_phys_addr;
}

G_DEFINE_TYPE_WITH_CODE (GstIONAllocator, gst_ion_allocator,
    GST_TYPE_DMABUF_ALLOCATOR,
    G_IMPLEMENT_INTERFACE (GST_TYPE_PHYS_MEMORY_ALLOCATOR,
        gst_ion_allocator_iface_init));

static gint
gst_ion_ioctl (gint fd, gint req, void *arg)
{
  gint ret = ioctl (fd, req, arg);
  if (ret < 0) {
    GST_ERROR ("ioctl %x failed with code %d: %s\n", req, ret,
        strerror (errno));
  }
  return ret;
}

static void
gst_ion_mem_init (void)
{
  GstAllocator *allocator = g_object_new (gst_ion_allocator_get_type (), NULL);
  GstIONAllocator *self = GST_ION_ALLOCATOR (allocator);
  gint fd;

  fd = open ("/dev/ion", O_RDWR);
  if (fd < 0) {
    GST_WARNING ("Could not open ion driver");
    g_object_unref (self);
    return;
  }

  self->fd = fd;

  gst_allocator_register (GST_ALLOCATOR_ION, allocator);
}

GstAllocator *
gst_ion_allocator_obtain (void)
{
  static GOnce ion_allocator_once = G_ONCE_INIT;
  GstAllocator *allocator;

  g_once (&ion_allocator_once, (GThreadFunc) gst_ion_mem_init, NULL);

  allocator = gst_allocator_find (GST_ALLOCATOR_ION);
  if (allocator == NULL)
    GST_WARNING ("No allocator named %s found", GST_ALLOCATOR_ION);

  return allocator;
}

static GstMemory *
gst_ion_alloc_alloc (GstAllocator * allocator, gsize size,
    GstAllocationParams * params)
{
  GstIONAllocator *self = GST_ION_ALLOCATOR (allocator);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
  struct ion_allocation_data allocation_data = { 0 };
  struct ion_fd_data fd_data = { 0 };
  struct ion_handle_data handle_data = { 0 };
  ion_user_handle_t ion_handle;
  GstMemory *mem;
  gsize ion_size;
  gint dma_fd = -1;
  gint ret;

  if (self->fd < 0) {
    GST_ERROR ("ion allocate param wrong");
    return NULL;
  }

  ion_size = size + params->prefix + params->padding;
  allocation_data.len = ion_size;
  allocation_data.align = params->align;
  allocation_data.heap_id_mask = 1 << self->heap_id;
  allocation_data.flags = self->flags;
  if (gst_ion_ioctl (self->fd, ION_IOC_ALLOC, &allocation_data) < 0) {
    GST_ERROR ("ion allocate failed.");
    return NULL;
  }
  ion_handle = allocation_data.handle;

  fd_data.handle = ion_handle;
  ret = gst_ion_ioctl (self->fd, ION_IOC_MAP, &fd_data);
  if (ret < 0 || fd_data.fd < 0) {
    GST_ERROR ("map ioctl failed or returned negative fd");
    goto bail;
  }
  dma_fd = fd_data.fd;

  handle_data.handle = ion_handle;
  gst_ion_ioctl (self->fd, ION_IOC_FREE, &handle_data);

#else
  gint heapCnt = 0;
  gint heap_mask = 0;
  GstMemory *mem;
  gsize ion_size;
  gint dma_fd = -1;
  gint ret;

  struct ion_heap_query query;
  memset (&query, 0, sizeof (query));
  ret = gst_ion_ioctl (self->fd, ION_IOC_HEAP_QUERY, &query);
  if (ret != 0 || query.cnt == 0) {
    GST_ERROR ("can't query heap count");
    return NULL;
  }
  heapCnt = query.cnt;

  struct ion_heap_data ihd[heapCnt];
  memset (&ihd, 0, sizeof (ihd));
  query.cnt = heapCnt;
  query.heaps = (uintptr_t)&ihd;
  ret = gst_ion_ioctl (self->fd, ION_IOC_HEAP_QUERY, &query);
  if (ret != 0) {
    GST_ERROR ("can't get ion heaps");
    return NULL;
  }

  for (gint i = 0; i < heapCnt; i++) {
    if (ihd[i].type == ION_HEAP_TYPE_DMA) {
      heap_mask |= 1 << ihd[i].heap_id;
    }
  }

  ion_size = size + params->prefix + params->padding;
  struct ion_allocation_data data = {
    .len = ion_size,
    .heap_id_mask = heap_mask,
    .flags = self->flags,
  };
  ret = gst_ion_ioctl (self->fd, ION_IOC_ALLOC, &data);
  if (ret < 0) {
    GST_ERROR ("ion allocate failed.");
    return NULL;
  }
  dma_fd = data.fd;
#endif

  mem = gst_dmabuf_allocator_alloc (allocator, dma_fd, size);

  GST_DEBUG ("ion allocated size: %" G_GSIZE_FORMAT "DMA FD: %d", ion_size,
      dma_fd);

  return mem;

bail:
  if (dma_fd >= 0) {
    close (dma_fd);
  }
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
  handle_data.handle = ion_handle;
  gst_ion_ioctl (self->fd, ION_IOC_FREE, &handle_data);
#endif

  return NULL;
}

static void
gst_ion_allocator_dispose (GObject * object)
{
  GstIONAllocator *self = GST_ION_ALLOCATOR (object);

  if (self->fd > 0) {
    close (self->fd);
    self->fd = -1;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_ion_allocator_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstIONAllocator *self = GST_ION_ALLOCATOR (object);

  switch (prop_id) {
    case PROP_HEAP_ID:
      self->heap_id = g_value_get_uint (value);
      break;
    case PROP_FLAG:
      self->flags = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_ion_allocator_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstIONAllocator *self = GST_ION_ALLOCATOR (object);

  switch (prop_id) {
    case PROP_HEAP_ID:
      g_value_set_uint (value, self->heap_id);
      break;
    case PROP_FLAG:
      g_value_set_uint (value, self->flags);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_ion_allocator_class_init (GstIONAllocatorClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstAllocatorClass *allocator_class = GST_ALLOCATOR_CLASS (klass);

  gobject_class->dispose = GST_DEBUG_FUNCPTR (gst_ion_allocator_dispose);
  gobject_class->set_property = gst_ion_allocator_set_property;
  gobject_class->get_property = gst_ion_allocator_get_property;

  g_object_class_install_property (gobject_class, PROP_HEAP_ID,
      g_param_spec_uint ("heap-id", "Heap ID",
          "ION heap id", 0, G_MAXUINT32, DEFAULT_HEAP_ID,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FLAG,
      g_param_spec_uint ("flags", "Flags",
          "ION memory flags", 0, G_MAXUINT32, DEFAULT_FLAG,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  allocator_class->alloc = GST_DEBUG_FUNCPTR (gst_ion_alloc_alloc);

  GST_DEBUG_CATEGORY_INIT (ion_allocator_debug, "ionmemory", 0,
      "DMA FD memory allocator based on ion");
}

static void
gst_ion_allocator_init (GstIONAllocator * self)
{
  GstAllocator *allocator = GST_ALLOCATOR (self);

  allocator->mem_type = GST_ALLOCATOR_ION;

  self->heap_id = DEFAULT_HEAP_ID;
  self->flags = DEFAULT_FLAG;
}
