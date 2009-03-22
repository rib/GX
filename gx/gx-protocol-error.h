#ifndef _GX_PROCOCOL_ERROR_H_
#define _GX_PROCOCOL_ERROR_H_

#include <glib.h>

#include <gx/generated-code/gx-xcb-dependencies-gen.h>

#define GX_PROTOCOL_ERROR gx_protocol_error_quark ()

GQuark gx_protocol_error_quark (void);

typedef	int GXProtocolError;

typedef struct {
    int		     protocol_error_code;
    GXProtocolError  gx_protocol_error;
    const char	    *description;
} GXProtocolErrorDetails;


const char *
gx_protocol_error_get_description (GXProtocolError code);

GXProtocolError
gx_protocol_error_from_xcb_error (xcb_generic_error_t *error);

#endif /* _GX_PROCOCOL_ERROR_H_ */

