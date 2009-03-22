
#include <gx/gx-event.h>
#include <gx/gx-connection.h>

#include <glib.h>

static GHashTable *event_details_hash = NULL;

void
gx_event_details_add_extension (GXEventDetails *extension_event_details)
{
  GXEventDetails *details;

  if (!event_details_hash)
    event_details_hash = g_hash_table_new (g_int_hash, g_int_equal);

  for (details = extension_event_details;
       details->description != NULL;
       details++)
    {
      g_hash_table_insert (event_details_hash,
			   &details->protocol_event_code,
			   details);
    }
}

void
_gx_event_details_hash_free (void)
{
  if (event_details_hash)
    {
      g_hash_table_unref (event_details_hash);
      event_details_hash = NULL;
    }
}

const char *
gx_event_get_name (GXGenericEvent *event)
{
  GXEventDetails *details;
  gint key;

  if (!event_details_hash)
    return "Unknown event";

  key = event->type;
  details = g_hash_table_lookup (event_details_hash,
				 &key);
  if (details)
    return details->description;
  else
    return "Unknown event";
}

guint32
gx_event_get_window_xid (GXGenericEvent *event)
{
  GXEventDetails *details;
  guint8 *event_buf = (guint8 *)event;
  gint key;

  if (!event_details_hash)
    event_details_hash = g_hash_table_new (g_int_hash, g_int_equal);

  key = event->type;
  details = g_hash_table_lookup (event_details_hash,
				 &key);
  if (!details)
    {
      g_warning ("gx_event_get_window_xid: failed to lookup event details\n");
      return 0;
    }

  if (!details->window_xid_offset)
    return 0;

  return *(guint32 *)(event_buf + details->window_xid_offset);
}

GXGenericEvent *
gx_event_from_xcb_event (xcb_generic_event_t *event)
{
  /* We don't want to be too strict in case the client is interacting
   * with funky new extensions with unknown event types... */
  /* g_assert ( g_hash_table_lookup (event->response_type) ); */

  return (GXGenericEvent *)event;
}

