#include "wldisplay.h"
#include <gst/drm/gstdrmallocator.h>
#include "wayland-drm-client-protocol.h"
#include <tcc_drm.h>
#include <tcc_drmif.h>
#include <wayland-client.h>

struct wl_buffer *
gst_wl_drm_memory_construct_wl_buffer (GstMemory * mem, GstWlDisplay * display,
    const GstVideoInfo * info)
{
  gint video_width = GST_VIDEO_INFO_WIDTH (info);
  gint video_height = GST_VIDEO_INFO_HEIGHT (info);
  int fd = -1;
  struct tcc_bo *bo;
  struct wl_buffer *buffer;

  /* TODO get format, etc from caps.. and query device for
   * supported formats, and make this all more flexible to
   * cope with various formats:
   */
  uint32_t fourcc = GST_MAKE_FOURCC ('N', 'V', '1', '2');
  uint32_t name;
  /* note: wayland and mesa use the terminology:
   *    stride - rowstride in bytes
   *    pitch  - rowstride in pixels
   */
  uint32_t strides[3] = {
    GST_ROUND_UP_4 (video_width), GST_ROUND_UP_4 (video_width), 0,
  };
  uint32_t offsets[3] = {
    0, strides[0] * video_height, 0
  };

  fd = gst_fd_memory_get_fd (mem);

  if (fd < 0 ) {
    GST_DEBUG ("Invalid fd");
    return NULL;
  }

  struct drm_gem_flink req;

  int ret;
  ret = tcc_prime_fd_to_handle(display->dev, fd, &req.handle);
  if (ret) {
    GST_DEBUG ("could not get handle, drmPrimeFDToHandle returned %d", ret);
    return NULL;
  }

  ret = drmIoctl(display->fd, DRM_IOCTL_GEM_FLINK, &req);
  if (ret) {
    GST_DEBUG ("could not get name, DRM_IOCTL_GEM_FLINK returned %d", ret);
    return NULL;
  }

  name = req.name;

  GST_LOG ("width = %d , height = %d , fourcc = %d ",  video_width, video_height, fourcc );
  buffer = wl_drm_create_planar_buffer (display->drm, name,
      video_width, video_height, fourcc,
      offsets[0], strides[0],
      offsets[1], strides[1],
      offsets[2], strides[2]);

  GST_DEBUG ("create planar buffer: %p (name=%d)",
      buffer, name);

  return buffer;
}

