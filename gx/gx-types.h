#ifndef _GX_TYPES_H_
#define _GX_TYPES_H_

#include <xcb/xcb.h>

//typedef guint32 GXColorMap;

typedef xcb_generic_reply_t GXGenericReply;
typedef xcb_generic_error_t GXGenericProtocolError;

#if !defined (GX_COOKIE_TYPEDEF)
typedef struct _GXCookie	GXCookie;
#define GX_COOKIE_TYPEDEF
#endif

#ifndef GX_CONNECTION_TYPEDEF
typedef struct _GXConnection	GXConnection;
#define GX_CONNECTION_TYPEDEF
#endif

#ifndef GX_SCREEN_TYPEDEF
typedef struct _GXScreen	GXScreen;
#define GX_SCREEN_TYPEDEF
#endif

#ifndef GX_DRAWABLE_TYPEDEF
typedef struct _GXDrawable	GXDrawable;
#define GX_DRAWABLE_TYPEDEF
#endif

#ifndef GX_PIXMAP_TYPEDEF
typedef struct _GXPixmap	GXPixmap;
#define GX_PIXMAP_TYPEDEF
#endif

#ifndef GX_WINDOW_TYPEDEF
typedef struct _GXWindow        GXWindow;
#define GX_WINDOW_TYPEDEF
#endif

#ifndef GX_GCONTEXT_TYPEDEF
typedef struct _GXGContext	GXGContext;
#define GX_GCONTEXT_TYPEDEF
#endif

#endif /* _GX_TYPES_H_ */

