
#include <gx/gx-protocol-error.h>

#include <glib.h>


static GHashTable *protocol_error_details_hash = NULL;

GQuark
gx_protocol_error_quark (void)
{
  return g_quark_from_static_string ("gx-protocol-error-quark");
}

const char *
gx_protocol_error_get_description (GXProtocolError code)
{
  GXProtocolErrorDetails *details;

  if (!protocol_error_details_hash)
    return "Unknown error";

  details = g_hash_table_lookup (protocol_error_details_hash,
				 &code);
  if (details)
    return details->description;
  else
    return "Unknown error";
}

void
gx_protocol_error_details_add_extension (
		      GXProtocolErrorDetails *extension_error_details)
{
  GXProtocolErrorDetails *details;
  static		  guint gx_protocol_error = 1;

  if (!protocol_error_details_hash)
    protocol_error_details_hash = g_hash_table_new (g_int_hash, g_int_equal);

  for (details = extension_error_details;
       details->description != NULL;
       details++)
    {
      details->gx_protocol_error = gx_protocol_error++;
      g_hash_table_insert (protocol_error_details_hash,
			   &details->protocol_error_code,
			   details);
    }
}

void
_gx_protocol_error_details_hash_free (void)
{
  if (protocol_error_details_hash)
    {
      g_hash_table_unref (protocol_error_details_hash);
      protocol_error_details_hash = NULL;
    }
}

/* FIXME - we should probably choose a specific default when the error code
 * is unknown */
GXProtocolError
gx_protocol_error_from_xcb_error (xcb_generic_error_t *error)
{
  GXProtocolErrorDetails *details;

  if (!protocol_error_details_hash)
    return 0;

  details = g_hash_table_lookup (protocol_error_details_hash,
				 &error->error_code);
  if (details)
    return details->gx_protocol_error;
  else
    return 0;
}

