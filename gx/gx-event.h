#ifndef _GX_EVENT_H_
#define _GX_EVENT_H_

#include <glib.h>

#include <gx/generated-code/gx-xcb-dependencies-gen.h>

typedef struct _GXGenericEvent {
    guint8  type;	    /**< Type of the event */
    guint8  pad0;           /**< Padding */
    guint16 sequence;       /**< Sequence number */
    guint32 pad[7];         /**< Padding */
    guint32 full_sequence;  /**< Full sequence */
} GXGenericEvent;

/* FIXME - Should this be made private to the extension libraries? */
typedef struct {
    int		protocol_event_code;
    const char *description;
    size_t	window_xid_offset;
} GXEventDetails;


void
gx_event_details_add_extension (GXEventDetails *extension_event_details);

void
_gx_event_details_hash_free (void);

const char *
gx_event_get_name (GXGenericEvent *event);

GXGenericEvent *
gx_event_from_xcb_event (xcb_generic_event_t *event);

guint32
gx_event_get_window_xid (GXGenericEvent *event);

#endif /* _GX_EVENT_H_ */

