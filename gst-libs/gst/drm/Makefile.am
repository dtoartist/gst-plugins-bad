lib_LTLIBRARIES = libgstdrm-@GST_API_VERSION@.la

CLEANFILES =

libgstdrm_@GST_API_VERSION@_la_SOURCES = \
	gstdrmallocator.c

libgstdrm_@GST_API_VERSION@includedir = \
	$(includedir)/gstreamer-@GST_API_VERSION@/gst/drm

libgstdrm_@GST_API_VERSION@include_HEADERS = \
	gstdrmallocator.h

libgstdrm_@GST_API_VERSION@_la_CFLAGS = \
	$(DRM_CFLAGS) \
	$(TCCDRM_CFLAGS) \
	$(GST_PLUGINS_BAD_CFLAGS) \
	$(GST_PLUGINS_BASE_CFLAGS) \
	-DGST_USE_UNSTABLE_API \
	$(GST_CFLAGS)

libgstdrm_@GST_API_VERSION@_la_LIBADD = \
	$(DRM_LIBS) \
	$(GST_PLUGINS_BASE_LIBS) \
	$(GST_BASE_LIBS) \
        -lgstallocators-$(GST_API_VERSION) \
	$(GST_LIBS)

libgstdrm_@GST_API_VERSION@_la_LDFLAGS = \
	$(DRM_LDFLAGS) \
	$(GST_LIB_LDFLAGS) \
	$(GST_ALL_LDFLAGS) \
	$(GST_LT_LDFLAGS)

