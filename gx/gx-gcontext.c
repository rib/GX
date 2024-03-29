/*
 * vim: tabstop=8 shiftwidth=2 noexpandtab softtabstop=2 cinoptions=>2,{2,:0,t0,(0,W4
 *
 * <copyright_assignments>
 * Copyright (C) 2008  Robert Bragg
 * </copyright_assignments>
 *
 * <license>
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 * </license>
 *
 */

#include <gx/gx-connection.h>
#include <gx/gx-gcontext.h>
#include <gx/gx-drawable.h>

#include <string.h>

/* Macros and defines */
#define GX_GCONTEXT_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GX_TYPE_GCONTEXT, GXGContextPrivate))

#if 0
enum {
    SIGNAL_NAME,
    LAST_SIGNAL
};
#endif

enum {
    PROP_0,
    PROP_CONNECTION,
    PROP_XID,
    PROP_DRAWABLE,
    PROP_COMPONENT_VALUES
};

struct _GXGContextPrivate
{
  GXConnection	  *connection;
  guint32	   xid;

  GXDrawable	  *drawable_construct;
  GXMaskValueItem *component_values_construct;
};

static void gx_gcontext_get_property(GObject *object,
				   guint id,
				   GValue *value,
				   GParamSpec *pspec);
static void gx_gcontext_set_property(GObject *object,
				   guint property_id,
				   const GValue *value,
				   GParamSpec *pspec);
/* static void gx_gcontext_mydoable_interface_init(gpointer interface,
   gpointer data); */
static void gx_gcontext_init (GXGContext *self);
static void gx_gcontext_constructed (GObject *self);
static void gx_gcontext_finalize (GObject *self);


/* static guint gx_gcontext_signals[LAST_SIGNAL] = { 0 }; */

G_DEFINE_TYPE(GXGContext, gx_gcontext, G_TYPE_OBJECT);


static void
gx_gcontext_class_init (GXGContextClass *klass) /* Class Initialization */
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GParamSpec   *new_param;

  gobject_class->constructed = gx_gcontext_constructed;
  gobject_class->finalize = gx_gcontext_finalize;

  gobject_class->get_property = gx_gcontext_get_property;
  gobject_class->set_property = gx_gcontext_set_property;

  new_param = g_param_spec_object ("connection", /* name */
				   "Connection",	/* nick name */
				   "Connection",	/* description */
				   GX_TYPE_CONNECTION,	/* GType */
				   G_PARAM_READABLE	/* flags */
				   | G_PARAM_WRITABLE	/* flags */
				   | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (gobject_class, PROP_CONNECTION, new_param);

  new_param = g_param_spec_uint ("xid", /* name */
			        "XID",	/* nick name */
			        "XID to send when creating a window",
			        0,	/* minimum */
			        G_MAXUINT32,	/* maximum */
			        0,	/* default */
			        G_PARAM_WRITABLE
			        | G_PARAM_READABLE
			        | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (gobject_class, PROP_XID, new_param);

  new_param = g_param_spec_object ("drawable", /* name */
				   "Drawable",	/* nick name */
				   "Reference Drawable",	/* description */
				   GX_TYPE_DRAWABLE,	/* GType */
				   G_PARAM_WRITABLE	/* flags */
				   | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (gobject_class, PROP_DRAWABLE, new_param);

  new_param = g_param_spec_pointer ("component_values", /* name */
			            "Component Mask Value Items",
			            "An array of initial component values as "
				    "Mask Value Items",
				    G_PARAM_WRITABLE
				    | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (gobject_class, PROP_COMPONENT_VALUES,
				   new_param);

  /* set up signals */
#if 0 /* template code */
  klass->signal_member = signal_default_handler;
  gx_gcontext_signals[SIGNAL_NAME] =
    g_signal_new("signal_name", /* name */
		 G_TYPE_FROM_CLASS(klass), /* interface GType */
		 G_SIGNAL_RUN_LAST, /* signal flags */
		 G_STRUCT_OFFSET(GXGContextClass, signal_member),
		 NULL, /* accumulator */
		 NULL, /* accumulator data */
		 g_cclosure_marshal_VOID__VOID, /* c marshaller */
		 G_TYPE_NONE, /* return type */
		 0 /* number of parameters */
		 /* vararg, list of param types */
    );
#endif

  g_type_class_add_private (klass, sizeof(GXGContextPrivate));
}

static void
gx_gcontext_get_property (GObject *object,
		          guint id,
		          GValue *value,
		          GParamSpec *pspec)
{
  GXGContext* self = GX_GCONTEXT (object);

  switch (id)
    {
#if 0 /* template code */
    case PROP_NAME:
      g_value_set_int(value, self->priv->property);
      break;
#endif
    case PROP_CONNECTION:
      g_value_set_object (value, self->priv->connection);
      break;
    case PROP_XID:
      g_value_set_uint (value, self->priv->xid);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
      break;
    }
}

static void
gx_gcontext_set_property (GObject *object,
		          guint property_id,
		          const GValue *value,
		          GParamSpec *pspec)
{
  GXGContext* self = GX_GCONTEXT (object);

  switch (property_id)
    {
#if 0 /* template code */
    case PROP_NAME:
      gx_gcontext_set_property(self, g_value_get_int(value));
      break;
#endif
    case PROP_CONNECTION:
      self->priv->connection = g_value_get_object (value);
      break;
    case PROP_XID:
      self->priv->xid = g_value_get_uint (value);
      break;
    case PROP_DRAWABLE:
      self->priv->drawable_construct = g_value_get_object (value);
      break;
    case PROP_COMPONENT_VALUES:
      self->priv->component_values_construct = g_value_get_pointer (value);
      break;
    default:
      g_warning ("gx_gcontext_set_property on unknown property");
      return;
    }
}

#if 0 /* template code */
static void
gx_gcontext_mydoable_interface_init(gpointer interface,
				  gpointer data)
{
  MyDoableIface *mydoable = interface;
  g_assert(G_TYPE_FROM_INTERFACE(mydoable) == MY_TYPE_MYDOABLE);

  mydoable->method1 = gx_gcontext_method1;
  mydoable->method2 = gx_gcontext_method2;
}
#endif

static void
gx_gcontext_init (GXGContext *self)
{
  self->priv = GX_GCONTEXT_GET_PRIVATE (self);

  self->priv->connection = NULL;
  self->priv->xid = 0;
  self->priv->drawable_construct = NULL;
  self->priv->component_values_construct = NULL;
}

static void
gx_gcontext_constructed (GObject *object)
{
  GXGContext *self = GX_GCONTEXT (object);
  GXConnection *connection = gx_gcontext_get_connection (self);
  xcb_connection_t *xcb_connection =
    gx_connection_get_xcb_connection (connection);

  guint32 value_list_len = 0;
  guint32 *value_list = NULL;
  guint32 value_mask = 0;

  if (self->priv->component_values_construct)
    {
      value_list_len =
	gx_mask_value_items_get_count (
	    self->priv->component_values_construct);
      value_list = alloca (value_list_len * 4);

      gx_mask_value_items_get_list (
	  self->priv->component_values_construct,
	  &value_mask,
	  value_list);
    }

  xcb_create_gc (xcb_connection,
		 xcb_generate_id (xcb_connection),
		 self->priv->drawable_construct,
		 value_mask,
		 value_list);

  g_object_unref (connection);

  /* G_OBJECT_CLASS (gx_gcontext_parent_class)->constructed (object); */
}

GXGContext*
gx_gcontext_new (GXConnection *connection,
		 GXDrawable *drawable,
		 GXMaskValueItem *component_values)
{
  return GX_GCONTEXT (g_object_new (GX_TYPE_GCONTEXT,
				    "connection", connection,
				    "drawable", drawable,
				    "component_values", component_values,
				    NULL));
}

void
gx_gcontext_finalize (GObject *object)
{
  /* GXGContext *self = GX_GCONTEXT(object); */

  /* destruct your object here */
  G_OBJECT_CLASS (gx_gcontext_parent_class)->finalize (object);
}

GXConnection *
gx_gcontext_get_connection (GXGContext *self)
{
  return g_object_ref (self->priv->connection);
}

guint32
gx_gcontext_get_xid (GXGContext *self)
{
    return self->priv->xid;
}

