/* GStreamer
 *
 * Copyright (C) 2014 Samsung Electronics. All rights reserved.
 *   Author: Thiago Santos <thiagoss@osg.samsung.com>
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

#ifndef _GST_ADAPTIVE_DEMUX_H_
#define _GST_ADAPTIVE_DEMUX_H_

#include <gst/gst.h>
#include <gst/base/gstadapter.h>

G_BEGIN_DECLS

#define GST_TYPE_ADAPTIVE_DEMUX \
  (gst_adaptive_demux_get_type())
#define GST_ADAPTIVE_DEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ADAPTIVE_DEMUX,GstAdaptiveDemux))
#define GST_ADAPTIVE_DEMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_ADAPTIVE_DEMUX,GstAdaptiveDemuxClass))
#define GST_ADAPTIVE_DEMUX_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj),GST_TYPE_ADAPTIVE_DEMUX,GstAdaptiveDemuxClass))
#define GST_IS_ADAPTIVE_DEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ADAPTIVE_DEMUX))
#define GST_IS_ADAPTIVE_DEMUX_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_ADAPTIVE_DEMUX))
#define GST_ADAPTIVE_DEMUX_CAST(obj) ((GstAdaptiveDemux *)obj)

#define GST_ADAPTIVE_DEMUX_STREAM_CAST(obj) ((GstAdaptiveDemuxStream *)obj)

/**
 * GST_ADAPTIVE_DEMUX_SINK_NAME:
 *
 * The name of the templates for the sink pad.
 */
#define GST_ADAPTIVE_DEMUX_SINK_NAME    "sink"

/**
 * GST_ADAPTIVE_DEMUX_SINK_PAD:
 * @obj: a #GstAdaptiveDemux
 *
 * Gives the pointer to the sink #GstPad object of the element.
 */
#define GST_ADAPTIVE_DEMUX_SINK_PAD(obj)        (((GstAdaptiveDemux *) (obj))->sinkpad)

#define GST_ADAPTIVE_DEMUX_STREAM_PAD(obj)      (((GstAdaptiveDemuxStream *) (obj))->pad)

#define GST_ADAPTIVE_DEMUX_STREAM_NEED_HEADER(obj) (((GstAdaptiveDemuxStream *) (obj))->need_header)

#define STATISTICS_MESSAGE_NAME "adaptive-streaming-statistics"

#define GST_MANIFEST_GET_LOCK(d) (&(GST_ADAPTIVE_DEMUX_CAST(d)->manifest_lock))
#define GST_MANIFEST_LOCK(d) (g_mutex_lock (GST_MANIFEST_GET_LOCK (d)))
#define GST_MANIFEST_UNLOCK(d) (g_mutex_unlock (GST_MANIFEST_GET_LOCK (d)))

#define GST_ELEMENT_ERROR_FROM_ERROR(el, msg, err) G_STMT_START { \
  gchar *__dbg = g_strdup_printf ("%s: %s", msg, err->message);         \
  GST_WARNING_OBJECT (el, "error: %s", __dbg);                          \
  gst_element_message_full (GST_ELEMENT(el), GST_MESSAGE_ERROR,         \
    err->domain, err->code,                                             \
    NULL, __dbg, __FILE__, GST_FUNCTION, __LINE__);                     \
  g_clear_error (&err); \
} G_STMT_END

typedef struct _GstAdaptiveDemuxStreamFragment GstAdaptiveDemuxStreamFragment;
typedef struct _GstAdaptiveDemuxStream GstAdaptiveDemuxStream;
typedef struct _GstAdaptiveDemux GstAdaptiveDemux;
typedef struct _GstAdaptiveDemuxClass GstAdaptiveDemuxClass;
typedef struct _GstAdaptiveDemuxPrivate GstAdaptiveDemuxPrivate;

struct _GstAdaptiveDemuxStreamFragment
{
  GstClockTime timestamp;
  GstClockTime duration;

  gchar *uri;
  gint64 range_start;
  gint64 range_end;

  /* when headers are needed */
  gchar *header_uri;
  gint64 header_range_start;
  gint64 header_range_end;

  /* when index is needed */
  gchar *index_uri;
  gint64 index_range_start;
  gint64 index_range_end;
};

struct _GstAdaptiveDemuxStream
{
  GstPad *pad;

  GstAdaptiveDemux *demux;

  GstSegment segment;

  GstAdapter *adapter;

  GstCaps *pending_caps;
  GstEvent *pending_segment;
  GstTagList *pending_tags;
  gboolean need_header;

  GstFlowReturn last_ret;
  GError *last_error;

  GstTask *download_task;
  GRecMutex download_lock;

  gboolean restart_download;
  gboolean discont;

  gboolean downloading_header;
  gboolean downloading_index;

  /* download tooling */
  GstElement *src;
  GstPad *src_srcpad;
  GMutex fragment_download_lock;
  GCond fragment_download_cond;
  gboolean download_finished;
  gboolean starting_fragment;
  gboolean first_fragment_buffer;
  gint64 download_start_time;
  gint64 download_chunk_start_time;
  gint64 download_total_time;
  gint64 download_total_bytes;
  gint current_download_rate;

  GstAdaptiveDemuxStreamFragment fragment;

  guint download_error_count;

  /* TODO check if used */
  gboolean eos;
};

/**
 * GstAdaptiveDemux:
 *
 * The opaque #GstAdaptiveDemux data structure.
 */
struct _GstAdaptiveDemux
{
  /*< private >*/
  GstBin     bin;

  gsize stream_struct_size;

  /*< protected >*/
  GstPad         *sinkpad;

  GstAdaptiveDemuxPrivate *priv;

  GList *streams;
  GList *next_streams;

  GstSegment segment;

  gboolean cancelled;

  GMutex manifest_lock;
  GCond manifest_cond;

  gchar *manifest_uri;
  gchar *manifest_base_uri;

  gboolean have_group_id;
  guint group_id;
};

/**
 * GstAdaptiveDemuxClass:
 *
 */
struct _GstAdaptiveDemuxClass
{
  /*< private >*/
  GstBinClass bin_class;

  /*< public >*/

  /**
   * process_manifest: Parse the manifest
   * @demux: #GstAdaptiveDemux
   * @manifest: the manifest to be parsed
   *
   * Parse the manifest and add the created streams using
   * gst_adaptive_demux_stream_new()
   *
   * Returns: #TRUE if successful
   */
  gboolean      (*process_manifest) (GstAdaptiveDemux * demux, GstBuffer * manifest);

  /**
   * get_manifest_update_interval:
   * @demux: #GstAdaptiveDemux
   *
   * Used during live streaming, the subclass should return the interval
   * between successive manifest updates
   *
   * Returns: the update interval in microseconds
   */
  gint64        (*get_manifest_update_interval) (GstAdaptiveDemux * demux);
  /**
   * update_manifest:
   * @demux: #GstAdaptiveDemux
   * @buf: Downloaded manifest data
   *
   * During live streaming, this will be called for the subclass to update its
   * manifest with the new version
   *
   * Returns: #GST_FLOW_OK is all succeeded, #GST_FLOW_EOS if the stream ended
   *          or #GST_FLOW_ERROR if an error happened
   */
  GstFlowReturn (*update_manifest) (GstAdaptiveDemux * demux, GstBuffer * buf);

  gboolean      (*is_live)          (GstAdaptiveDemux * demux);
  GstClockTime  (*get_duration)     (GstAdaptiveDemux * demux);

  /**
   * reset:
   * @demux: #GstAdaptiveDemux
   *
   * Reset the internal state of the subclass, getting ready to restart with
   * a new stream afterwards
   */
  void          (*reset)            (GstAdaptiveDemux * demux);

  /**
   * seek:
   * @demux: #GstAdaptiveDemux
   * @seek: a seek #GstEvent
   *
   * The demuxer should seek on all its streams to the specified position
   * in the seek event
   *
   * Returns: #TRUE if successful
   */
  gboolean      (*seek)             (GstAdaptiveDemux * demux, GstEvent * seek);

  /**
   * has_next_period:
   * @demux: #GstAdaptiveDemux
   *
   * Checks if there is a next period following the current one.
   * DASH can have multiple medias chained in its manifest, when one finishes
   * this function is called to verify if there is a new period to be played
   * in sequence.
   *
   * Returns: #TRUE if there is another period
   */
  gboolean      (*has_next_period)  (GstAdaptiveDemux * demux);
  /**
   * advance_period:
   * @demux: #GstAdaptiveDemux
   *
   * Advances the manifest to the next period. New streams should be created
   * using gst_adaptive_demux_stream_new().
   */
  void          (*advance_period)  (GstAdaptiveDemux * demux);

  void          (*stream_free)     (GstAdaptiveDemuxStream * stream);
  GstFlowReturn (*stream_seek)     (GstAdaptiveDemuxStream * stream, GstClockTime ts);
  gboolean      (*stream_has_next_fragment)  (GstAdaptiveDemuxStream * stream);
  GstFlowReturn (*stream_advance_fragment) (GstAdaptiveDemuxStream * stream);
  /**
   * stream_update_fragment_info:
   * @stream: #GstAdaptiveDemuxStream
   *
   * Requests the stream to set the information about the current fragment to its
   * current fragment struct
   *
   * Returns: #GST_FLOW_OK in success, #GST_FLOW_ERROR on error and #GST_FLOW_EOS
   *          if there is no fragment.
   */
  GstFlowReturn (*stream_update_fragment_info) (GstAdaptiveDemuxStream * stream);
  /**
   * stream_select_bitrate:
   * @stream: #GstAdaptiveDemuxStream
   * @bitrate: the bitrate to select (in bytes per second)
   *
   * The stream should try to select the bitrate that is the greater, but not
   * greater than the requested bitrate. If it needs a codec change it should
   * create the new stream using gst_adaptive_demux_stream_new(). If it only
   * needs a caps change it should set the new caps using
   * gst_adaptive_demux_stream_set_caps().
   *
   * Returns: #TRUE if the stream changed bitrate, #FALSE otherwise
   */
  gboolean      (*stream_select_bitrate) (GstAdaptiveDemuxStream * stream, guint64 bitrate);
  /**
   * stream_get_fragment_waiting_time:
   * @stream: #GstAdaptiveDemuxStream
   *
   * For live streams, requests how much time should be waited before starting
   * to download the fragment. This is useful to avoid downloading a fragment that
   * isn't available yet.
   *
   * Returns: The waiting time in microsseconds
   */
  gint64        (*stream_get_fragment_waiting_time) (GstAdaptiveDemuxStream * stream);

  /**
   * start_fragment:
   * @demux: #GstAdaptiveDemux
   * @stream: #GstAdaptiveDemuxStream
   *
   * Notifies the subclass that the given stream is starting the download
   * of a new fragment. Can be used to reset/init internal state that is
   * needed before each fragment, like decryption engines.
   *
   * Returns: #TRUE if successful.
   */
  gboolean      (*start_fragment) (GstAdaptiveDemux * demux, GstAdaptiveDemuxStream * stream);
  /**
   * finish_fragment:
   * @demux: #GstAdaptiveDemux
   * @stream: #GstAdaptiveDemuxStream
   *
   * Notifies the subclass that a fragment download was finished.
   * It can be used to cleanup internal state after a fragment and
   * also push any pending data before moving to the next fragment.
   */
  GstFlowReturn (*finish_fragment) (GstAdaptiveDemux * demux, GstAdaptiveDemuxStream * stream);
  /**
   * data_received:
   * @demux: #GstAdaptiveDemux
   * @stream: #GstAdaptiveDemuxStream
   *
   * Notifies the subclass that a fragment chunk was downloaded. The subclass
   * can look at the data at the adapter and modify/push data as desired.
   *
   * Returns: #GST_FLOW_OK if successful, #GST_FLOW_ERROR in case of error.
   */
  GstFlowReturn (*data_received) (GstAdaptiveDemux * demux, GstAdaptiveDemuxStream * stream);

  /**
   * get_live_seek_range:
   * @demux: #GstAdaptiveDemux
   * @start: pointer to put the start position allowed to seek to
   * @stop: pointer to put the stop position allowed to seek to
   *
   * Gets the allowed seek start and stop positions for the current live stream
   *
   * Return: %TRUE if successful
   */
  gboolean (*get_live_seek_range) (GstAdaptiveDemux * demux, gint64 * start, gint64 * stop);
};

GType    gst_adaptive_demux_get_type (void);

void     gst_adaptive_demux_set_stream_struct_size (GstAdaptiveDemux * demux,
                                                    gsize struct_size);


GstAdaptiveDemuxStream *gst_adaptive_demux_stream_new (GstAdaptiveDemux * demux,
                                                       GstPad * pad);
void gst_adaptive_demux_stream_set_caps (GstAdaptiveDemuxStream * stream,
                                         GstCaps * caps);
void gst_adaptive_demux_stream_set_tags (GstAdaptiveDemuxStream * stream,
                                         GstTagList * tags);
void gst_adaptive_demux_stream_fragment_clear (GstAdaptiveDemuxStreamFragment * f);

GstFlowReturn gst_adaptive_demux_stream_push_buffer (GstAdaptiveDemuxStream * stream, GstBuffer * buffer);
GstFlowReturn
gst_adaptive_demux_stream_advance_fragment (GstAdaptiveDemux * demux,
    GstAdaptiveDemuxStream * stream, GstClockTime duration);
GstFlowReturn
gst_adaptive_demux_stream_advance_fragment_unlocked (GstAdaptiveDemux * demux,
    GstAdaptiveDemuxStream * stream, GstClockTime duration);

G_END_DECLS

#endif
