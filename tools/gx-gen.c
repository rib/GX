/* GXGen - Glib/GObject style XCB binding generator
 *
 * Copyright (C) 2004-2005 Josh Triplett
 * Copyright (C) 2008 Robert Bragg
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This package is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 */

#include <xgen.h>
#include <libxml/parser.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <stdarg.h>
#include <string.h>

typedef struct _GXGenPart
{
  char *name;
  GString *string;
} GXGenPart;


typedef enum {
  GXGEN_OBJECT_TYPE_CONNECTION = 1,
  GXGEN_OBJECT_TYPE_DRAWABLE,
  GXGEN_OBJECT_TYPE_PIXMAP,
  GXGEN_OBJECT_TYPE_WINDOW,
  GXGEN_OBJECT_TYPE_GCONTEXT
} GXGenObjectType;

typedef struct _GXGenObject {
  GXGenObjectType  type;
  const char	  *name_cc;
  const char	  *name_lc;
  const char	  *first_arg;
} GXGenObject;

typedef struct _GXGenNamespace
{
  GList *gx_words; /* list of CamelCase words */

  /* Most of the time this list is empty, and so the gx_words are
   * shared, but there are some special cases, such as for the
   * GetProperty/SetProperty requests */
  GList *xcb_words; /* list of CamelCase words */
} GXGenNamespace;

typedef struct _GXGenDefinition
{
  GXGenNamespace *namespace;
  const GXGenObject *object;

  /* Only valid for XGEN_REQUEST definitions... */
  const XGenFieldDefinition *first_object_field;
} GXGenDefinition;

typedef struct _GXGenOutputContext
{
  const XGenState	       *state;
  GList			       *parts;
  const XGenExtension	       *extension;
  const XGenRequest	       *out_request;
  char			       *typedefs_part;
  char			       *protos_part;
  char			       *c_part;
} GXGenOutputContext;

#define BASE_TYPE(NAME, TYPE, SIZE) \
  { \
   ._parent = { \
      .name = NAME, \
      .type = TYPE \
    }, \
    .size = SIZE \
  }
static const XGenBaseType core_type_definitions[] = {
  BASE_TYPE("void", XGEN_VOID, 0),
  BASE_TYPE("char", XGEN_CHAR, 1),
  BASE_TYPE("float", XGEN_FLOAT, 4),
  BASE_TYPE("double", XGEN_DOUBLE, 8),
  BASE_TYPE("BOOL", XGEN_BOOLEAN, 1),
  BASE_TYPE("BYTE", XGEN_UNSIGNED, 1),
  BASE_TYPE("CARD8", XGEN_UNSIGNED, 1),
  BASE_TYPE("CARD16", XGEN_UNSIGNED, 2),
  BASE_TYPE("CARD32", XGEN_UNSIGNED, 4),
  BASE_TYPE("INT8", XGEN_SIGNED, 1),
  BASE_TYPE("INT16", XGEN_SIGNED, 2),
  BASE_TYPE("INT32", XGEN_SIGNED, 4)
};
#undef BASE_TYPE

struct _TypeMapping
{
  char *from;
  char *to;
};

static struct _TypeMapping core_type_mappings[] = {
  {"void", "void"},
  {"char", "gchar"},
  {"float", "gfloat"},
  {"double", "gdouble"},
  {"BOOL", "guint8"},
  {"BYTE", "guint8"},
  {"CARD8", "guint8"},
  {"CARD16", "guint16"},
  {"CARD32", "guint32"},
  {"INT8", "gint8"},
  {"INT16", "gint16"},
  {"INT32", "gint32"},

  {NULL}
};

struct _CamelCaseMapping
{
  char *uppercase;
  char *camelcase;
};

/* We need to use a simple dictionary to convert various uppercase
 * X protocol types into CamelCase.
 *
 * NB: replacements are done in order from top to bottom so put
 * shorter words at the bottom.
 *
 * NB: replacements must have the same length.
 */
static struct _CamelCaseMapping camelcase_dictionary[] = {
  {"RECTANGLE","Rectangle"},
  {"TIMESTAMP","Timestamp"},
  {"COLORMAP","Colormap"},
  {"COLORITEM","Coloritem"},
  {"FONTABLE","Fontable"},
  /* {"GLYPHSET","GlyphSet"}, */
  {"CONTEXT","Context"},
  {"COUNTER","Counter"},
  {"PICTURE","Picture"},
  {"SEGMENT","Segment"},
  {"TRIGGER","Trigger"},
  {"BUTTON","Button"},
  {"CURSOR","Cursor"},
  {"DIRECT","Direct"},
  {"FORMAT","Format"},
  {"REGION","Region"},
  {"SCREEN","Screen"},
  {"SYSTEM","System"},
  {"VISUAL","Visual"},
  {"DEPTH","Depth"},
  {"FIXED","Fixed"},
  {"GLYPH","Glyph"},
  {"POINT","Point"},
  {"VALUE","Value"},
  {"ATOM","Atom"},
  {"CHAR","Char"},
  {"CODE","Code"},
  {"FONT","Font"},
  {"FORM","Form"},
  {"HOST","Host"},
  {"KIND","Kind"},
  {"INFO","Info"},
  {"LINE","Line"},
  {"PICT","Pict"},
  {"PROP","Prop"},
  {"SPAN","Span"},
  {"TEST","Test"},
  {"TYPE","Type"},
  {"ARC","Arc"},
  {"FIX","Fix"},
  {"INT","Int"},
  {"KEY","Key"},
  {"MAP","Map"},
  {"SET","Set"},
  {"SYM","Sym"},
  {NULL}
};

#define GXGEN_WORD_SPLIT_REGEX \
  "("\
  "DECnet|" /* special case */ \
  "[A-Z0-9][a-z]+|" \
  "[A-Z0-9]+(?![a-z])|" \
  "[a-z]+|" \
  ")"

/* Most extension names are considered to be one word. The following
 * exceptions are split into words using the GXGEN_WORD_SPLIT_REGEX
 */
static const char *special_extension_names[] = {
  "XPrint",
  "XCMisc",
  "BigRequests"
};

static const GXGenObject gxgen_object_descriptions[] = {
  {
    .type = GXGEN_OBJECT_TYPE_CONNECTION,
    .name_cc = "Connection",
    .name_lc = "connection",
    .first_arg = "GXConnection *connection",
#if 0
    .h_typedefs = GXGEN_PART_CONNECTION_OBJ_H_TYPEDEFS,
    .h_protos = GXGEN_PART_CONNECTION_OBJ_H_PROTOS,
    .c_funcs = GXGEN_PART_CONNECTION_OBJ_C_FUNCS,
#endif
  },
  {
    .type = GXGEN_OBJECT_TYPE_WINDOW,
    .name_cc = "Window",
    .name_lc = "window",
    .first_arg = "GXWindow *window",
  },
  {
    .type = GXGEN_OBJECT_TYPE_PIXMAP,
    .name_cc = "Pixmap",
    .name_lc = "pixmap",
    .first_arg = "GXPixmap *pixmap",
  },
  {
    .type = GXGEN_OBJECT_TYPE_DRAWABLE,
    .name_cc = "Drawable",
    .name_lc = "drawable",
    .first_arg = "GXDrawable *drawable",
  },
  {
    .type = GXGEN_OBJECT_TYPE_GCONTEXT,
    .name_cc = "GContext",
    .name_lc = "gcontext",
    .first_arg = "GXGContext *gc",
  },

  { 0 }
};

/**
 * Some macro sugar for outputing a formatted string to the current
 * C file
 */
#define _C(...) \
  out (output_context, output_context->c_part, __VA_ARGS__)

/**
 * Some macro sugar for outputing a formatted string to the current
 * headers typedef section
 */
#define _TD(...) \
  out (output_context, output_context->typedefs_part, __VA_ARGS__)

/**
 * Some macro sugar for outputing a formatted string to the current
 * headers function prototype section
 */
#define _H(...) \
  out (output_context, output_context->protos_part, __VA_ARGS__)

/**
 * Some macro sugar for outputing a formatted string to the current
 * C file and current headers prototype section
 */
#define _CH(...) \
  out2 (output_context, \
        output_context->c_part, \
	output_context->protos_part, \
	__VA_ARGS__)

#define _TEST_C(...) \
  out (output_context, "gxgen-tests", __VA_ARGS__)

static GRegex *cname_regex;



static GList *
gxgen_split_name (const char *camelcase_name)
{
  char *str;
  char **strv;
  int i;
  GList *words = NULL;

  g_return_val_if_fail (camelcase_name && camelcase_name[0], NULL);

  strv = g_regex_split (cname_regex, camelcase_name, 0);
  for (i = 0; strv[i]; i++)
    {
      str = strv[i];
      if (strcmp (str, "") == 0)
	continue;
      words = g_list_append (words, g_strdup (str));
    }
  g_strfreev (strv);

  return words;
}

static char *
gxgen_join_words (GList *words, char *seperator)
{
  GString *string = g_string_new ("");
  GList *tmp;

  for (tmp = words; tmp != NULL; tmp = tmp->next)
    {
      string = g_string_append (string, tmp->data);
      string = g_string_append (string, seperator);
    }

  string = g_string_set_size (string, string->len - strlen (seperator));

  return g_string_free (string, FALSE);
}

static char *
gxgen_words_to_camelcase (GList *words)
{
  return gxgen_join_words (words, "");
}

static char *
gxgen_words_to_lowercase (GList *words)
{
  char *cc = gxgen_join_words (words, "_");
  char *lc = g_ascii_strdown (cc, -1);
  g_free (cc);
  return lc;
}

static char *
gxgen_words_to_uppercase (GList *words)
{
  char *cc = gxgen_join_words (words, "_");
  char *uc = g_ascii_strup (cc, -1);
  g_free (cc);
  return uc;
}

static char *
gxgen_namespace_to_gx_name (GXGenNamespace *namespace)
{
  char *lc_words = gxgen_words_to_lowercase (namespace->gx_words);
  char *gx_name = g_strdup_printf ("gx_%s", lc_words);
  g_free (lc_words);
  return gx_name;
}

static char *
gxgen_namespace_to_gx_type (GXGenNamespace *namespace)
{
  char *cc_words = gxgen_words_to_camelcase (namespace->gx_words);
  char *gx_typedef = g_strdup_printf ("GX%s", cc_words);
  g_free (cc_words);
  return gx_typedef;
}

static char *
gxgen_namespace_to_gx_define (GXGenNamespace *namespace)
{
  char *uc_words = gxgen_words_to_uppercase (namespace->gx_words);
  char *gx_define = g_strdup_printf ("GX_%s", uc_words);
  g_free (uc_words);
  return gx_define;
}

static char *
gxgen_namespace_to_xcb_name (GXGenNamespace *namespace)
{
  GList *words =
    namespace->xcb_words ? namespace->xcb_words : namespace->gx_words;
  char *lc_words = gxgen_words_to_lowercase (words);
  char *xcb_name = g_strdup_printf ("xcb_%s", lc_words);
  g_free (lc_words);
  return xcb_name;
}

static char *
gxgen_namespace_to_xcb_type (GXGenNamespace *namespace)
{
  GList *words =
    namespace->xcb_words ? namespace->xcb_words : namespace->gx_words;
  char *lc_words = gxgen_words_to_lowercase (words);
  //char *xcb_typedef = g_strdup_printf ("xcb_%s_t", cc_words);
  char *xcb_typedef = g_strdup_printf ("xcb_%s", lc_words);
  g_free (lc_words);
  return xcb_typedef;
}

static char *
gxgen_namespace_to_anon_define (GXGenNamespace *namespace)
{
  return gxgen_words_to_uppercase (namespace->gx_words);
}

GList *
gxgen_split_extension_name (const char *extension_name)
{
  GList *words = NULL;
  int i;

  for (i = 0; special_extension_names[i]; i++)
    if (strcmp (extension_name, special_extension_names[i]) == 0)
      return gxgen_split_name (extension_name);
  words = g_list_prepend (words, g_strdup (extension_name));
  return words;
}

/* Deep copy a list of strings */
static GList *
copy_word_list (GList *words)
{
  GList *tmp;
  GList *copy = NULL;

  for (tmp = words; tmp != NULL; tmp = tmp->next)
    copy = g_list_prepend (copy, g_strdup (tmp->data));

  copy = g_list_reverse (copy);

  return copy;
}

static GXGenNamespace *
gxgen_namespace_new (const char *object_name, /* optional */
		     const XGenDefinition *def, /* optional */
		     const char *camelcase_name_fmt, /* printf style format */
		     ...)
{
  GXGenNamespace *namespace = g_new0 (GXGenNamespace, 1);
  char *camelcase_name;
  char *gx_name_cc;
  GList *gx_words = NULL;
  GList *xcb_words = NULL;
  GList *name_words;
  va_list ap;
  int i;
  char *str;

  g_assert (camelcase_name_fmt);
  va_start (ap, camelcase_name_fmt);
  camelcase_name = g_strdup_vprintf (camelcase_name_fmt, ap);
  va_end (ap);

  if (object_name)
    gx_words = g_list_append (gx_words, g_strdup (object_name));

  if (def)
    {
      gboolean xproto =
	strcmp (def->extension->header, "xproto") == 0 ? TRUE : FALSE;

      if (!xproto)
	{
	  GList *ext_words = gxgen_split_extension_name (def->extension->name);
	  gx_words = g_list_concat (gx_words, ext_words);
	  xcb_words = g_list_concat (xcb_words, copy_word_list (ext_words));
	}

      if (xproto && object_name && def->type == XGEN_REQUEST)
	{
	  if (strcmp (camelcase_name, "GetProperty") == 0)
	    gx_words =
	      g_list_concat (gx_words, gxgen_split_name ("GetXProperty"));
	  else if (strcmp (camelcase_name, "SetProperty") == 0)
	    gx_words =
	      g_list_concat (gx_words, gxgen_split_name ("SetXProperty"));
	}
    }


  gx_name_cc = g_strdup (camelcase_name);

  /* At the very least; if a lower case name is passed then we
   * uppercase the first letter: */
  /* FIXME - this should possibly also use a dictionary */
  gx_name_cc[0] = g_ascii_toupper (gx_name_cc[0]);

  for (i = 0; camelcase_dictionary[i].uppercase; i++)
    while ((str = strstr (gx_name_cc, camelcase_dictionary[i].uppercase)))
      memcpy (str, camelcase_dictionary[i].camelcase,
              strlen(camelcase_dictionary[i].uppercase));

  name_words = gxgen_split_name (gx_name_cc);
  gx_words = g_list_concat (gx_words, name_words);
  g_free (gx_name_cc);

  name_words = gxgen_split_name (camelcase_name);
  xcb_words = g_list_concat (xcb_words, name_words);

  namespace->gx_words = gx_words;
  namespace->xcb_words = xcb_words;

  return namespace;
}

static void
gxgen_namespace_free (GXGenNamespace *namespace)
{
  g_list_foreach (namespace->gx_words, (GFunc)g_free, NULL);
  g_list_free (namespace->gx_words);
  g_list_foreach (namespace->xcb_words, (GFunc)g_free, NULL);
  g_list_free (namespace->xcb_words);
  g_free (namespace);
}

static GXGenNamespace *
gxgen_namespace_copy (GXGenNamespace *namespace)
{
  GXGenNamespace *copy = g_new0 (GXGenNamespace, 1);
  GList *tmp;
  copy->gx_words = g_list_copy (namespace->gx_words);
  for (tmp = copy->gx_words; tmp != NULL; tmp = tmp->next)
    tmp->data = g_strdup (tmp->data);
  copy->xcb_words = g_list_copy (namespace->xcb_words);
  for (tmp = copy->xcb_words; tmp != NULL; tmp = tmp->next)
    tmp->data = g_strdup (tmp->data);
  return copy;
}

static void
gxgen_namespace_append (GXGenNamespace *namespace, char *camelcase_name)
{
  namespace->gx_words =
    g_list_concat (namespace->gx_words, gxgen_split_name (camelcase_name));
  namespace->xcb_words =
    g_list_concat (namespace->xcb_words, gxgen_split_name (camelcase_name));
}

static const GXGenObject *
gxgen_get_object (GXGenObjectType type)
{
  const GXGenObject *object;

  for (object = gxgen_object_descriptions; object->name_cc; object++)
    if (object->type == type)
      return object;
  return NULL;
}

static const GXGenObject *
identify_request_object (XGenRequest *request,
			 XGenFieldDefinition **first_object_field)
{
  GList *tmp;
  const GXGenObject *object = NULL;

  /* Look at the first field (after those that are ignored)
   * to identify what object this request belongs too.
   *
   * The default object for requests to become member of is
   * GXConnection.
   */
  if (g_list_length (request->fields) >= 2)
    {
      for (tmp = request->fields; tmp != NULL; tmp = tmp->next)
	{
	  XGenFieldDefinition *field = tmp->data;

	  if (strcmp (field->name, "opcode") == 0
	      || strcmp (field->name, "pad") == 0
	      || strcmp (field->name, "length") == 0)
	    continue;

	  if (strcmp (field->name, "window") == 0)
	    {
	      object = gxgen_get_object (GXGEN_OBJECT_TYPE_WINDOW);
	      *first_object_field = field;
	      break;
	    }
	  else if (strcmp (field->name, "pixmap") == 0)
	    {
	      object = gxgen_get_object (GXGEN_OBJECT_TYPE_PIXMAP);
	      *first_object_field = field;
	      break;
	    }
	  else if (strcmp (field->name, "drawable") == 0)
	    {
	      object = gxgen_get_object (GXGEN_OBJECT_TYPE_DRAWABLE);
	      *first_object_field = field;
	      break;
	    }
	  else if (strcmp (field->name, "gc") == 0)
	    {
	      object = gxgen_get_object (GXGEN_OBJECT_TYPE_GCONTEXT);
	      *first_object_field = field;
	      break;
	    }
	}
    }

  /* By default requests become members of the GXConnection
   * object */
  if (!object)
    return gxgen_get_object (GXGEN_OBJECT_TYPE_CONNECTION);

  return object;
}

/* For every XGenDefinition that XGen parses, this function is called so
 * we have an opportunity to extend the definition with private data. */
static void
gxgen_definition_notify (XGenDefinition *def)
{
  GXGenDefinition *gxgen_def = g_new0 (GXGenDefinition, 1);
  XGenFieldDefinition *first_object_field = NULL;
  const char *obj_name_cc = NULL;

  if (def->type == XGEN_REQUEST)
    {
      const GXGenObject *object;
      object = identify_request_object (XGEN_REQUEST_DEF (def),
					&first_object_field);
      gxgen_def->first_object_field = first_object_field;
      gxgen_def->object = object;
      obj_name_cc = object->name_cc;
    }

  gxgen_def->namespace =
    gxgen_namespace_new (obj_name_cc, def, def->name);

  xgen_definition_set_private (def, gxgen_def);
}

#if 0
static void
gxgen_request_notify (XGenRequest *request)
{

}

static void
gxgen_reply_notify (XGenReply *reply)
{

}

static void
gxgen_error_notify (XGenError *error)
{

}

static void
gxgen_event_notify (XGenEvent *event)
{

}

static void
gxgen_struct_notify (XGenStruct *struct_def)
{

}

static void
gxgen_xid_union_notify (XGenXIDUnion *xid_union)
{

}

static void
gxgen_union_notify (XGenUnion *union_def)
{

}

static void
gxgen_xid_def_notify (XGenBaseType *xid_def)
{

}

static void
gxgen_enum_notify (XGenEnum *enum_def)
{

}

static void
gxgen_typedef_notify (XGenTypedef *typedef_def)
{

}
#endif

static char *
gxgen_definition_to_gx_type (XGenDefinition * definition,
			     gboolean object_types)
{
  GXGenDefinition *gxgen_def = xgen_definition_get_private (definition);
  GXGenNamespace *namespace = gxgen_def->namespace;
  struct _TypeMapping *mapping;

  for (mapping = core_type_mappings; mapping->from != NULL; mapping++)
    if (strcmp (mapping->from, definition->name) == 0)
      return g_strdup (mapping->to);

  if (strcmp (definition->name, "WINDOW") == 0)
    return g_strdup (object_types ? "GXWindow *" : "guint32");
  else if (strcmp (definition->name, "DRAWABLE") == 0)
    return g_strdup (object_types ? "GXDrawable *" : "guint32");
  else if (strcmp (definition->name, "PIXMAP") == 0)
    return g_strdup (object_types ? "GXPixmap *" : "guint32");
  else if (strcmp (definition->name, "GCONTEXT") == 0)
    return g_strdup (object_types ? "GXGContext *" : "guint32");

    {
      char *debug = gxgen_namespace_to_gx_type (namespace);
      return debug;
    }
  return gxgen_namespace_to_gx_type (namespace);
}

static void
outv (GXGenOutputContext *output_context,
      const char *part_name,
      const char *format,
      va_list ap)
{
  GList *tmp;
  GString *string = NULL;

  for (tmp = output_context->parts; tmp != NULL; tmp = tmp->next)
    {
      GXGenPart *part = tmp->data;
      if (strcmp (part_name, part->name) == 0)
	{
	  string = part->string;
	  break;
	}
    }

  if (!string)
    {
      GXGenPart *part = g_new0 (GXGenPart, 1);
      part->name = g_strdup (part_name);
      part->string = g_string_new ("");
      output_context->parts = g_list_prepend (output_context->parts, part);
      string = part->string;
    }

  g_string_append_vprintf (string, format, ap);
}

static void
out (GXGenOutputContext *output_context,
      const char *part_name,
      const char *format, ...)
{
  va_list ap;

  va_start (ap, format);
  outv (output_context, part_name, format, ap);
  va_end (ap);
}

/**
 * out2:
 *
 * This is a convenience function that can output to two gstring parts. This
 * can be handy for function prototypes that need to be emitted to a header
 * file and a C file.
 */
static void
out2 (GXGenOutputContext *output_context,
      const char *part0,
      const char *part1,
      const char *format, ...)
{
  va_list ap;

  va_start (ap, format);
  outv (output_context, part0, format, ap);
  outv (output_context, part1, format, ap);
  va_end (ap);
}

static void
output_extension_init (GXGenOutputContext *output_context)
{
  const XGenExtension *extension = output_context->extension;

  /* This one is dealt with in gx_init ()... */
  if (strcmp (extension->header, "xproto") == 0)
    return;

  /* For outputting via _C(), _TD(), _H() and _CH()... */
  output_context->c_part =
    g_strdup_printf ("%s-main-c", extension->header);
#if 0
  output_context->typedefs_part =
    g_strdup_printf ("%s-main-typedefs", extension->header);
  output_context->protos_part =
    g_strdup_printf ("%s-main-header", extension->header);
#endif

  _C ("void\n");
  _C ("gx_extension_%s_init (int *argc, char **argv[])\n", extension->header);
  _C ("{\n");

  /* XXX: Is it a bad idea to implicitly call gx_init, or should it
   * _always_ be the users responsability to have called it? */
  _C ("\tgx_init (argc, argv);\n");
  _C ("\tgx_event_details_add_extension (_gx_%s_event_details);\n",
      extension->header);

  _C ("}\n");

  g_free (output_context->c_part);
  output_context->c_part = NULL;
#if 0
  g_free (output_context->typedefs_part);
  output_context->typedefs_part = NULL;
  g_free (output_context->protos_part);
  output_context->protos_part = NULL;
#endif
}

static void
output_object_headers (GXGenOutputContext *output_context)
{
  const XGenExtension *extension = output_context->extension;
  GXGenObjectType object_type[] = {
    GXGEN_OBJECT_TYPE_CONNECTION,
    GXGEN_OBJECT_TYPE_DRAWABLE,
    GXGEN_OBJECT_TYPE_PIXMAP,
    GXGEN_OBJECT_TYPE_WINDOW,
    GXGEN_OBJECT_TYPE_GCONTEXT,
    0
  };
  int i;

  for (i = 0; object_type[i]; i++)
    {
      const GXGenObject *obj = gxgen_get_object (object_type[i]);
      char *h_header_part, *h_footer_part, *c_header_part;
      char *ext_name_uc = g_ascii_strup (extension->header, -1);
      char *obj_name_uc = g_ascii_strup (obj->name_lc, -1);
      GList *tmp;

      h_header_part = g_strdup_printf ("E@%s:O@%s:N@h-header:",
				       extension->name, obj->name_lc);
      h_footer_part = g_strdup_printf ("E@%s:O@%s:N@h-footer:",
				       extension->name, obj->name_lc);

      out (output_context, h_header_part,
	   "#ifndef _GX_%s_%s_H_\n", obj_name_uc, ext_name_uc);
      out (output_context, h_header_part,
	   "#define _GX_%s_%s_H_\n", obj_name_uc, ext_name_uc);
      out (output_context, h_header_part,
	   "#include <glib.h>\n"
	   "#include <gx/gx-mask-value-item.h>\n");

      out (output_context, h_header_part,
	   "#include <gx/generated-code/extensions/gx-%s.h>\n",
	   extension->header);

      for (tmp = extension->imports; tmp != NULL; tmp = tmp->next)
	{
	  XGenExtension *import = tmp->data;

	  out (output_context, h_header_part,
	       "#include <gx/generated-code/extensions/gx-%s.h>\n",
	       import->header);
	}
      out (output_context, h_footer_part,
	   "#endif /* _GX_%s_%s_H_ */\n", obj_name_uc, ext_name_uc);

      g_free (obj_name_uc);
      g_free (ext_name_uc);
      g_free (h_header_part);
      g_free (h_footer_part);

      c_header_part = g_strdup_printf ("E@%s:O@%s:N@c-header:",
				       extension->name, obj->name_lc);
      out (output_context, c_header_part,
	   "#include <gx/generated-code/extensions/gx-%s.h>\n",
	   extension->header);

      out (output_context, c_header_part,
	   "#include <gx/gx-connection.h>\n");
      out (output_context, c_header_part,
	   "#include <gx/gx-drawable.h>\n");
      out (output_context, c_header_part,
	   "#include <gx/gx-pixmap.h>\n");
      out (output_context, c_header_part,
	   "#include <gx/gx-window.h>\n");
      out (output_context, c_header_part,
	   "#include <gx/gx-gcontext.h>\n");
      out (output_context, c_header_part,
	   "#include <gx/gx-cookie.h>\n");
      out (output_context, c_header_part,
	   "#include <gx/gx-protocol-error.h>\n");
      out (output_context, c_header_part,
	   "#include <stdlib.h>\n");

#if 0
      for (j = 0; object_type[j]; j++)
	{
	  if (j == i)
	    continue;
	  out (output_context, h_header_part,
	       "#include <gx/generated-code/gx-%s-%s.h>\n", extension->header);
	}
#endif

      g_free (c_header_part);
    }
}

/* NB: This function assumes you are outputting to the typedef section of the
 * current header */
static void
output_pad_field (GXGenOutputContext *output_context,
		  XGenFieldDefinition *field,
		  int index)
{
  char *type_name_cc;
  char *pad_name;

  if (field->length->value == 1)
    pad_name = g_strdup_printf ("pad%u", index);
  else
    pad_name = g_strdup_printf ("pad%u[%lu]", index, field->length->value);

  type_name_cc = gxgen_definition_to_gx_type (field->definition, FALSE);
  _TD ("\t%s %s;\n", type_name_cc, pad_name);
  g_free (type_name_cc);
  g_free (pad_name);
}

static void
output_field_xcb_reference (GXGenOutputContext *output_context,
			    XGenFieldDefinition * field)
{
  if (strcmp (field->definition->name, "DRAWABLE") == 0)
    _C ("gx_drawable_get_xid (%s)", field->name);
  else if (strcmp (field->definition->name, "PIXMAP") == 0
	   || strcmp (field->definition->name, "WINDOW") == 0)
    {
      _C ("gx_drawable_get_xid (\n"
	  "\t\t\t\tGX_DRAWABLE(%s))",
	  field->name);
    }
  else if (strcmp (field->definition->name, "GCONTEXT") == 0)
    {
      _C ("gx_gcontext_get_xid (%s)",
	  field->name);
    }
  else if (field->definition->type == XGEN_STRUCT
	   || field->definition->type == XGEN_UNION)
    {
      GXGenDefinition *gxgen_def =
	xgen_definition_get_private (field->definition);
      char *xcb_type = gxgen_namespace_to_xcb_type (gxgen_def->namespace);

      if (field->length)
	_C ("(%s_t *)%s", xcb_type, field->name);
      else
	{
	  /* NB: XCB passes structures by value,
	   * while GX passes them by reference */
	  _C ("*(%s_t *)%s", xcb_type, field->name);
	}

      g_free (xcb_type);
    }
  else
    _C ("%s", field->name);
}

static gboolean
is_special_xid_definition (XGenDefinition *definition)
{
  if (strcmp (definition->name, "DRAWABLE") == 0
      || strcmp (definition->name, "PIXMAP") == 0
      || strcmp (definition->name, "WINDOW") == 0
      || strcmp (definition->name, "GCONTEXT") == 0)
    return TRUE;
  else
    return FALSE;
}

static void
output_typedefs (GXGenOutputContext *output_context)
{
  const XGenExtension *extension = output_context->extension;
  GList *tmp;
  char *typedef_type_cc;
  char *typedef_name_cc;

  /* For outputting via _TD()... */
  output_context->typedefs_part =
    g_strdup_printf ("E@%s:%s", extension->name, "O@connection:N@typedefs:");

  for (tmp = extension->all_definitions; tmp != NULL; tmp = tmp->next)
    {
      XGenDefinition *def = tmp->data;

      if (def->type == XGEN_BOOLEAN
	  || def->type == XGEN_CHAR
	  || def->type == XGEN_SIGNED || def->type == XGEN_UNSIGNED)
	{
	  GXGenDefinition *gxgen_def = xgen_definition_get_private (def);
	  GXGenNamespace *namespace = gxgen_def->namespace;

	  if (strcmp (def->name, "char") == 0)
	    continue;

	  typedef_type_cc = gxgen_definition_to_gx_type (def, FALSE);
	  typedef_name_cc = gxgen_namespace_to_gx_type (namespace);

	  _TD ("typedef %s %s;\n", typedef_type_cc, typedef_name_cc);

	  g_free (typedef_type_cc);
	  g_free (typedef_name_cc);
	}
      else if (def->type == XGEN_XID || def->type == XGEN_XIDUNION)
	{
	  if (is_special_xid_definition (def))
	    continue;

	  typedef_name_cc = gxgen_definition_to_gx_type (def, FALSE);

	  _TD ("typedef guint32 %s;\n", typedef_name_cc);

	  g_free (typedef_name_cc);
	}
    }
  _TD ("\n");

  for (tmp = extension->typedefs; tmp != NULL; tmp = tmp->next)
    {
      XGenDefinition *def = tmp->data;
      GXGenDefinition *gxgen_def = xgen_definition_get_private (def);
      XGenTypedef *typedef_def = XGEN_TYPEDEF_DEF (def);
      GXGenDefinition *gxgen_ref_def =
	xgen_definition_get_private (typedef_def->reference);
      char *typedef_type_cc;
      char *typedef_name_cc;

      typedef_type_cc = gxgen_namespace_to_gx_type (gxgen_ref_def->namespace);
      typedef_name_cc = gxgen_namespace_to_gx_type (gxgen_def->namespace);

      _TD ("typedef %s %s;\n", typedef_type_cc, typedef_name_cc);

      g_free (typedef_type_cc);
      g_free (typedef_name_cc);
    }
  _TD ("\n\n\n");

  g_free (output_context->typedefs_part);
  output_context->typedefs_part = NULL;
}

static void
output_structs_and_unions (GXGenOutputContext *output_context)
{
  const XGenExtension *extension = output_context->extension;
  GList *tmp;

  /* For outputting via _TD()... */
  output_context->typedefs_part =
    g_strdup_printf ("E@%s:%s", extension->name, "O@connection:N@typedefs:");

  for (tmp = extension->all_definitions; tmp != NULL; tmp = tmp->next)
    {
      XGenDefinition *def = tmp->data;
      GXGenDefinition *gxgen_def = xgen_definition_get_private (def);
      guint pad = 0;
      GList *tmp2;
      char *gx_type;
      char *xcb_type;

      if (def->type != XGEN_STRUCT && def->type != XGEN_UNION)
	continue;

      /* Some types are special cased if they are represented as
       * objects */
      if (strcmp (def->name, "SCREEN") == 0)
	continue;

      gx_type = gxgen_namespace_to_gx_type (gxgen_def->namespace);
      xcb_type = gxgen_namespace_to_xcb_type (gxgen_def->namespace);

      _TD ("typedef %s _%s {\n",
	   def->type == XGEN_STRUCT ? "struct" : "union",
	   gx_type);

      _TEST_C ("\tg_assert (sizeof (%s) == sizeof (%s_t));\n",
	       gx_type, xcb_type);

      for (tmp2 = XGEN_STRUCT_DEF (def)->fields;
	   tmp2 != NULL;
	   tmp2 = tmp2->next)
	{
	  XGenFieldDefinition *field = tmp2->data;
	  if (strcmp (field->name, "pad") == 0)
	    {
	      output_pad_field (output_context,
				field,
				pad++);
	    }
	  else
	    {
	      /* Dont print trailing list fields */
	      if (!(tmp2->next == NULL && field->length != NULL))
		{
		  _TD ("\t%s %s;\n",
		       gxgen_definition_to_gx_type (field->definition,
						    FALSE),
		       field->name);
		}
	    }
	}

      _TD ("} %s;\n\n", gx_type);

      g_free (gx_type);
    }
  _TD ("\n");

  g_free (output_context->typedefs_part);
  output_context->typedefs_part = NULL;
}

static void
output_enums (GXGenOutputContext *output_context)
{
  const XGenExtension *extension = output_context->extension;
  GList *tmp;

  /* For outputting via _TD()... */
  output_context->typedefs_part =
    g_strdup_printf ("E@%s:%s", extension->name, "O@connection:N@typedefs:");

  for (tmp = extension->enums; tmp != NULL; tmp = tmp->next)
    {
      XGenDefinition *def = tmp->data;
      GXGenDefinition *gxgen_def = xgen_definition_get_private (def);
      GList *tmp2;
      char *gx_type = gxgen_namespace_to_gx_type (gxgen_def->namespace);

      /* We need the "Type" suffix otherwise we end up with a GXPixmap
       * typedef which will cause a conflict */
      _TD ("typedef enum _%sType\n{\n", gx_type);

      for (tmp2 = XGEN_ENUM_DEF (def)->items; tmp2 != NULL; tmp2 = tmp2->next)
	{
	  XGenItemDefinition *item = tmp2->data;
	  GXGenNamespace *item_namespace =
	    gxgen_namespace_copy (gxgen_def->namespace);
	  gxgen_namespace_append (item_namespace, item->name);
	  char *item_gx_define = gxgen_namespace_to_gx_define (item_namespace);

	  if (item->type == XGEN_ITEM_AS_VALUE)
	    _TD (" %s = %s,\n", item_gx_define, item->value);
	  else
	    _TD (" %s = (1 << %u),\n", item_gx_define, item->bit);

	  g_free (item_gx_define);
	  gxgen_namespace_free (item_namespace);
	}
      _TD ("} %sType;\n\n", gx_type);

      g_free (gx_type);
    }

  g_free (output_context->typedefs_part);
  output_context->typedefs_part = NULL;
}

static void
output_reply_typedef (GXGenOutputContext *output_context)
{
  const XGenRequest *request = output_context->out_request;
  GXGenDefinition *gxgen_def =
    xgen_definition_get_private (XGEN_DEF (request));
  char *gx_type = gxgen_namespace_to_gx_type (gxgen_def->namespace);
  GList *tmp;
  guint pad = 0;

  if (!request->reply)
    return;

  _TD ("typedef struct {\n");

  for (tmp = request->reply->fields;
       tmp != NULL; tmp = tmp->next)
    {
      XGenFieldDefinition *field = tmp->data;
      if (strcmp (field->name, "pad") == 0)
	output_pad_field (output_context, field, pad++);
      else
	{
	  /* Dont print trailing list members */
	  if (!(tmp->next == NULL && field->length != NULL))
	    {
	      _TD ("\t%s %s;\n",
		   gxgen_definition_to_gx_type (field->definition, FALSE),
		   field->name);
	    }
	}
    }
  _TD ("\n} %sX11Reply;\n\n", gx_type);

  _TD ("typedef struct {\n");
  _TD ("\tGXConnection *connection;\n");
  _TD ("\t%sX11Reply *x11_reply;\n", gx_type);
  _TD ("\n} %sReply;\n\n", gx_type);
}

static void
output_reply_list_get (GXGenOutputContext *output_context)
{
  const XGenRequest *request = output_context->out_request;
  GXGenDefinition *gxgen_def =
    xgen_definition_get_private (XGEN_DEF (request));
  char *gx_name = gxgen_namespace_to_gx_name (gxgen_def->namespace);
  char *gx_type = gxgen_namespace_to_gx_type (gxgen_def->namespace);
  XGenFieldDefinition *field;

  if (!request->reply)
    return;

  field = (g_list_last (request->reply->fields))->data;
  if (field->length == NULL)
    return;

  /* FIXME - shouldn't be restricted to FIELDREF length types */
  if (field->length->type != XGEN_FIELDREF)
    return;

  if (is_special_xid_definition (field->definition))
    _CH ("GList *\n");
  else
    _CH ("GArray *\n");

  _CH ("%s_get_%s (%sReply *%s_reply)",
       gx_name,
       field->name,
       gx_type,
       gx_name);

  _H (";\n");
  _C ("\n{\n");

  _C ("  %s *p = (%s *)(%s_reply->x11_reply + 1);\n",
      gxgen_definition_to_gx_type (field->definition, FALSE),
      gxgen_definition_to_gx_type (field->definition, FALSE),
      gx_name);

  if (is_special_xid_definition (field->definition))
    _C ("  GList *tmp = NULL;\n");
  else
    _C ("  GArray *tmp;\n");

  _C ("\n");

  _C ("  /* It's possible the connection has been closed since the reply\n"
      "   * was recieved: (the reply struct contains weak pointer) */\n"
      "  if (!%s_reply->connection)\n"
      "    return NULL;\n",
      gx_name);

  if (is_special_xid_definition (field->definition))
    _C ("  int i;\n");
  else
    _C ("  tmp = g_array_new (TRUE, FALSE, sizeof(%s));\n",
        gxgen_definition_to_gx_type (field->definition, FALSE));

  if (is_special_xid_definition (field->definition))
    {
      _C ("  for (i = 0; i< %s_reply->x11_reply->%s; i++)\n"
	  "    {\n",
	  gx_name,
	  field->length->field);

      _C ("      /* FIXME - mutex */\n");

      _C ("      %s item = gx_%s_find_from_xid (p[i]);\n",
	  gxgen_definition_to_gx_type (field->definition, TRUE),
	  gxgen_def->object->name_lc);

      _C (
       "      if (!item)\n"
       "	item = g_object_new (gx_%s_get_type(),\n"
       "			     \"connection\", %s_reply->connection,\n"
       "			     \"xid\", p[i],\n"
       "			     \"wrap\", TRUE,\n"
       "			     NULL);\n",
       gxgen_def->object->name_lc, gx_name);
      _C ("      tmp = g_list_prepend (tmp, item);\n");
      _C ("    }\n");
    }
  else
    {
      _C ("  tmp = g_array_append_vals (tmp, p, %s_reply->x11_reply->%s);\n",
	  gx_name, field->length->field);
    }

  if (is_special_xid_definition (field->definition))
    _C ("  tmp = g_list_reverse (tmp);");

  _C ("  return tmp;\n");

  _C ("}\n");

}

static void
output_reply_list_free (GXGenOutputContext *output_context)
{
  const XGenRequest *request = output_context->out_request;
  GXGenDefinition *gxgen_def =
    xgen_definition_get_private (XGEN_DEF (request));
  char *gx_name = gxgen_namespace_to_gx_name (gxgen_def->namespace);
  XGenFieldDefinition *field;

  if (!request->reply)
    return;

  field = (g_list_last (request->reply->fields))->data;
  if (field->length == NULL)
    return;

  /* FIXME - shouldn't be restricted to FIELDREF length types */
  if (field->length->type != XGEN_FIELDREF)
    return;

  _CH ("\nvoid\n");
  _CH ("%s_free_%s (", gx_name, field->name);
  if (is_special_xid_definition (field->definition))
    _CH ("GList *%s)", field->name);
  else
    _CH ("GArray *%s)", field->name);

  _H (";\n");
  _C ("\n{\n");

  if (is_special_xid_definition (field->definition))
    {
      _C ("\n"
	  "\tg_list_foreach (%s, (GFunc)g_object_unref, NULL);\n",
	  field->name);
    }
  else
    _C ("\tg_array_free (%s, TRUE);\n", field->name);

  _C ("}\n");
}

static void
output_reply_free (GXGenOutputContext *output_context)
{
  const XGenRequest *request = output_context->out_request;
  GXGenDefinition *gxgen_def =
    xgen_definition_get_private (XGEN_DEF (request));
  char *gx_name = gxgen_namespace_to_gx_name (gxgen_def->namespace);
  char *gx_type = gxgen_namespace_to_gx_type (gxgen_def->namespace);

  if (!request->reply)
    return;

  _CH ("void\n"
       "%s_reply_free (%sReply *%s_reply)",
       gx_name, gx_type, gx_name);

  _H (";\n");
  _C ("\n{\n");

  _C ("  free (%s_reply->x11_reply);\n", gx_name);
  _C ("  g_slice_free (%sReply, %s_reply);\n", gx_type, gx_name);

  _C ("}\n");
}

static void
output_mask_value_variable_declarations (GXGenOutputContext *output_context)
{
  _C ("\tguint32 value_list_len = "
	  "gx_mask_value_items_get_count (mask_value_items);\n");
  _C ("\tguint32 *value_list = "
	  "alloca (value_list_len * 4);\n");
  _C ("\tguint32 value_mask;\n");
  _C ("\n");

  _C ("\tgx_mask_value_items_get_list (mask_value_items, "
	  "&value_mask, value_list);\n");
}

/**
 * output_reply_variable_definitions:
 * @part: The particular stream you to output too
 * @request: The request to which you will be replying
 *
 * This function outputs the variable declarations needed
 * for preparing a reply. This should be called in the
 * *_reply () funcs that take a cookie or the synchronous
 * request functions.
 */
static void
output_reply_variable_declarations (GXGenOutputContext *output_context)
{
  const XGenRequest *request = output_context->out_request;
  GXGenDefinition *gxgen_def =
    xgen_definition_get_private (XGEN_DEF (request));
  char *gx_type = gxgen_namespace_to_gx_type (gxgen_def->namespace);

  _C ("\txcb_generic_error_t *xcb_error;\n");

  if (request->reply)
    _C ("\t%sReply *reply = g_slice_new (%sReply);\n", gx_type, gx_type);
}

/**
 * output_async_request:
 *
 * This function outputs the code for all gx_*_async () functions
 */
void
output_async_request (GXGenOutputContext *output_context)
{
  const XGenRequest *request = output_context->out_request;
  const XGenDefinition *def = XGEN_DEF (request);
  GXGenDefinition *gxgen_def = xgen_definition_get_private (def);
  char *gx_name = gxgen_namespace_to_gx_name (gxgen_def->namespace);
  //char *gx_type = gxgen_namespace_to_gx_type (gxgen_def->namespace);
  //char *gx_define = gxgen_namespace_to_gx_define (gxgen_def->namespace);
  char *xcb_type = gxgen_namespace_to_xcb_type (gxgen_def->namespace);
  char *xcb_name = gxgen_namespace_to_xcb_name (gxgen_def->namespace);
  const GXGenObject *obj = gxgen_def->object;
  GList *tmp;
  GXGenNamespace *cookie_namespace;
  char *cookie_gx_define;
  gboolean has_mask_value_items = FALSE;

  _CH ("\nGXCookie *\n%s_async (%s", gx_name, obj->first_arg);

  for (tmp = request->fields; tmp != NULL; tmp = tmp->next)
    {
      XGenFieldDefinition *field = tmp->data;
      char *field_gx_type;

      if (strcmp (field->name, "opcode") == 0
	  || strcmp (field->name, "pad") == 0
	  || strcmp (field->name, "length") == 0)
	continue;

      if (gxgen_def->first_object_field
	  && field == gxgen_def->first_object_field)
	continue;
#if 0
      if ((obj->type == GXGEN_OBJECT_TYPE_WINDOW
	   && strcmp (field->name, "window") == 0)
	  || (obj->type == GXGEN_OBJECT_TYPE_DRAWABLE
	      && strcmp (field->name, "drawable") == 0))
	continue;
#endif

      field_gx_type = gxgen_definition_to_gx_type (field->definition, TRUE);

      if (field->length)
	_CH (",\n\t\tconst %s *%s", field_gx_type, field->name);
      else if (field->definition->type == XGEN_VALUEPARAM)
	{
	  _CH (",\n\t\tGXMaskValueItem *mask_value_items");
	  has_mask_value_items = TRUE;
	}
      else if (field->definition->type == XGEN_STRUCT
	       || field->definition->type == XGEN_UNION)
	{
	  _CH (",\n\t\t%s *%s", field_gx_type, field->name);
	}
      else
	_CH (",\n\t\t%s %s", field_gx_type, field->name);
    }
  _H (");\n\n");
  _C (")\n{\n");

  /*
   * *_async() code
   */
  if (obj->type != GXGEN_OBJECT_TYPE_CONNECTION)
    {
      g_assert (gxgen_def->first_object_field);
      _C ("\tGXConnection *connection = gx_%s_get_connection (%s);\n",
	   obj->name_lc, gxgen_def->first_object_field->name);
    }

  if (!request->reply)
    _C ("\txcb_void_cookie_t xcb_cookie;\n");
  else
    _C ("\t%s_cookie_t xcb_cookie;\n", xcb_type);

  _C ("\tGXCookie *cookie;\n\n");

  if (has_mask_value_items)
    output_mask_value_variable_declarations (output_context);

  _C ("\n");

  if (request->reply)
    {
      _C ("\txcb_cookie =\n"
	  "\t\t%s (\n"
	  "\t\t\tgx_connection_get_xcb_connection (connection)",
	  xcb_name);
    }
  else
    {
      _C ("\txcb_cookie =\n"
	  "\t\t%s_checked (\n"
	  "\t\t\tgx_connection_get_xcb_connection (connection)",
	  xcb_name);
    }

  for (tmp = request->fields; tmp != NULL;
       tmp = tmp->next)
    {
      XGenFieldDefinition *field = tmp->data;

      if (strcmp (field->name, "opcode") == 0
	  || strcmp (field->name, "pad") == 0
	  || strcmp (field->name, "length") == 0)
	continue;

      if (field->definition->type != XGEN_VALUEPARAM)
	{
	  /* Some special cased field types require a function call
	   * to lookup their counterpart xcb value */
	  _C (",\n\t\t\t");
	  output_field_xcb_reference (output_context, field);
	}
      else
	{
	  _C (",\n\t\t\tvalue_mask");
	  _C (",\n\t\t\tvalue_list");
	}
    }
  _C (");\n\n");

  cookie_namespace =
    gxgen_namespace_new (NULL, def, "%sCookie", def->name);
  cookie_gx_define = gxgen_namespace_to_gx_define (cookie_namespace);

  out (output_context,
       "cookie-typedefs",
       "\t%s,\n", cookie_gx_define);

  _C ("\tcookie = gx_cookie_new (connection, %s, xcb_cookie.sequence);\n",
      cookie_gx_define);

  g_free (cookie_gx_define);

  if (obj->type != GXGEN_OBJECT_TYPE_CONNECTION)
    _C ("\tg_object_unref (connection);\n");

  _C ("\tgx_connection_register_cookie (connection, cookie);\n");

  _C ("\treturn cookie;\n");

  _C ("}\n");
}

void
output_reply (GXGenOutputContext *output_context)
{
  const XGenRequest *request = output_context->out_request;
  GXGenDefinition *gxgen_def =
    xgen_definition_get_private (XGEN_DEF (request));
  char *gx_name = gxgen_namespace_to_gx_name (gxgen_def->namespace);
  char *gx_type = gxgen_namespace_to_gx_type (gxgen_def->namespace);
  char *xcb_name = gxgen_namespace_to_xcb_name (gxgen_def->namespace);
  char *xcb_type = gxgen_namespace_to_xcb_type (gxgen_def->namespace);

  if (request->reply)
    _CH ("\n%sReply *\n", gx_type);
  else
    _CH ("\ngboolean\n");

  _CH ("%s_reply (GXCookie *cookie, GError **error)\n", gx_name);

  _H (";\n");
  _C ("\n{\n");

  _C ("\tGXConnection *connection = gx_cookie_get_connection (cookie);\n");

  if (!request->reply)
    _C ("\txcb_void_cookie_t xcb_cookie;\n");
  else
    _C ("\t%s_cookie_t xcb_cookie;\n", xcb_type);

  output_reply_variable_declarations (output_context);
  _C ("\n");

  _C ("\tg_return_val_if_fail (error == NULL || *error == NULL, %s);\n",
      request->reply == NULL ? "FALSE" : "NULL");

  _C ("\n");

  if (request->reply)
    _C ("\treply->connection = connection;\n\n");

  if (request->reply)
    {
      _C ("\treply->x11_reply = (%sX11Reply *)\n"
	  "\t\tgx_cookie_get_reply (cookie);\n",
	  gx_type);

      _C ("\tif (!reply->x11_reply)\n"
	  "\t  {\n");
    }

  /* If the cookie doesn't have an associated reply, then we see
   * first see if it has an associated error instead.
   */
  /* FIXME - we need a mechanism for translating X errors into a glib
   * error domain, code and message. */
  _C ("\txcb_error = gx_cookie_get_error (cookie);\n");
  /* FIXME create a func for outputing this... */
  _C ("\tif (xcb_error)\n"
      "\t  {\n"
      "\t\tg_set_error (error,\n"
      "\t\t\tGX_PROTOCOL_ERROR,\n"
      "\t\t\tgx_protocol_error_from_xcb_error (xcb_error),\n"
      "\t\t\t\"Protocol Error\");\n"
      "\t\treturn %s;\n"
      "\t  }\n",
      request->reply != NULL ? "NULL" : "FALSE");
  /* FIXME - free reply */
  /* FIXME - check we don't skip any other function cleanup */

  _C ("\txcb_cookie.sequence = gx_cookie_get_sequence (cookie);\n");

  /* If the cookie has no associated reply or error, then we ask
   * XCB for a reply/error
   */
  if (request->reply)
    {
      _C ("\treply->x11_reply = (%sX11Reply *)\n"
	  "\t\t%s_reply (\n"
	  "\t\t\tgx_connection_get_xcb_connection (connection),\n"
	  "\t\t\txcb_cookie,\n"
	  "\t\t\t&xcb_error);\n",
	  gx_type,
	  xcb_name);
    }
  else
    {
      _C ("\txcb_error = \n"
	  "\t\txcb_request_check (\n"
	  "\t\t\tgx_connection_get_xcb_connection (connection),\n"
	  "\t\t\txcb_cookie);\n");
    }

  _C ("\tif (xcb_error)\n"
      "\t  {\n"
      "\t\tg_set_error (error,\n"
      "\t\t\tGX_PROTOCOL_ERROR,\n"
      "\t\t\tgx_protocol_error_from_xcb_error (xcb_error),\n"
      "\t\t\t\"Protocol Error\");\n"
      "\t\treturn %s;\n"
      "\t  }\n",
      request->reply != NULL ? "NULL" : "FALSE");


  if (request->reply)
    _C ("\n\t  }\n");

  _C ("\tgx_connection_unregister_cookie (connection, cookie);\n");

  if (!request->reply)
    _C ("\treturn TRUE;\n");
  else
    _C ("\treturn reply;\n");

  _C ("}\n");
}

void
output_sync_request (GXGenOutputContext *output_context)
{
  const XGenRequest *request = output_context->out_request;
  GXGenDefinition *gxgen_def =
    xgen_definition_get_private (XGEN_DEF (request));
  char *gx_name = gxgen_namespace_to_gx_name (gxgen_def->namespace);
  char *gx_type = gxgen_namespace_to_gx_type (gxgen_def->namespace);
  char *xcb_name = gxgen_namespace_to_xcb_name (gxgen_def->namespace);
  char *xcb_type = gxgen_namespace_to_xcb_type (gxgen_def->namespace);
  const GXGenObject *obj = gxgen_def->object;
  gboolean has_mask_value_items = FALSE;
  GList *tmp;

  if (!request->reply)
    _CH ("\ngboolean\n");
  else
    _CH ("\n%sReply *\n", gx_type);

  _CH ("%s (%s", gx_name, obj->first_arg);

  for (tmp = request->fields;
       tmp != NULL; tmp = tmp->next)
    {
      XGenFieldDefinition *field = tmp->data;
      char *field_gx_type;

      if (strcmp (field->name, "opcode") == 0
	  || strcmp (field->name, "pad") == 0
	  || strcmp (field->name, "length") == 0)
	continue;

      if (gxgen_def->first_object_field
	  && field == gxgen_def->first_object_field)
	continue;

      field_gx_type = gxgen_definition_to_gx_type (field->definition, TRUE);

      if (field->length)
	{
	  _CH (",\n\t\tconst %s *%s", field_gx_type, field->name);
	}
      else if (field->definition->type == XGEN_VALUEPARAM)
	{
	  _CH (",\n\t\tGXMaskValueItem *mask_value_items");
	  has_mask_value_items = TRUE;
	}
      else if (field->definition->type == XGEN_STRUCT
	       || field->definition->type == XGEN_UNION)
	{
	  _CH (",\n\t\t%s *%s", field_gx_type, field->name);
	}
      else
	  _CH (",\n\t\t%s %s", field_gx_type, field->name);
    }
  _CH (",\n\t\tGError **error)");

  _H (";\n\n");
  _C ("\n{\n");

  if (obj->type != GXGEN_OBJECT_TYPE_CONNECTION)
    {
      g_assert (gxgen_def->first_object_field->name);
      _C ("\tGXConnection *connection = gx_%s_get_connection (%s);\n",
	  obj->name_lc, gxgen_def->first_object_field->name);
    }

  if (!request->reply)
    _C ("\txcb_void_cookie_t cookie;\n");
  else
    _C ("\t%s_cookie_t cookie;\n",
	xcb_type);
  output_reply_variable_declarations (output_context);

  /* If the request has a XGEN_VALUEPARAM field, then we will need
   * to translate an array of GXMaskValueItems from the user.
   */
  if (has_mask_value_items)
    output_mask_value_variable_declarations (output_context);

  _C ("\n");
  if (!request->reply)
    _C ("\tg_return_val_if_fail (error == NULL || *error == NULL, FALSE);\n");
  else
    _C ("\tg_return_val_if_fail (error == NULL || *error == NULL, NULL);\n");

  if (request->reply)
    _C ("\treply->connection = connection;\n\n");

  if (request->reply)
    {
      _C ("\tcookie =\n"
	  "\t\t%s (\n",
	  xcb_name);
    }
  else
    {
      _C ("\tcookie =\n"
	  "\t\t%s_checked (\n",
	  xcb_name);
    }
  _C ("\t\t\tgx_connection_get_xcb_connection (connection)");

  for (tmp = request->fields; tmp != NULL;
       tmp = tmp->next)
    {
      XGenFieldDefinition *field = tmp->data;

      if (strcmp (field->name, "opcode") == 0
	  || strcmp (field->name, "pad") == 0
	  || strcmp (field->name, "length") == 0)
	continue;

      if (field->definition->type != XGEN_VALUEPARAM)
	{
	  /* Some special cased field types require a function call
	   * to lookup their counterpart xcb value */
	  _C (",\n\t\t\t");
	  output_field_xcb_reference (output_context, field);
	}
      else
	{
	  _C (",\n\t\t\tvalue_mask");
	  _C (",\n\t\t\tvalue_list");
	}
    }
  _C (");\n\n");

  if (request->reply)
    {
      _C ("\treply->x11_reply = (%sX11Reply *)\n"
	  "\t\t%s_reply (\n"
	  "\t\t\tgx_connection_get_xcb_connection (connection),\n"
	  "\t\t\tcookie,\n"
	  "\t\t\t&xcb_error);\n",
	  gx_type,
	  xcb_name);
    }
  else
    {
      _C ("\txcb_error = \n"
	  "\t\txcb_request_check (\n"
	  "\t\t\tgx_connection_get_xcb_connection (connection),\n"
	  "\t\t\tcookie);\n");
    }

  /* FIXME create a func for outputing this... */
  _C ("\tif (xcb_error)\n"
      "\t  {\n"
      "\t\tg_set_error (error,\n"
      "\t\t\tGX_PROTOCOL_ERROR,\n"
      "\t\t\tgx_protocol_error_from_xcb_error (xcb_error),\n"
      "\t\t\t\"Protocol Error\");\n"
      "\t\treturn %s;\n"
      "\t  }\n",
      request->reply != NULL ? "NULL" : "FALSE");

  if (obj->type != GXGEN_OBJECT_TYPE_CONNECTION)
    _C ("\tg_object_unref (connection);\n");

  if (!request->reply)
    _C ("\n\treturn TRUE;\n");
  else
    _C ("\n\treturn reply;\n");

  _C ("}\n\n");
}

static void
output_requests (GXGenOutputContext *output_context)
{
  const XGenExtension *extension = output_context->extension;
  GList *tmp;

  for (tmp = extension->requests; tmp != NULL; tmp = tmp->next)
    {
      XGenRequest *request = tmp->data;
      XGenDefinition *def = XGEN_DEF (request);
      GXGenDefinition *gxgen_def = xgen_definition_get_private (def);
      const GXGenObject *obj = gxgen_def->object;

      /* Some requests are special cased and implemented within object
       * constructors and so we don't emit code for them...
       */
      if (strcmp (def->name, "CreateWindow") == 0
	  || strcmp (def->name, "CreatePixmap") == 0
	  || strcmp (def->name, "CreateGC") == 0)
	continue;

      output_context->out_request = request;

      /* For outputting via _C(), _TD(), _H() and _CH()... */
      output_context->c_part =
	g_strdup_printf ("E@%s:O@%s:N@c-funcs:",
			 extension->name, obj->name_lc);
      output_context->typedefs_part =
	g_strdup_printf ("E@%s:O@%s:N@typedefs:",
			 extension->name, obj->name_lc);
      output_context->protos_part =
	g_strdup_printf ("E@%s:O@%s:N@protos:",
			 extension->name, obj->name_lc);

      /* If the request has a reply definition then we typedef
       * the reply struct.
       */
      output_reply_typedef (output_context);

      /* Some replys include a list of data. If this is such a request
       * then we output a getter function for trailing list fields */
      output_reply_list_get (output_context);
      output_reply_list_free (output_context);

      output_reply_free (output_context);

      output_async_request (output_context);
      output_reply (output_context);

      output_sync_request (output_context);

      g_free (output_context->c_part);
      output_context->c_part = NULL;
      g_free (output_context->typedefs_part);
      output_context->typedefs_part = NULL;
      g_free (output_context->protos_part);
      output_context->protos_part = NULL;
    }
}

static void
output_event_typedefs (GXGenOutputContext *output_context)
{
  const XGenExtension *extension = output_context->extension;
  GList *tmp;
  guint pad = 0;

  /* For outputting via _TD()... */
  output_context->typedefs_part =
    g_strdup_printf ("E@%s:%s", extension->name, "O@connection:N@typedefs:");

  for (tmp = extension->events; tmp != NULL; tmp = tmp->next)
    {
      XGenEvent *event = tmp->data;
      XGenDefinition *def = XGEN_DEF (event);
      GXGenDefinition *gxgen_def = xgen_definition_get_private (def);
      char *gx_type = gxgen_namespace_to_gx_type (gxgen_def->namespace);
      GList *tmp2;

      _TD ("\ntypedef struct {\n");

      for (tmp2 = event->fields; tmp2 != NULL; tmp2 = tmp2->next)
	{
	  XGenFieldDefinition *field = tmp2->data;
	  if (strcmp (field->name, "pad") == 0)
	    output_pad_field (output_context, field, pad++);
	  else
	    {
	      /* Dont print trailing list fields */
	      if (!(tmp2->next == NULL && field->length != NULL))
		{
		  _TD ("\t%s %s;\n",
		       gxgen_definition_to_gx_type (field->definition, FALSE),
		       field->name);
		}
	    }
	}
      _TD ("} %sEvent;\n", gx_type);
    }
}

static void
output_error_extras (GXGenOutputContext *output_context)
{
  const XGenExtension *extension = output_context->extension;
  GList		      *tmp;
  GXGenNamespace      *typedef_namespace;
  char		      *typedef_name;

  if (!extension->errors)
    return;

  /* For outputting via _H() and _C()... */
  output_context->protos_part =
    g_strdup_printf ("%s-error-code-enums",
		     extension->header);
  output_context->c_part =
    g_strdup_printf ("%s-error-details",
		     extension->header);

  typedef_namespace =
    gxgen_namespace_new (NULL,
			 extension->errors->data,
			 "ProtocolErrorCode");
  typedef_name = gxgen_namespace_to_gx_type (typedef_namespace);
  gxgen_namespace_free (typedef_namespace);

  _H ("typedef enum _%s\n", typedef_name);
  _H ("{\n");

  _C ("GXProtocolErrorDetails _gx_%s_error_details[] = {\n",
      extension->header);

  for (tmp = extension->errors; tmp != NULL; tmp = tmp->next)
    {
      XGenDefinition *definition = tmp->data;
      XGenError *error = XGEN_ERROR_DEF (definition);
      GXGenDefinition *gxgen_def = xgen_definition_get_private (definition);
      char *error_code_define;
      GXGenNamespace *namespace =
	gxgen_namespace_new (NULL, definition, "ProtocolError%s",
			     definition->name);
      error_code_define = gxgen_namespace_to_gx_define (namespace);
      gxgen_namespace_free (namespace);

      _H ("\t%s = %d,\n", error_code_define, error->number);
      _C ("{%d, 0, \"%s\"},\n", error->number, error_code_define);
    }

  _C ("\t{0}");
  _C ("};\n");

  _H ("} %s\n", typedef_name);
  g_free (typedef_name);

  g_free (output_context->protos_part);
  output_context->protos_part = NULL;
  g_free (output_context->c_part);
  output_context->c_part = NULL;
}

static void
output_event_extras (GXGenOutputContext *output_context)
{
  const XGenExtension *extension = output_context->extension;
  GList		      *tmp;
  GXGenNamespace      *typedef_namespace;
  char		      *typedef_name;

  if (!extension->events)
    return;

  /* For outputting via _H() and _C()... */
  output_context->protos_part =
    g_strdup_printf ("%s-event-code-enums",
		     extension->header);
  output_context->c_part =
    g_strdup_printf ("%s-event-details",
		     extension->header);

  typedef_namespace =
    gxgen_namespace_new (NULL,
			 extension->events->data,
			 "EventCode");
  typedef_name = gxgen_namespace_to_gx_type (typedef_namespace);
  gxgen_namespace_free (typedef_namespace);

  _H ("typedef enum _%s\n", typedef_name);
  _H ("{\n");

  _C ("#include <gx/gx-event.h>\n");
  _C ("#include <gx/generated-code/extensions/gx-%s.h>\n",
      extension->header);
  _C ("\n");
  _C ("GXEventDetails _gx_%s_event_details[] = {\n", extension->header);

  for (tmp = extension->events; tmp != NULL; tmp = tmp->next)
    {
      XGenDefinition *definition = tmp->data;
      XGenEvent *event = XGEN_EVENT_DEF (definition);
      GXGenDefinition *gxgen_def = xgen_definition_get_private (definition);
      char *gx_type =
	gxgen_namespace_to_gx_type (gxgen_def->namespace);
      GList *tmp2;
      gboolean found_window_offset = FALSE;
      GXGenNamespace *namespace;
      char *event_code_define;

      namespace =
	gxgen_namespace_new (NULL, definition, "Event%s", definition->name);
      event_code_define = gxgen_namespace_to_gx_define (namespace);
      gxgen_namespace_free (namespace);

      _H ("\t%s = %d,\n", event_code_define, event->number);
      _C ("\t{%d, \"%s\", ", event->number, event_code_define);

      for (tmp2 = event->fields; tmp2 != NULL; tmp2 = tmp2->next)
	{
	  XGenFieldDefinition *field = tmp2->data;
	  if (strcmp (field->name, "window") == 0)
	    {
	      found_window_offset = TRUE;
	      _C ("offsetof (%sEvent, window)},\n", gx_type);
	      break;
	    }
	}

      g_free (event_code_define);

      if (!found_window_offset)
	_C ("0},\n");
    }

  _C ("\t{0}");
  _C ("};\n");

  _H ("} GX%sEventCode\n", typedef_name);
  g_free (typedef_name);

  g_free (output_context->protos_part);
  output_context->protos_part = NULL;
  g_free (output_context->c_part);
  output_context->c_part = NULL;
}

static void
output_extension_code (GXGenOutputContext *output_context)
{
  const XGenExtension *extension = output_context->extension;
  char *extension_header_part_name =
    g_strdup_printf ("%s-includes", extension->header);
  char *ext_name_uc = g_ascii_strup (extension->header, -1);
  GXGenObjectType object_type[] = {
    GXGEN_OBJECT_TYPE_CONNECTION,
    GXGEN_OBJECT_TYPE_DRAWABLE,
    GXGEN_OBJECT_TYPE_PIXMAP,
    GXGEN_OBJECT_TYPE_WINDOW,
    GXGEN_OBJECT_TYPE_GCONTEXT,
    0
  };
  int i;

  out (output_context, extension_header_part_name,
       "#ifndef __GX_EXT_%s_H_\n", ext_name_uc);
  out (output_context, extension_header_part_name,
       "#define __GX_EXT_%s_H_\n", ext_name_uc);

  if (strcmp (extension->header, "xproto") == 0)
    {
      out (output_context, extension_header_part_name,
	   "#include <gx/gx-types.h>\n");
    }

  for (i = 0; object_type[i]; i++)
    {
      const GXGenObject *obj = gxgen_get_object (object_type[i]);

      out (output_context, extension_header_part_name,
	   "#include <gx/generated-code/gx-%s-%s-gen.h>\n",
	   obj->name_lc, extension->header);
    }

  out (output_context, extension_header_part_name,
       "#endif /* __GX_EXT_%s_H_ */\n", ext_name_uc);
  g_free (ext_name_uc);
  g_free (extension_header_part_name);

  out (output_context, "xcb-dependencies",
       "#include <xcb/%s.h>\n", extension->header);

  output_extension_init (output_context);
  output_object_headers (output_context);
  output_enums (output_context);
  output_typedefs (output_context);
  output_structs_and_unions (output_context);
  output_requests (output_context);
  output_event_typedefs (output_context);
  output_error_extras (output_context);
  output_event_extras (output_context);
}

static GXGenOutputContext *
output_everything_to_part_buffers (XGenState *state)
{
  GXGenOutputContext *output_context = g_new0 (GXGenOutputContext, 1);
  GList *tmp;

  output_context->state = state;

  output_context->parts = NULL;

  out (output_context,
       "xcb-dependencies",
       "#include <xcb/xcb.h>\n");

  out (output_context,
       "cookie-typedefs",
       "typedef enum _GXCookieType\n{\n");

  _TEST_C ("#include <glib.h>\n");
  _TEST_C ("#include <gx.h>\n");
  _TEST_C ("\n");
  _TEST_C ("int\n");
  _TEST_C ("main (int argc, char **argv)\n");
  _TEST_C ("{\n");

  for (tmp = state->extensions; tmp != NULL; tmp = tmp->next)
    {
      output_context->extension = tmp->data;
      output_extension_code (output_context);
    }

  _TEST_C ("\treturn 0;\n");
  _TEST_C ("}\n");

  out (output_context,
       "cookie-typedefs",
       "} GXCookieType;\n");

  return output_context;
}

static void
write_parts_to_file (GXGenOutputContext *output_context,
		     const char *filename,
		     GList *parts)
{
  FILE *file;
  char *path = g_strdup_printf ("generated-code/%s", filename);
  GList *tmp;

  file = fopen (path, "w");
  if (!file)
    {
      char *error = g_strdup_printf ("Failed to open %s", filename);
      perror (error);
      g_free (error);
      g_free (path);
      return;
    }

  for (tmp = parts; tmp != NULL; tmp = tmp->next)
    {
      GXGenPart *part = tmp->data;
      fwrite (part->string->str, 1, part->string->len, file);
    }

  fclose (file);

  g_free (path);
  return;
}

static void
write_part_to_file (GXGenOutputContext *output_context,
		    const char *filename,
		    const char *part_name)
{
  GList *parts = NULL;
  GList *tmp;

  for (tmp = output_context->parts; tmp != NULL; tmp = tmp->next)
    {
      GXGenPart *part = tmp->data;
      if (strcmp (part->name, part_name) == 0)
	{
	  parts = g_list_prepend (parts, part);
	  break;
	}
    }
  if (!parts)
    return;
  write_parts_to_file (output_context, filename, parts);
  g_list_free (parts);
}

static void
write_object_parts_to_files (GXGenOutputContext *output_context,
			     const char *object_name,
			     const XGenExtension *extension)
{
  GList *tmp;
  char *obj_pattern = g_strdup_printf ("O@%s:", object_name);
  char *ext_pattern = g_strdup_printf ("E@%s:", extension->name);
  GList *header_parts = NULL;
  GList *c_code_parts = NULL;
  //GXGenPart *c_funcs_part = NULL;
  GXGenPart *h_header, *h_footer = NULL;
  char *filename;

  for (tmp = output_context->parts; tmp != NULL; tmp = tmp->next)
    {
      GXGenPart *part = tmp->data;
      gchar **part_fields;
      int i;
      char *name = NULL;

      if (!strstr (part->name, obj_pattern)
	  || !strstr (part->name, ext_pattern))
	continue;

      part_fields = g_strsplit (part->name, ":", 0);
      for (i = 0; part_fields[i]; i++)
	{
	  name = strstr (part_fields[i], "N@");
	  if (name)
	    break;
	}
      g_assert (name);
      name += 2;

      if (strstr (name, "typedefs"))
	header_parts = g_list_prepend (header_parts, part);
      else if (strstr (name, "protos"))
	header_parts = g_list_append (header_parts, part);
      else if (strstr (name, "h-header"))
	h_header = part;
      else if (strstr (name, "h-footer"))
	h_footer = part;
      else if (strstr (name, "c-header"))
	c_code_parts = g_list_prepend (c_code_parts, part);
      else if (strstr (name, "c-funcs"))
	c_code_parts = g_list_append (c_code_parts, part);
      else
	g_warning ("Un-recognised object part %s\n", name);

      g_strfreev (part_fields);
    }

  g_free (obj_pattern);
  g_free (ext_pattern);

  header_parts = g_list_prepend (header_parts, h_header);
  header_parts = g_list_append (header_parts, h_footer);

  filename = g_strdup_printf ("gx-%s-%s-gen.h",
			      object_name,
			      extension->header);
  write_parts_to_file (output_context, filename,
		       header_parts);
  g_free (filename);

  filename = g_strdup_printf ("gx-%s-%s-gen.c",
			      object_name,
			      extension->header);
  write_parts_to_file (output_context, filename,
		       c_code_parts);
  g_free (filename);
}

static void
write_all_gx_code (GXGenOutputContext *output_context)
{
  const XGenState *state = output_context->state;
  GList *tmp;

  for (tmp = state->extensions; tmp != NULL; tmp = tmp->next)
    {
      const XGenExtension *extension = tmp->data;
      char *extension_header =
	g_strdup_printf ("extensions/gx-%s.h", extension->header);
      char *extension_header_part_name =
	g_strdup_printf ("%s-includes", extension->header);
      char *extension_main_c =
	g_strdup_printf ("gx-%s-main-gen.c", extension->header);
      char *extension_main_c_part_name =
	g_strdup_printf ("%s-main-c", extension->header);
      char *extension_event_codes =
	g_strdup_printf ("gx-%s-event-codes-gen.h",
			 extension->header);
      char *extension_event_codes_part =
	g_strdup_printf ("%s-event-code-enums", extension->header);
      char *extension_event_details =
	g_strdup_printf ("gx-%s-event-details-gen.c",
			 extension->header);
      char *extension_event_details_part =
	g_strdup_printf ("%s-event-details", extension->header);
      char *extension_error_codes =
	g_strdup_printf ("gx-%s-protocol-error-codes-gen.h",
			 extension->header);
      char *extension_error_codes_part =
	g_strdup_printf ("%s-error-code-enums", extension->header);
      char *extension_error_details =
	g_strdup_printf ("gx-%s-protocol-error-details-gen.c",
			 extension->header);
      char *extension_error_details_part =
	g_strdup_printf ("%s-error-details", extension->header);

      write_part_to_file (output_context, extension_header,
			  extension_header_part_name);
      g_free (extension_header);
      g_free (extension_header_part_name);

      write_part_to_file (output_context, extension_main_c,
			  extension_main_c_part_name);
      g_free (extension_main_c);
      g_free (extension_main_c_part_name);

      write_part_to_file (output_context, extension_event_codes,
			  extension_event_codes_part);
      g_free (extension_event_codes);
      g_free (extension_event_codes_part);

      write_part_to_file (output_context, extension_event_details,
			  extension_event_details_part);
      g_free (extension_event_details);
      g_free (extension_event_details_part);

      write_part_to_file (output_context, extension_error_codes,
			  extension_error_codes_part);
      g_free (extension_error_codes);
      g_free (extension_error_codes_part);

      write_part_to_file (output_context, extension_error_details,
			  extension_error_details_part);
      g_free (extension_error_details);
      g_free (extension_error_details_part);

      write_object_parts_to_files (output_context,
				   "connection",
				   tmp->data);
      write_object_parts_to_files (output_context,
				   "drawable",
				   tmp->data);
      write_object_parts_to_files (output_context,
				   "pixmap",
				   tmp->data);
      write_object_parts_to_files (output_context,
				   "window",
				   tmp->data);
      write_object_parts_to_files (output_context,
				   "gcontext",
				   tmp->data);
    }

  write_part_to_file (output_context, "gx-cookie-gen.h",
		      "cookie-typedefs");
  write_part_to_file (output_context, "gx-protocol-error-codes-gen.h",
		      "error-code-enums");
  write_part_to_file (output_context, "gx-protocol-error-details-gen.c",
		      "error-details");
  write_part_to_file (output_context, "gx-xcb-dependencies-gen.h",
		      "xcb-dependencies");
  write_part_to_file (output_context, "gxgen-tests-gen.c",
		      "gxgen-tests");
}

static XGenEventHandlers handlers =
{
  .definition_notify = gxgen_definition_notify
};

int
main (int argc, char **argv)
{
  int i;
  GList *files = NULL;
  XGenState *state;
  GXGenOutputContext *output_context;

  xgen_set_handlers (&handlers);

  /* The regex used for splitting CamelCase names.
   * NB: this is taken from xcb so we get the same splitting */
  cname_regex = g_regex_new (GXGEN_WORD_SPLIT_REGEX, 0, 0, NULL);

  for (i = 1; i < argc && argv[i]; i++)
    files = g_list_prepend (files, g_strdup (argv[i]));

  state = xgen_parse_xcb_proto_files (files);
  if (!state)
    g_error ("Failed to parse XCB proto files\n");

  g_list_foreach (files, (GFunc)g_free, NULL);
  g_list_free (files);

  output_context = output_everything_to_part_buffers (state);
  write_all_gx_code (output_context);

  return 0;
}

