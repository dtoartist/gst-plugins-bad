/*
 * GStreamer
 *
 * Copyright (C) 2012 Texas Instruments
 *
 * Authors:
 *  Pooja Prajod <poojaprajod@ti.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * SECTION:GstDRMAllocator
 * @short_description: GStreamer DRM allocator support
 *
 * Since: 1.6.3
 */


#include "gstdrmallocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>

#define INVALID_DRM_FD (-1)

GST_DEBUG_CATEGORY (drmallocator_debug);
#define GST_CAT_DEFAULT drmallocator_debug

#define gst_drm_allocator_parent_class parent_class
G_DEFINE_TYPE (GstDRMAllocator, gst_drm_allocator, GST_TYPE_FD_ALLOCATOR);

static GstMemory *
gst_drm_allocator_alloc (GstAllocator * allocator, gsize size,
    GstAllocationParams * params)
{
  GstDRMAllocator *self = GST_DRM_ALLOCATOR (allocator);
  int fd = -1;
  int DrmDeviceFD = self->DrmDeviceFD;
  GstMemory *mem;
  /* Variable for DRM Dumb Buffers */

  struct drm_mode_create_dumb creq;
  struct drm_mode_destroy_dumb dreq;
  int ret ;
  
  GST_LOG_OBJECT (self, "DRM Memory alloc");  
  
  memset(&creq, 0, sizeof(struct drm_mode_create_dumb));
  creq.width = size;
  creq.height = 1;
  creq.bpp = 8;

  /* Create a DRM dumb buffer */
  ret = drmIoctl (DrmDeviceFD, DRM_IOCTL_MODE_CREATE_DUMB, &creq);
  if (ret < 0) {
    GST_ERROR_OBJECT (self, "Create DRM dumb buffer failed");
    return NULL;
  }
  /* Get a dmabuf fd from the dumb buffer handle */
  drmPrimeHandleToFD (DrmDeviceFD, creq.handle, DRM_CLOEXEC | O_RDWR, &fd);

  if (fd < 0) {
    GST_ERROR_OBJECT (self, "Invalid fd returned: %d", fd);
    goto fail;
  }

  /* Get a dmabuf gstmemory with the fd */
  mem = gst_fd_allocator_alloc (allocator, fd, size, 0);  

  if (G_UNLIKELY (!mem)) {
    GST_ERROR_OBJECT (self, "GstDmaBufMemory allocation failed");
    close (fd);
    goto fail;
  }

  return mem;

  fail:
    memset(&dreq, 0, sizeof(struct drm_mode_destroy_dumb));
    dreq.handle = creq.handle;
    drmIoctl (DrmDeviceFD, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
    return NULL;
}

static void
gst_drm_allocator_free (GstAllocator * allocator, GstMemory * mem)
{
  GstDRMAllocator *self = GST_DRM_ALLOCATOR (allocator);
  uint32_t handle = 0;
  int DrmDeviceFD = self->DrmDeviceFD;
  int fd = -1;

  GST_LOG_OBJECT (self, "DRM Memory free");

  g_return_if_fail (GST_IS_ALLOCATOR (allocator));
  g_return_if_fail (mem != NULL);
  g_return_if_fail (gst_is_drm_memory (mem));

  fd = gst_fd_memory_get_fd (mem);
  drmPrimeFDToHandle(DrmDeviceFD, fd, &handle);    

  if (handle) {
    struct drm_mode_destroy_dumb dreq;
    memset(&dreq, 0, sizeof(struct drm_mode_destroy_dumb));
    dreq.handle = handle;
    drmIoctl (DrmDeviceFD, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
  }
  
  close (fd);
  GST_ALLOCATOR_CLASS (parent_class)->free (allocator, mem);
}

static void
gst_drm_allocator_finalize (GObject * obj)
{
  GstDRMAllocator *self = GST_DRM_ALLOCATOR (obj);
  GST_LOG_OBJECT (obj, "DRM Allocator finalize");

  close (self->DrmDeviceFD);
  self->DrmDeviceFD = INVALID_DRM_FD;

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
gst_drm_allocator_class_init (GstDRMAllocatorClass * klass)
{
  GstAllocatorClass *drm_alloc = (GstAllocatorClass *) klass;

  drm_alloc->alloc = GST_DEBUG_FUNCPTR (gst_drm_allocator_alloc);
  drm_alloc->free = GST_DEBUG_FUNCPTR (gst_drm_allocator_free);
  GST_DEBUG_CATEGORY_INIT (drmallocator_debug, "drmallocator", 0,
    "GstDRMAllocator debug");

}

static void
gst_drm_allocator_init (GstDRMAllocator * self)
{
  GstAllocator *alloc = GST_ALLOCATOR_CAST (self);
  GObjectClass *object_class = G_OBJECT_CLASS (GST_DRM_ALLOCATOR_GET_CLASS(self));
  
  if (self->DrmDeviceFD <= 0) {
    self->DrmDeviceFD = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (self->DrmDeviceFD < 0) {
      GST_ERROR_OBJECT (self, "Failed to open DRM device");
    } else {
      drmDropMaster (self->DrmDeviceFD);
    }
  }

  alloc->mem_type = GST_ALLOCATOR_DMABUF;

  object_class->finalize = gst_drm_allocator_finalize;

  GST_OBJECT_FLAG_UNSET (self, GST_ALLOCATOR_FLAG_CUSTOM_ALLOC);
}

void
gst_drm_allocator_register (void)
{
  gst_allocator_register (GST_ALLOCATOR_DRM,
      g_object_new (GST_TYPE_DRM_ALLOCATOR, NULL));
}

GstAllocator *
gst_drm_allocator_get (void)
{
  GstAllocator *alloc;
  alloc = gst_allocator_find (GST_ALLOCATOR_DRM);
  if (!alloc) {
    gst_drm_allocator_register();
    alloc = gst_allocator_find (GST_ALLOCATOR_DRM);
  }
  return alloc; 
}

gboolean
gst_is_drm_memory (GstMemory * mem)
{
  return gst_memory_is_type (mem, GST_ALLOCATOR_DMABUF);
}
