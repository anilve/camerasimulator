#include <gst/gst.h>
#include <glib.h>
#include <stdlib.h>

/* This function is a callback for the bus to track all changes */
static gboolean
bus_call (GstBus     *bus,
          GstMessage *msg,
          gpointer    data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_ERROR: {
      gchar  *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);

      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }

  return TRUE;
}



int main(int argc, char *argv[]) 
{
    // GLib main loop, which is a fundamental component for handling events and managing the lifecycle 
    // of a GStreamer application
    GMainLoop *loop;

    GstElement *pipeline;
    GstElement *src, *decodebin, *sink;

    GstBus *bus;
    guint bus_watch_id;

      /* Check input arguments */
    if (argc != 2) {
        g_printerr ("Usage: %s <Video File>\n", argv[0]);
        return -1;
     }

    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Create the main loop
    loop = g_main_loop_new(NULL, FALSE); 

    // Create the pipeline element
    pipeline = gst_pipeline_new("my-video-pipeline");

    if (!pipeline) {
        g_printerr("Could not create pipeline\n");
        return -1;
    }

    // Source element (input file)
    src = gst_element_factory_make("filesrc", "source");
    if (!src) {
        g_printerr("Could not create source element\n");
        goto error;
    }
    /* we set the input filename to the source element */
    g_object_set(src, "location", argv[1], NULL);

    // Decoder element
    decodebin = gst_element_factory_make("decodebin", "decoder");
    if (!decodebin) {
        g_printerr("Could not create decodebin element\n");
        goto error;
    }

    // Sink element (RTSP output)
    sink = gst_element_factory_make("rtspsink", "sink");
    if (!sink) {
        g_printerr("Could not create sink element\n");
        goto error;
    }
    g_object_set(sink, "location", "rtsp://localhost:554/live/stream1", NULL);

    // Create the bin
    GstBin *bin = GST_BIN(pipeline);
    gst_bin_add_many(bin, src, decodebin, sink, NULL);

    /* we add a message handler */
    bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
    gst_object_unref (bus);

    // Link elements
    if (!gst_element_link(src, decodebin)) {
        g_printerr("Could not link source to decoder\n");
        goto error;
    }
    if (!gst_element_link(decodebin, sink)) {
        g_printerr("Could not link decoder to sink\n");
        goto error;
    }

    // Play the pipeline
    g_print ("Now playing: %s\n", argv[1]);

    GstStateChangeReturn ret;
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Could not start playing pipeline\n");
        goto error;
    }

    /* Iterate */
    g_print ("Running...\n");
    g_main_loop_run (loop);


    /* Out of the main loop, clean up nicely */
    g_print ("Returned, stopping playback\n");
    gst_element_set_state (pipeline, GST_STATE_NULL);

    g_print ("Deleting pipeline\n");
    gst_object_unref (GST_OBJECT (pipeline));
    g_source_remove (bus_watch_id);
    g_main_loop_unref (loop);

    return 0;

error:
    gst_object_unref(pipeline);
    return 0;
}

// gcc mp4videoplaback.c -o mp4videortsp `pkg-config --cflags --libs gstreamer-1.0`
