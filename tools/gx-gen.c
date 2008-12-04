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
  out (output_context, output_context->h_typedef_part, __VA_ARGS__)

/**
 * Some macro sugar for outputing a formatted string to the current
 * headers function prototype section
 */
#define _H(...) \
  out (output_context, output_context->h_part, __VA_ARGS__)

/**
 * Some macro sugar for outputing a formatted string to the current
 * C file and current headers prototype section
 */
#define _CH(...) \
  out2 (output_context, \
        output_context->c_part, \
	output_context->h_part, \
	__VA_ARGS__)

typedef enum
{
  GXGEN_PART_CONNECTION_OBJ_H_INC,
  GXGEN_PART_CONNECTION_OBJ_H_TYPEDEFS,
  GXGEN_PART_CONNECTION_OBJ_H_MACROS,
  GXGEN_PART_CONNECTION_OBJ_H_PROTOS,

  GXGEN_PART_CONNECTION_OBJ_C_INC,
  GXGEN_PART_CONNECTION_OBJ_C_PROTOS,
  GXGEN_PART_CONNECTION_OBJ_C_FUNCS,

  GXGEN_PART_DRAWABLE_OBJ_H_INC,
  GXGEN_PART_DRAWABLE_OBJ_H_TYPEDEFS,
  GXGEN_PART_DRAWABLE_OBJ_H_MACROS,
  GXGEN_PART_DRAWABLE_OBJ_H_PROTOS,

  GXGEN_PART_DRAWABLE_OBJ_C_INC,
  GXGEN_PART_DRAWABLE_OBJ_C_PROTOS,
  GXGEN_PART_DRAWABLE_OBJ_C_FUNCS,

  GXGEN_PART_PIXMAP_OBJ_H_INC,
  GXGEN_PART_PIXMAP_OBJ_H_TYPEDEFS,
  GXGEN_PART_PIXMAP_OBJ_H_MACROS,
  GXGEN_PART_PIXMAP_OBJ_H_PROTOS,

  GXGEN_PART_PIXMAP_OBJ_C_INC,
  GXGEN_PART_PIXMAP_OBJ_C_PROTOS,
  GXGEN_PART_PIXMAP_OBJ_C_FUNCS,

  GXGEN_PART_WINDOW_OBJ_H_INC,
  GXGEN_PART_WINDOW_OBJ_H_TYPEDEFS,
  GXGEN_PART_WINDOW_OBJ_H_MACROS,
  GXGEN_PART_WINDOW_OBJ_H_PROTOS,

  GXGEN_PART_WINDOW_OBJ_C_INC,
  GXGEN_PART_WINDOW_OBJ_C_PROTOS,
  GXGEN_PART_WINDOW_OBJ_C_FUNCS,

  GXGEN_PART_GCONTEXT_OBJ_H_INC,
  GXGEN_PART_GCONTEXT_OBJ_H_TYPEDEFS,
  GXGEN_PART_GCONTEXT_OBJ_H_MACROS,
  GXGEN_PART_GCONTEXT_OBJ_H_PROTOS,

  GXGEN_PART_GCONTEXT_OBJ_C_INC,
  GXGEN_PART_GCONTEXT_OBJ_C_PROTOS,
  GXGEN_PART_GCONTEXT_OBJ_C_FUNCS,

  GXGEN_PART_COOKIE_OBJ_H_TYPEDEFS,

  GXGEN_PART_ERROR_CODES_H_ENUMS,
  GXGEN_PART_ERROR_DETAILS_C,

  GXGEN_PART_XCB_DEPENDENCIES_H,

  GXGEN_PART_TESTS_C,

  GXGEN_PART_COUNT
} GXGenPart;


typedef enum {
  GXGEN_IS_CONNECTION_OBJ,
  GXGEN_IS_DRAWABLE_OBJ,
  GXGEN_IS_PIXMAP_OBJ,
  GXGEN_IS_WINDOW_OBJ,
  GXGEN_IS_GCONTEXT_OBJ
} GXGenOutputObjectType;

typedef struct _GXGenOutputObject {
  GXGenOutputObjectType	 type;
  /* lc=lowercase
   * cc=camel case
   * uc=uppercase
   */
  const char		*name_cc;
  char			*name_uc;
  char			*name_lc;
  const char		*first_arg;
  XGenFieldDefinition	*first_object_field;
  GXGenPart		 h_typedefs, h_protos, c_funcs;
} GXGenOutputObject;

typedef struct _GXGenOutputRequest
{
  const XGenRequest *request;
  /* lc=lowercase
   * cc=camel case
   * uc=uppercase
   */
  const char	    *xcb_name_lc;
  const char	    *xcb_name_cc;
  const char	    *xcb_name_uc;
  const char	    *gx_name_lc;
  const char	    *gx_name_cc;
  const char	    *gx_name_uc;
} GXGenOutputRequest;

typedef struct _GXGenOutputNamespace
{
  /* lc=lowercase
   * cc=camel case
   * uc=uppercase
   */
  char *gx_lc;
  char *gx_uc;
  char *gx_cc;
  char *xcb_lc;
  char *xcb_cc;
  char *xcb_uc;
} GXGenOutputNamespace;

typedef struct _GXGenOutputContext
{
  const XGenState	       *state;
  GString		      **parts;
  const XGenExtension	       *extension;
  const GXGenOutputObject      *obj;
  const GXGenOutputRequest     *out_request;
  const GXGenOutputNamespace   *namespace;
  GXGenPart			h_typedef_part;
  GXGenPart			h_part;
  GXGenPart			c_part;
} GXGenOutputContext;


static GRegex *cname_regex;


static char *
gxgen_get_lowercase_name (const char *name)
{
  gint pos;
  GString *new_name;
  gchar *tmp, *ret;
  char **strv;
  int i;

  g_return_val_if_fail (name && name[0], NULL);
  new_name = g_string_new ("");

  strv = g_regex_split(cname_regex, name, 0);
  for (i = 0; strv[i]; i++)
    {
      char *str = strv[i];
      if (strcmp (str, "") == 0)
	continue;
      new_name = g_string_append (new_name, str);
      new_name = g_string_append_c (new_name, '_');
    }
  new_name = g_string_set_size (new_name, new_name->len-1);

  tmp = g_string_free (new_name, FALSE);
  ret = g_ascii_strdown (tmp, -1);
  g_free (tmp);

  return ret;
}

static char *
gxgen_get_uppercase_name (const char *name)
{
  char *name_lc = gxgen_get_lowercase_name (name);
  char *name_uc = g_ascii_strup (name_lc, -1);
  g_free (name_lc);
  return name_uc;
}

static char *
gxgen_get_camelcase_name (const char *name)
{
  char *name_cc = g_strdup (name);
  int i;
  char *str;

  g_return_val_if_fail (name && name[0], NULL);

  for (i = 0; camelcase_dictionary[i].uppercase; i++)
    while ((str = strstr (name_cc, camelcase_dictionary[i].uppercase)))
      memcpy (str, camelcase_dictionary[i].camelcase,
	      strlen(camelcase_dictionary[i].uppercase));

  /* At the very least; if a lower case name is passed then we
   * uppercase the first letter: */
  /* FIXME - this should also use a dictionary */
  if (g_ascii_islower (name_cc[0]))
    name_cc[0] = g_ascii_toupper (name_cc[0]);

  return name_cc;
}

/* TODO:
 * Implement a neater way to solve the problem that "namespaces" are currently
 * used for.
 *
 * Can we just add private GX fields to request/reply/error/typedef definitions
 * and simply store the determined XCB and GX names of all the definitions
 * up front at parse time.
 *
 * I.e. if there was a callback from the XGen code for each new definition
 * that gets parsed, we could probably just have a single function for
 * determining all the XCB and GX names.
 *
 * Then when we come to emit the code it's just a case of picking the right
 * variation of the name (uppercase/lowercase, GX/XCB etc) instead of
 * repeatedly compositing the names from namespace prefixes as we currently do.
 */

GXGenOutputNamespace *
setup_request_namespace (const XGenExtension *extension,
			 const GXGenOutputObject *object)
{
  GXGenOutputNamespace *namespace = g_new0 (GXGenOutputNamespace, 1);

  if (strcmp (extension->header, "xproto") == 0)
    {
      namespace->gx_cc = g_strdup (object->name_cc);
      namespace->gx_uc = g_strdup_printf ("%s_", object->name_uc);
      namespace->gx_lc = g_strdup_printf ("%s_", object->name_lc);
    }
  else
    {
      char *extension_cc = gxgen_get_camelcase_name (extension->header);
      char *extension_uc = gxgen_get_uppercase_name (extension->header);
      char *extension_lc = gxgen_get_lowercase_name (extension->header);
      namespace->gx_cc =
	g_strdup_printf ("%s%s",
			 object->name_cc,
			 extension_cc);
      namespace->gx_uc =
	g_strdup_printf ("%s_%s_",
			 object->name_uc,
			 extension_uc);
      namespace->gx_lc =
	g_strdup_printf ("%s_%s_",
			 object->name_lc,
			 extension_lc);
      g_free (extension_cc);
      g_free (extension_uc);
      g_free (extension_lc);
    }

  if (strcmp (extension->header, "xproto") == 0)
    {
      namespace->xcb_lc = g_strdup ("");
      namespace->xcb_cc = g_strdup ("");
      namespace->xcb_uc = g_strdup ("");
    }
  else
    {
      char *extension_lc = gxgen_get_lowercase_name (extension->header);
      namespace->xcb_lc =
	g_strdup_printf ("%s_", extension_lc);
      namespace->xcb_cc = g_strdup ("FIXME");
      namespace->xcb_uc = g_strdup ("FIXME");

      g_free (extension_lc);
    }

  return namespace;
}

GXGenOutputNamespace *
setup_data_type_namespace (const XGenExtension *extension)
{
  GXGenOutputNamespace *namespace = g_new0 (GXGenOutputNamespace, 1);

  if (strcmp (extension->header, "xproto") == 0)
    {
      namespace->gx_lc = g_strdup ("");
      namespace->gx_uc = g_strdup ("");
      namespace->gx_cc = g_strdup ("");
    }
  else
    {
      namespace->gx_cc = gxgen_get_camelcase_name (extension->header);
      namespace->gx_uc = gxgen_get_uppercase_name (extension->header);
      namespace->gx_lc = gxgen_get_lowercase_name (extension->header);
    }

  if (strcmp (extension->header, "xproto") == 0)
    {
      namespace->xcb_lc = g_strdup ("");
      namespace->xcb_uc = g_strdup ("");
      namespace->xcb_cc = g_strdup ("");
    }
  else
    {
      char *extension_lc = gxgen_get_lowercase_name (extension->header);

      namespace->xcb_lc =
	g_strdup_printf ("%s_", extension_lc);
      namespace->xcb_cc = g_strdup ("FIXME");
      namespace->xcb_uc = g_strdup ("FIXME");

      g_free (extension_lc);
    }

  return namespace;
}

static void
free_namespace (GXGenOutputNamespace *namespace)
{
  g_free (namespace->gx_cc);
  g_free (namespace->gx_uc);
  g_free (namespace->gx_lc);
  g_free (namespace->xcb_cc);
  g_free (namespace->xcb_uc);
  g_free (namespace->xcb_lc);
  g_free (namespace);
}

static char *
gxgen_definition_to_gx_type (XGenDefinition * definition,
			     gboolean object_types)
{
  struct _TypeMapping *mapping;
  char *name_cc;
  char *ret;
  GXGenOutputNamespace *namespace;

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

  namespace = setup_data_type_namespace (definition->extension);

  name_cc = gxgen_get_camelcase_name (definition->name);
  ret = g_strdup_printf ("GX%s%s", namespace->gx_cc, name_cc);
  g_free (name_cc);

  return ret;
}

static void
out (GXGenOutputContext *output_context,
     GXGenPart part,
     const char *format, ...)
{
  va_list ap;

  va_start (ap, format);
  g_string_append_vprintf (output_context->parts[part], format, ap);
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
      GXGenPart part0,
      GXGenPart part1,
      const char *format, ...)
{
  va_list ap;

  va_start (ap, format);
  g_string_append_vprintf (output_context->parts[part0], format, ap);
  g_string_append_vprintf (output_context->parts[part1], format, ap);
  va_end (ap);
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
  if (strstr (field->definition->name, "64"))
    g_print ("DEBUG: int64\n");
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
      _C ("gx_gcontext_get_xcb_gcontext (%s)",
	  field->name);
    }
  else if (field->length == NULL
	   && (field->definition->type == XGEN_STRUCT
	       || field->definition->type == XGEN_UNION))
    {
      GXGenOutputNamespace *namespace =
	setup_data_type_namespace (field->definition->extension);
      char * type_lc = gxgen_get_lowercase_name (field->definition->name);
      /* NB: XCB passes structures by value,
       * while GX passes them by reference */
      _C ("*(xcb_%s%s_t *)%s",
	  namespace->xcb_lc,
	  type_lc,
	  field->name);

      g_free (type_lc);
      free_namespace (namespace);
    }
  else
    {
      _C ("%s", field->name);
    }
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
  output_context->h_typedef_part = GXGEN_PART_CONNECTION_OBJ_H_TYPEDEFS;

  for (tmp = extension->all_definitions; tmp != NULL; tmp = tmp->next)
    {
      XGenDefinition *def = tmp->data;

      if (def->type == XGEN_BOOLEAN
	  || def->type == XGEN_CHAR
	  || def->type == XGEN_SIGNED || def->type == XGEN_UNSIGNED)
	{

	  if (strcmp (def->name, "char") == 0)
	    continue;

	  typedef_type_cc = gxgen_definition_to_gx_type (def, FALSE);
	  typedef_name_cc = gxgen_get_camelcase_name (def->name);

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

      XGenTypedef *typedef_def = XGEN_TYPEDEF_DEF (def);
      typedef_type_cc =
	gxgen_get_camelcase_name (typedef_def->reference->name);
      typedef_name_cc = gxgen_definition_to_gx_type (def, FALSE);

      _TD ("typedef %s %s;\n", typedef_type_cc, typedef_name_cc);

      g_free (typedef_type_cc);
      g_free (typedef_name_cc);
    }
  _TD ("\n\n\n");
}

static void
output_structs_and_unions (GXGenOutputContext *output_context)
{
  const XGenExtension *extension = output_context->extension;
  GList *tmp;

  /* For outputting via _TD()... */
  output_context->h_typedef_part = GXGEN_PART_CONNECTION_OBJ_H_TYPEDEFS;

  for (tmp = extension->all_definitions; tmp != NULL; tmp = tmp->next)
    {
      XGenDefinition *def = tmp->data;
      guint pad = 0;
      GList *tmp2;
      GXGenOutputNamespace *namespace;
      char *name_cc;

      if (def->type != XGEN_STRUCT && def->type != XGEN_UNION)
	continue;

      /* Some types are special cased if they are represented as
       * objects */
      if (strcmp (def->name, "SCREEN") == 0)
	continue;

      namespace = setup_data_type_namespace (def->extension);

      _TD ("typedef %s {\n",
	   def->type == XGEN_STRUCT ? "struct" : "union");

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

      /* _TD (""); */
      name_cc = gxgen_get_camelcase_name (def->name);
      _TD ("} GX%s%s;\n\n", namespace->gx_cc, name_cc);
      g_free (name_cc);

      free_namespace (namespace);
    }
  _TD ("\n");
}

static void
output_enums (GXGenOutputContext *output_context)
{
  const XGenExtension *extension = output_context->extension;
  GList *tmp;

  /* For outputting via _TD()... */
  output_context->h_typedef_part = GXGEN_PART_CONNECTION_OBJ_H_TYPEDEFS;

  for (tmp = extension->enums; tmp != NULL; tmp = tmp->next)
    {
      XGenDefinition *def = tmp->data;
      GList *tmp2;
      GXGenOutputNamespace *namespace;
      char *name_cc;

      namespace = setup_data_type_namespace (def->extension);

      _TD ("typedef enum\n{\n");

      for (tmp2 = XGEN_ENUM_DEF (def)->items; tmp2 != NULL; tmp2 = tmp2->next)
	{
	  XGenItemDefinition *item = tmp2->data;
	  char *enum_stem_uc = gxgen_get_uppercase_name (item->name);
	  char *enum_prefix_uc = gxgen_get_uppercase_name (def->name);

	  if (item->type == XGEN_ITEM_AS_VALUE)
	    {
	      _TD (" GX_%s%s_%s = %s,\n",
		   namespace->gx_uc,
		   enum_prefix_uc,
		   enum_stem_uc,
		   item->value);
	    }
	  else
	    {
	      _TD (" GX_%s%s_%s = (1 << %u),\n",
		   namespace->gx_uc,
		   enum_prefix_uc,
		   enum_stem_uc,
		   item->bit);
	    }

	  g_free (enum_stem_uc);
	  g_free (enum_prefix_uc);
	}
      name_cc = gxgen_get_camelcase_name (def->name);
      _TD ("} GX%s%s;\n\n", namespace->gx_cc, name_cc);
      g_free (name_cc);

      free_namespace (namespace);
    }
}

static GXGenOutputObject *
setup_output_object (XGenRequest *request)
{
  GXGenOutputObject *obj = g_new0(GXGenOutputObject, 1);
  GList *tmp;

  /* FIXME - this could be implemented using static
   * descriptions of the objects. */

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
	      obj->type = GXGEN_IS_WINDOW_OBJ;
	      obj->name_cc = "Window";
	      obj->first_arg = "GXWindow *window";
	      obj->first_object_field = field;
	      obj->h_typedefs = GXGEN_PART_WINDOW_OBJ_H_TYPEDEFS;
	      obj->h_protos = GXGEN_PART_WINDOW_OBJ_H_PROTOS;
	      obj->c_funcs = GXGEN_PART_WINDOW_OBJ_C_FUNCS;
	      break;
	    }
	  else if (strcmp (field->name, "pixmap") == 0)
	    {
	      obj->type = GXGEN_IS_PIXMAP_OBJ;
	      obj->name_cc = "Pixmap";
	      obj->first_arg = "GXPixmap *pixmap";
	      obj->first_object_field = field;
	      obj->h_typedefs = GXGEN_PART_PIXMAP_OBJ_H_TYPEDEFS;
	      obj->h_protos = GXGEN_PART_PIXMAP_OBJ_H_PROTOS;
	      obj->c_funcs = GXGEN_PART_PIXMAP_OBJ_C_FUNCS;
	      break;
	    }
	  else if (strcmp (field->name, "drawable") == 0)
	    {
	      obj->type = GXGEN_IS_DRAWABLE_OBJ;
	      obj->name_cc = "Drawable";
	      obj->first_arg = "GXDrawable *drawable";
	      obj->first_object_field = field;
	      obj->h_typedefs = GXGEN_PART_DRAWABLE_OBJ_H_TYPEDEFS;
	      obj->h_protos = GXGEN_PART_DRAWABLE_OBJ_H_PROTOS;
	      obj->c_funcs = GXGEN_PART_DRAWABLE_OBJ_C_FUNCS;
	      break;
	    }
	  else if (strcmp (field->name, "gc") == 0)
	    {
	      obj->type = GXGEN_IS_GCONTEXT_OBJ;
	      obj->name_cc = "GContext";
	      obj->first_arg = "GXGContext *gc";
	      obj->first_object_field = field;
	      obj->h_typedefs = GXGEN_PART_GCONTEXT_OBJ_H_TYPEDEFS;
	      obj->h_protos = GXGEN_PART_GCONTEXT_OBJ_H_PROTOS;
	      obj->c_funcs = GXGEN_PART_GCONTEXT_OBJ_C_FUNCS;
	      break;
	    }
	  //else
	  //  break;
	}
    }

  /* By default requests become members of the GXConnection
   * object */
  if (!obj->name_cc)
    {
      obj->type = GXGEN_IS_CONNECTION_OBJ;
      obj->name_cc = "Connection";
      obj->first_arg = "GXConnection *connection";
      obj->h_typedefs = GXGEN_PART_CONNECTION_OBJ_H_TYPEDEFS;
      obj->h_protos = GXGEN_PART_CONNECTION_OBJ_H_PROTOS;
      obj->c_funcs = GXGEN_PART_CONNECTION_OBJ_C_FUNCS;
    }

  obj->name_lc = gxgen_get_lowercase_name (obj->name_cc);
  obj->name_uc = gxgen_get_uppercase_name (obj->name_cc);

  return obj;
}

void
free_output_object (GXGenOutputObject *obj)
{
  g_free (obj->name_uc);
  g_free (obj);
}

static void
output_reply_typedef (GXGenOutputContext *output_context)
{
  const GXGenOutputRequest *out_request = output_context->out_request;
  const XGenRequest *request = out_request->request;
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
  _TD ("\n} GX%s%sX11Reply;\n\n",
       output_context->namespace->gx_cc,
       out_request->gx_name_cc);

  _TD ("typedef struct {\n");
  _TD ("\tGXConnection *connection;\n");
  _TD ("\tGX%s%sX11Reply *x11_reply;\n",
       output_context->namespace->gx_cc,
       out_request->gx_name_cc);
  _TD ("\n} GX%s%sReply;\n\n",
       output_context->namespace->gx_cc,
       out_request->gx_name_cc);
}

static void
output_reply_list_get (GXGenOutputContext *output_context)
{
  const GXGenOutputRequest *out_request = output_context->out_request;
  const XGenRequest *request = out_request->request;
  const GXGenOutputObject *obj = output_context->obj;
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

  _CH ("gx_%s%s_get_%s (GX%s%sReply *%s_reply)",
       output_context->namespace->gx_lc,
       out_request->gx_name_lc,
       field->name,
       output_context->namespace->gx_cc,
       out_request->gx_name_cc,
       out_request->gx_name_lc);

  _H (";\n");
  _C ("\n{\n");

  _C ("  %s *p = (%s *)(%s_reply->x11_reply + 1);\n",
      gxgen_definition_to_gx_type (field->definition, FALSE),
      gxgen_definition_to_gx_type (field->definition, FALSE),
      out_request->gx_name_lc);

  if (is_special_xid_definition (field->definition))
    _C ("  GList *tmp = NULL;\n");
  else
    _C ("  GArray *tmp;\n");

  _C ("\n");

  _C ("  /* It's possible the connection has been closed since the reply\n"
      "   * was recieved: (the reply struct contains weak pointer) */\n"
      "  if (!%s_reply->connection)\n"
      "    return NULL;\n",
      out_request->gx_name_lc);

  if (is_special_xid_definition (field->definition))
    _C ("  int i;\n");
  else
    _C ("  tmp = g_array_new (TRUE, FALSE, sizeof(%s));\n",
        gxgen_definition_to_gx_type (field->definition, FALSE));

  if (is_special_xid_definition (field->definition))
    {
      _C ("  for (i = 0; i< %s_reply->x11_reply->%s; i++)\n"
	  "    {\n",
	  out_request->gx_name_lc,
	  field->length->field);

      _C ("      /* FIXME - mutex */\n");

      _C ("      %s item = _gx_%s_find_from_xid (p[i]);\n",
	  gxgen_definition_to_gx_type (field->definition, TRUE),
	  obj->name_lc);

      _C (
       "      if (!item)\n"
       "	item = g_object_new (gx_%s_get_type(),\n"
       "			     \"connection\", %s_reply->connection,\n"
       "			     \"xid\", p[i],\n"
       "			     \"wrap\", TRUE,\n"
       "			     NULL);\n",
       obj->name_lc, out_request->gx_name_lc);
      _C ("      tmp = g_list_prepend (tmp, item);\n");
      _C ("    }\n");
    }
  else
    {
      _C ("  tmp = g_array_append_vals (tmp, p, %s_reply->x11_reply->%s);\n",
	  out_request->gx_name_lc, field->length->field);
    }

  if (is_special_xid_definition (field->definition))
    _C ("  tmp = g_list_reverse (tmp);");

  _C ("  return tmp;\n");

  _C ("}\n");

}

static void
output_reply_list_free (GXGenOutputContext *output_context)
{
  const GXGenOutputRequest *out_request = output_context->out_request;
  const XGenRequest *request = out_request->request;
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
  _CH ("gx_%s%s_free_%s (",
       output_context->namespace->gx_lc,
       out_request->gx_name_lc,
       field->name);
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
  const GXGenOutputRequest *out_request = output_context->out_request;
  const XGenRequest *request = out_request->request;

  if (!request->reply)
    return;

  _CH ("void\n"
       "gx_%s%s_reply_free (GX%s%sReply *%s_reply)",
       output_context->namespace->gx_lc,
       out_request->gx_name_lc,
       output_context->namespace->gx_cc,
       out_request->gx_name_cc,
       out_request->gx_name_lc);

  _H (";\n");
  _C ("\n{\n");

  _C ("  free (%s_reply->x11_reply);\n",
      out_request->gx_name_lc);
  _C ("  g_slice_free (GX%s%sReply, %s_reply);\n",
      output_context->namespace->gx_cc,
      out_request->gx_name_cc,
      out_request->gx_name_lc);

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
  const GXGenOutputRequest *out_request = output_context->out_request;
  const XGenRequest *request = out_request->request;

  _C ("\txcb_generic_error_t *xcb_error;\n");

  if (request->reply)
    {
      _C ("\tGX%s%sReply *reply = g_slice_new (GX%s%sReply);\n",
	  output_context->namespace->gx_cc,
	  out_request->gx_name_cc,
	  output_context->namespace->gx_cc,
	  out_request->gx_name_cc);
    }
}

/**
 * output_async_request:
 *
 * This function outputs the code for all gx_*_async () functions
 */
void
output_async_request (GXGenOutputContext *output_context)
{
  const GXGenOutputRequest *out_request = output_context->out_request;
  const XGenRequest *request = out_request->request;
  const GXGenOutputObject *obj = output_context->obj;
  GList *tmp;
  char *cookie_type_uc;
  gboolean has_mask_value_items = FALSE;

  if (strcmp (output_context->extension->header, "xevie") == 0
      && strcmp (XGEN_DEF (request)->name, "Send") == 0)
    g_print ("DEBUG xevie send\n");

  _CH ("\nGXCookie *\ngx_%s%s_async (%s",
       output_context->namespace->gx_lc,
       out_request->gx_name_lc,
       obj->first_arg);

  for (tmp = request->fields; tmp != NULL; tmp = tmp->next)
    {
      XGenFieldDefinition *field = tmp->data;
      const char *type;

      if (strcmp (field->name, "opcode") == 0
	  || strcmp (field->name, "pad") == 0
	  || strcmp (field->name, "length") == 0)
	continue;

      if (obj->first_object_field && field == obj->first_object_field)
	continue;

      if ((obj->type == GXGEN_IS_WINDOW_OBJ
	   && strcmp (field->name, "window") == 0)
	  || (obj->type == GXGEN_IS_DRAWABLE_OBJ
	      && strcmp (field->name, "drawable") == 0))
	continue;

      type = gxgen_definition_to_gx_type (field->definition, TRUE);

      if (field->length)
	_CH (",\n\t\tconst %s *%s", type, field->name);
      else if (field->definition->type == XGEN_VALUEPARAM)
	{
	  _CH (",\n\t\tGXMaskValueItem *mask_value_items");
	  has_mask_value_items = TRUE;
	}
      else if (field->definition->type == XGEN_STRUCT
	       || field->definition->type == XGEN_UNION)
	{
	  _CH (",\n\t\t%s *%s", type, field->name);
	}
      else
	_CH (",\n\t\t%s %s", type, field->name);
    }
  _H (");\n\n");
  _C (")\n{\n");

  /*
   * *_async() code
   */
  if (!obj->type == GXGEN_IS_CONNECTION_OBJ)
    {
      _C ("\tGXConnection *connection = gx_%s_get_connection (%s);\n",
	   obj->name_lc, obj->name_lc);
    }

  if (!request->reply)
    _C ("\txcb_void_cookie_t xcb_cookie;\n");
  else
    _C ("\txcb_%s%s_cookie_t xcb_cookie;\n",
	output_context->namespace->xcb_lc,
	out_request->xcb_name_lc);

  _C ("\tGXCookie *cookie;\n\n");

  if (has_mask_value_items)
    output_mask_value_variable_declarations (output_context);

  _C ("\n");

  if (request->reply)
    {
      _C ("\txcb_cookie =\n"
	  "\t\txcb_%s%s(\n"
	  "\t\t\tgx_connection_get_xcb_connection (connection)",
	  output_context->namespace->xcb_lc,
	  out_request->xcb_name_lc);
    }
  else
    {
      _C ("\txcb_cookie =\n"
	  "\t\txcb_%s%s_checked (\n"
	  "\t\t\tgx_connection_get_xcb_connection (connection)",
	  output_context->namespace->xcb_lc,
	  out_request->xcb_name_lc);
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

  cookie_type_uc =
    g_strdup_printf ("GX_COOKIE_%s%s",
		     output_context->namespace->gx_uc,
		     out_request->gx_name_uc);

  out (output_context,
       GXGEN_PART_COOKIE_OBJ_H_TYPEDEFS,
       "\t%s,\n", cookie_type_uc);

  _C ("\tcookie = gx_cookie_new (connection, %s, xcb_cookie.sequence);\n",
      cookie_type_uc);

  g_free (cookie_type_uc);

  if (!obj->type == GXGEN_IS_CONNECTION_OBJ)
    _C ("\tg_object_unref (connection);\n");

  _C (
       "\tgx_connection_register_cookie (connection, cookie);\n");

  _C ("\treturn cookie;\n");

  _C ("}\n");
}

void
output_reply (GXGenOutputContext *output_context)
{
  const GXGenOutputRequest *out_request = output_context->out_request;
  const XGenRequest *request = out_request->request;

  if (request->reply)
    {
      _CH ("\nGX%s%sReply *\n",
	   output_context->namespace->gx_cc,
	   out_request->gx_name_cc);
    }
  else
    _CH ("\ngboolean\n");

  _CH ("gx_%s%s_reply (GXCookie *cookie, GError **error)\n",
       output_context->namespace->gx_lc, out_request->gx_name_lc);

  _H (";\n");
  _C ("\n{\n");

  _C ("\tGXConnection *connection = gx_cookie_get_connection (cookie);\n");

  if (!request->reply)
    _C ("\txcb_void_cookie_t xcb_cookie;\n");
  else
    {
      _C ("\txcb_%s%s_cookie_t xcb_cookie;\n",
	   output_context->namespace->xcb_lc, out_request->xcb_name_lc);
    }
  output_reply_variable_declarations (output_context);
  _C ("\n");

  _C ("\tg_return_val_if_fail (error == NULL || *error == NULL, %s);\n",
      request->reply == NULL ? "FALSE" : "NULL");

  _C ("\n");

  if (request->reply)
    _C ("\treply->connection = connection;\n\n");

  if (request->reply)
    {
      _C ("\treply->x11_reply = (GX%s%sX11Reply *)\n"
	  "\t\tgx_cookie_get_reply (cookie);\n",
	  output_context->namespace->gx_cc,
	  out_request->gx_name_cc);

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
      "\t\t\tgx_protocol_error_from_xcb_generic_error (xcb_error),\n"
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
      _C ("\treply->x11_reply = (GX%s%sX11Reply *)\n"
	  "\t\txcb_%s%s_reply (\n"
	  "\t\t\tgx_connection_get_xcb_connection (connection),\n"
	  "\t\t\txcb_cookie,\n"
	  "\t\t\t&xcb_error);\n",
	  output_context->namespace->gx_cc,
	  out_request->gx_name_cc,
	  output_context->namespace->xcb_lc,
	  out_request->xcb_name_lc);
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
      "\t\t\tgx_protocol_error_from_xcb_generic_error (xcb_error),\n"
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
  const GXGenOutputRequest *out_request = output_context->out_request;
  const XGenRequest *request = out_request->request;
  const GXGenOutputObject *obj = output_context->obj;
  gboolean has_mask_value_items = FALSE;
  GList *tmp;

  if (!request->reply)
    _CH ("\ngboolean\n");
  else
    {
      _CH ("\nGX%s%sReply *\n",
	   output_context->namespace->gx_cc,
	   out_request->gx_name_cc);
    }

  _CH ("gx_%s%s (%s",
       output_context->namespace->gx_lc, out_request->gx_name_lc, obj->first_arg);

  for (tmp = request->fields;
       tmp != NULL; tmp = tmp->next)
    {
      XGenFieldDefinition *field = tmp->data;
      const char *type;

      if (strcmp (field->name, "opcode") == 0
	  || strcmp (field->name, "pad") == 0
	  || strcmp (field->name, "length") == 0)
	continue;

      if (obj->first_object_field && field == obj->first_object_field)
	continue;

      type = gxgen_definition_to_gx_type (field->definition, TRUE);

      if (field->length)
	{
	  _CH (",\n\t\tconst %s *%s", type, field->name);
	}
      else if (field->definition->type == XGEN_VALUEPARAM)
	{
	  _CH (",\n\t\tGXMaskValueItem *mask_value_items");
	  has_mask_value_items = TRUE;
	}
      else if (field->definition->type == XGEN_STRUCT
	       || field->definition->type == XGEN_UNION)
	{
	  _CH (",\n\t\t%s *%s", type, field->name);
	}
      else
	  _CH (",\n\t\t%s %s", type, field->name);
    }
  _CH (",\n\t\tGError **error)");

  _H (";\n\n");
  _C ("\n{\n");

  if (!obj->type == GXGEN_IS_CONNECTION_OBJ)
    {
      _C ("\tGXConnection *connection = gx_%s_get_connection (%s);\n",
	  obj->name_lc, obj->name_lc);
    }

  if (!request->reply)
    _C ("\txcb_void_cookie_t cookie;\n");
  else
    _C ("\txcb_%s%s_cookie_t cookie;\n",
	 output_context->namespace->xcb_lc, out_request->xcb_name_lc);
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
	  "\t\txcb_%s%s (\n",
	  output_context->namespace->xcb_lc,
	  out_request->xcb_name_lc);
    }
  else
    {
      _C ("\tcookie =\n"
	  "\t\txcb_%s%s_checked (\n",
	  output_context->namespace->xcb_lc,
	  out_request->xcb_name_lc);
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
      _C ("\treply->x11_reply = (GX%s%sX11Reply *)\n"
	  "\t\txcb_%s%s_reply (\n"
	  "\t\t\tgx_connection_get_xcb_connection (connection),\n"
	  "\t\t\tcookie,\n"
	  "\t\t\t&xcb_error);\n",
	  output_context->namespace->gx_cc,
	  out_request->gx_name_cc,
	  output_context->namespace->xcb_lc,
	  out_request->xcb_name_lc);
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
      "\t\t\tgx_protocol_error_from_xcb_generic_error (xcb_error),\n"
      "\t\t\t\"Protocol Error\");\n"
      "\t\treturn %s;\n"
      "\t  }\n",
      request->reply != NULL ? "NULL" : "FALSE");

  if (!obj->type == GXGEN_IS_CONNECTION_OBJ)
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
      XGenDefinition *request_def = XGEN_DEF (request);
      GXGenOutputRequest *out_request;
      GXGenOutputObject *obj = NULL;
      GXGenOutputNamespace *namespace;
      gchar *gx_name;

      /* Some requests are special cased and implemented within object
       * constructors and so we don't emit code for them...
       */
      if (strcmp (request_def->name, "CreateWindow") == 0
	  || strcmp (request_def->name, "CreatePixmap") == 0
	  || strcmp (request_def->name, "CreateGC") == 0)
	continue;

      out_request = g_new0 (GXGenOutputRequest, 1);
      out_request->request = request;

      obj = setup_output_object (request);
      output_context->obj = obj;

      /* For outputting via _C(), _TD(), _H() and _CH()... */
      output_context->c_part = obj->c_funcs;
      output_context->h_typedef_part = obj->h_typedefs;
      output_context->h_part = obj->h_protos;

      namespace = setup_request_namespace (output_context->extension,
					   output_context->obj);
      output_context->namespace = namespace;

      /* the get/set_property names clash with the gobject
       * property accessor functions */
      if (strcmp (request_def->name, "GetProperty") == 0)
	gx_name = g_strdup ("GetXProperty");
      else if (strcmp (request_def->name, "SetProperty") == 0)
	gx_name = g_strdup ("SetXProperty");
      else
	gx_name = g_strdup (request_def->name);

      out_request->gx_name_cc = gx_name;
      out_request->gx_name_lc =
	gxgen_get_lowercase_name (out_request->gx_name_cc);
      out_request->gx_name_uc =
	gxgen_get_uppercase_name (out_request->gx_name_cc);

      out_request->xcb_name_cc = g_strdup (request_def->name);
      out_request->xcb_name_lc =
	gxgen_get_lowercase_name (out_request->xcb_name_cc);
      out_request->xcb_name_uc =
	gxgen_get_uppercase_name (out_request->xcb_name_cc);

      output_context->out_request = out_request;

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


      free_namespace (namespace);
      free_output_object (obj);
      g_free ((char *)out_request->xcb_name_cc);
      g_free ((char *)out_request->xcb_name_lc);
      g_free ((char *)out_request->xcb_name_uc);
      g_free ((char *)out_request->gx_name_cc);
      g_free ((char *)out_request->gx_name_lc);
      g_free ((char *)out_request->gx_name_uc);
      g_free (out_request);
    }
}

static void
output_event_typedefs (GXGenOutputContext *output_context)
{
  const XGenExtension *extension = output_context->extension;
  GList *tmp;
  guint pad = 0;

  /* For outputting via _TD()... */
  output_context->h_typedef_part = GXGEN_PART_CONNECTION_OBJ_H_TYPEDEFS;

  for (tmp = extension->events; tmp != NULL; tmp = tmp->next)
    {
      XGenEvent *event = tmp->data;
      XGenDefinition *event_def = XGEN_DEF (event);
      GList *tmp2;
      GXGenOutputNamespace *namespace;

      namespace = setup_data_type_namespace (event_def->extension);

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
      _TD ("} GX%s%sEvent;\n",
	   namespace->gx_cc, event_def->name);

      free_namespace (namespace);
    }
}

static void
output_errors (GXGenOutputContext *output_context)
{
  const XGenExtension *extension = output_context->extension;
  GList *tmp;

  /* For outputting via _H() and _C()... */
  output_context->h_part = GXGEN_PART_ERROR_CODES_H_ENUMS;
  output_context->c_part = GXGEN_PART_ERROR_DETAILS_C;

  for (tmp = extension->errors; tmp != NULL; tmp = tmp->next)
    {
      XGenDefinition *definition = tmp->data;
      XGenError *error = XGEN_ERROR_DEF (definition);

      _H ("GX_PROTOCOL_ERROR_%s,\n",
	   gxgen_get_uppercase_name (definition->name));
      _C ("{%d, \"%s\"},\n",
	   error->number,
	   gxgen_get_uppercase_name (definition->name));
    }
}

static void
output_extension_code (GXGenOutputContext *output_context)
{
  output_context->h_part = GXGEN_PART_XCB_DEPENDENCIES_H;
  _H ("#include <xcb/%s.h>\n", output_context->extension->header);

  output_enums (output_context);

  output_typedefs (output_context);

  output_structs_and_unions (output_context);

  output_requests (output_context);

  output_event_typedefs (output_context);

  output_errors (output_context);
}

static void
output_all_gx_code (XGenState * state)
{
  GXGenOutputContext *output_context = g_new0 (GXGenOutputContext, 1);
  GString *parts[GXGEN_PART_COUNT];
  int i;
  GList *tmp;

  FILE *connection_header;
  FILE *connection_code;
  FILE *drawable_header;
  FILE *drawable_code;
  FILE *pixmap_header;
  FILE *pixmap_code;
  FILE *window_header;
  FILE *window_code;
  FILE *gcontext_header;
  FILE *gcontext_code;
  FILE *cookie_header;
  FILE *error_codes_header;
  FILE *error_details_code;
  FILE *xcb_dependencies_header;
  FILE *tests_code;

  output_context->state = state;

  for (i = 0; i < GXGEN_PART_COUNT; i++)
    parts[i] = g_string_new ("");

  output_context->parts = parts;

  out (output_context,
       GXGEN_PART_XCB_DEPENDENCIES_H, "#include <xcb/xcb.h>\n");

  out (output_context,
       GXGEN_PART_COOKIE_OBJ_H_TYPEDEFS,
       "typedef enum _GXCookieType\n{\n");
  for (tmp = state->extensions; tmp != NULL; tmp = tmp->next)
    {
      output_context->extension = tmp->data;
      output_extension_code (output_context);
    }
  out (output_context,
       GXGEN_PART_COOKIE_OBJ_H_TYPEDEFS,
       "} GXCookieType;\n");

  connection_header = fopen ("gx-connection-gen.h", "w");
  if (!connection_header)
    {
      perror ("Failed to open header file");
      return;
    }
  connection_code = fopen ("gx-connection-gen.c", "w");
  if (!connection_code)
    {
      perror ("Failed to open code file");
      return;
    }

  /* #includes */
  fwrite (parts[GXGEN_PART_CONNECTION_OBJ_H_INC]->str, 1,
	  parts[GXGEN_PART_CONNECTION_OBJ_H_INC]->len, connection_header);
  /* typedefs */
  fwrite (parts[GXGEN_PART_CONNECTION_OBJ_H_TYPEDEFS]->str, 1,
	  parts[GXGEN_PART_CONNECTION_OBJ_H_TYPEDEFS]->len,
	  connection_header);
  /* macros */
  fwrite (parts[GXGEN_PART_CONNECTION_OBJ_H_MACROS]->str, 1,
	  parts[GXGEN_PART_CONNECTION_OBJ_H_MACROS]->len, connection_header);
  /* function prototypes */
  fwrite (parts[GXGEN_PART_CONNECTION_OBJ_H_PROTOS]->str, 1,
	  parts[GXGEN_PART_CONNECTION_OBJ_H_PROTOS]->len, connection_header);

  fwrite (parts[GXGEN_PART_CONNECTION_OBJ_C_INC]->str, 1,
	  parts[GXGEN_PART_CONNECTION_OBJ_C_INC]->len, connection_code);
  fwrite (parts[GXGEN_PART_CONNECTION_OBJ_C_PROTOS]->str, 1,
	  parts[GXGEN_PART_CONNECTION_OBJ_C_PROTOS]->len, connection_code);
  fwrite (parts[GXGEN_PART_CONNECTION_OBJ_C_FUNCS]->str, 1,
	  parts[GXGEN_PART_CONNECTION_OBJ_C_FUNCS]->len, connection_code);

  fclose (connection_header);
  fclose (connection_code);

  drawable_header = fopen ("gx-drawable-gen.h", "w");
  if (!drawable_header)
    {
      perror ("Failed to open header file");
      return;
    }
  drawable_code = fopen ("gx-drawable-gen.c", "w");
  if (!drawable_code)
    {
      perror ("Failed to open code file");
      return;
    }

  /* #includes */
  fwrite (parts[GXGEN_PART_DRAWABLE_OBJ_H_INC]->str, 1,
	  parts[GXGEN_PART_DRAWABLE_OBJ_H_INC]->len, drawable_header);
  /* typedefs */
  fwrite (parts[GXGEN_PART_DRAWABLE_OBJ_H_TYPEDEFS]->str, 1,
	  parts[GXGEN_PART_DRAWABLE_OBJ_H_TYPEDEFS]->len, drawable_header);
  /* macros */
  fwrite (parts[GXGEN_PART_DRAWABLE_OBJ_H_MACROS]->str, 1,
	  parts[GXGEN_PART_DRAWABLE_OBJ_H_MACROS]->len, drawable_header);
  /* function prototypes */
  fwrite (parts[GXGEN_PART_DRAWABLE_OBJ_H_PROTOS]->str, 1,
	  parts[GXGEN_PART_DRAWABLE_OBJ_H_PROTOS]->len, drawable_header);

  fwrite (parts[GXGEN_PART_DRAWABLE_OBJ_C_INC]->str, 1,
	  parts[GXGEN_PART_DRAWABLE_OBJ_C_INC]->len, drawable_code);
  fwrite (parts[GXGEN_PART_DRAWABLE_OBJ_C_PROTOS]->str, 1,
	  parts[GXGEN_PART_DRAWABLE_OBJ_C_PROTOS]->len, drawable_code);
  fwrite (parts[GXGEN_PART_DRAWABLE_OBJ_C_FUNCS]->str, 1,
	  parts[GXGEN_PART_DRAWABLE_OBJ_C_FUNCS]->len, drawable_code);

  fclose (drawable_header);
  fclose (drawable_code);



  pixmap_header = fopen ("gx-pixmap-gen.h", "w");
  if (!pixmap_header)
    {
      perror ("Failed to open header file");
      return;
    }
  pixmap_code = fopen ("gx-pixmap-gen.c", "w");
  if (!pixmap_code)
    {
      perror ("Failed to open code file");
      return;
    }

  /* #includes */
  fwrite (parts[GXGEN_PART_PIXMAP_OBJ_H_INC]->str, 1,
	  parts[GXGEN_PART_PIXMAP_OBJ_H_INC]->len, pixmap_header);
  /* typedefs */
  fwrite (parts[GXGEN_PART_PIXMAP_OBJ_H_TYPEDEFS]->str, 1,
	  parts[GXGEN_PART_PIXMAP_OBJ_H_TYPEDEFS]->len, pixmap_header);
  /* macros */
  fwrite (parts[GXGEN_PART_PIXMAP_OBJ_H_MACROS]->str, 1,
	  parts[GXGEN_PART_PIXMAP_OBJ_H_MACROS]->len, pixmap_header);
  /* function prototypes */
  fwrite (parts[GXGEN_PART_PIXMAP_OBJ_H_PROTOS]->str, 1,
	  parts[GXGEN_PART_PIXMAP_OBJ_H_PROTOS]->len, pixmap_header);

  fwrite (parts[GXGEN_PART_PIXMAP_OBJ_C_INC]->str, 1,
	  parts[GXGEN_PART_PIXMAP_OBJ_C_INC]->len, pixmap_code);
  fwrite (parts[GXGEN_PART_PIXMAP_OBJ_C_PROTOS]->str, 1,
	  parts[GXGEN_PART_PIXMAP_OBJ_C_PROTOS]->len, pixmap_code);
  fwrite (parts[GXGEN_PART_PIXMAP_OBJ_C_FUNCS]->str, 1,
	  parts[GXGEN_PART_PIXMAP_OBJ_C_FUNCS]->len, pixmap_code);

  fclose (pixmap_header);
  fclose (pixmap_code);


  window_header = fopen ("gx-window-gen.h", "w");
  if (!window_header)
    {
      perror ("Failed to open header file");
      return;
    }
  window_code = fopen ("gx-window-gen.c", "w");
  if (!window_code)
    {
      perror ("Failed to open code file");
      return;
    }

  /* #includes */
  fwrite (parts[GXGEN_PART_WINDOW_OBJ_H_INC]->str, 1,
	  parts[GXGEN_PART_WINDOW_OBJ_H_INC]->len, window_header);
  /* typedefs */
  fwrite (parts[GXGEN_PART_WINDOW_OBJ_H_TYPEDEFS]->str, 1,
	  parts[GXGEN_PART_WINDOW_OBJ_H_TYPEDEFS]->len, window_header);
  /* macros */
  fwrite (parts[GXGEN_PART_WINDOW_OBJ_H_MACROS]->str, 1,
	  parts[GXGEN_PART_WINDOW_OBJ_H_MACROS]->len, window_header);
  /* function prototypes */
  fwrite (parts[GXGEN_PART_WINDOW_OBJ_H_PROTOS]->str, 1,
	  parts[GXGEN_PART_WINDOW_OBJ_H_PROTOS]->len, window_header);

  fwrite (parts[GXGEN_PART_WINDOW_OBJ_C_INC]->str, 1,
	  parts[GXGEN_PART_WINDOW_OBJ_C_INC]->len, window_code);
  fwrite (parts[GXGEN_PART_WINDOW_OBJ_C_PROTOS]->str, 1,
	  parts[GXGEN_PART_WINDOW_OBJ_C_PROTOS]->len, window_code);
  fwrite (parts[GXGEN_PART_WINDOW_OBJ_C_FUNCS]->str, 1,
	  parts[GXGEN_PART_WINDOW_OBJ_C_FUNCS]->len, window_code);

  fclose (window_header);
  fclose (window_code);



  gcontext_header = fopen ("gx-gcontext-gen.h", "w");
  if (!gcontext_header)
    {
      perror ("Failed to open header file");
      return;
    }
  gcontext_code = fopen ("gx-gcontext-gen.c", "w");
  if (!gcontext_code)
    {
      perror ("Failed to open code file");
      return;
    }

  /* #includes */
  fwrite (parts[GXGEN_PART_GCONTEXT_OBJ_H_INC]->str, 1,
	  parts[GXGEN_PART_GCONTEXT_OBJ_H_INC]->len, gcontext_header);
  /* typedefs */
  fwrite (parts[GXGEN_PART_GCONTEXT_OBJ_H_TYPEDEFS]->str, 1,
	  parts[GXGEN_PART_GCONTEXT_OBJ_H_TYPEDEFS]->len, gcontext_header);
  /* macros */
  fwrite (parts[GXGEN_PART_GCONTEXT_OBJ_H_MACROS]->str, 1,
	  parts[GXGEN_PART_GCONTEXT_OBJ_H_MACROS]->len, gcontext_header);
  /* function prototypes */
  fwrite (parts[GXGEN_PART_GCONTEXT_OBJ_H_PROTOS]->str, 1,
	  parts[GXGEN_PART_GCONTEXT_OBJ_H_PROTOS]->len, gcontext_header);

  fwrite (parts[GXGEN_PART_GCONTEXT_OBJ_C_INC]->str, 1,
	  parts[GXGEN_PART_GCONTEXT_OBJ_C_INC]->len, gcontext_code);
  fwrite (parts[GXGEN_PART_GCONTEXT_OBJ_C_PROTOS]->str, 1,
	  parts[GXGEN_PART_GCONTEXT_OBJ_C_PROTOS]->len, gcontext_code);
  fwrite (parts[GXGEN_PART_GCONTEXT_OBJ_C_FUNCS]->str, 1,
	  parts[GXGEN_PART_GCONTEXT_OBJ_C_FUNCS]->len, gcontext_code);

  fclose (gcontext_header);
  fclose (gcontext_code);


  cookie_header = fopen ("gx-cookie-gen.h", "w");
  if (!cookie_header)
    {
      perror ("Failed to open header file");
      return;
    }

  /* typedefs */
  fwrite (parts[GXGEN_PART_COOKIE_OBJ_H_TYPEDEFS]->str, 1,
	  parts[GXGEN_PART_COOKIE_OBJ_H_TYPEDEFS]->len, cookie_header);

  fclose (cookie_header);

  error_codes_header = fopen ("gx-protocol-error-codes-gen.h", "w");
  if (!error_codes_header)
    {
      perror ("Failed to open header file");
      return;
    }

  /* error codes enum */
  fwrite (parts[GXGEN_PART_ERROR_CODES_H_ENUMS]->str, 1,
	  parts[GXGEN_PART_ERROR_CODES_H_ENUMS]->len, error_codes_header);

  fclose (error_codes_header);

  error_details_code = fopen ("gx-protocol-error-details-gen.h", "w");
  if (!error_details_code)
    {
      perror ("Failed to open header file");
      return;
    }

  /* error details */
  fwrite (parts[GXGEN_PART_ERROR_DETAILS_C]->str, 1,
	  parts[GXGEN_PART_ERROR_DETAILS_C]->len, error_details_code);

  fclose (error_details_code);

  xcb_dependencies_header = fopen ("gx-xcb-dependencies-gen.h", "w");
  if (!xcb_dependencies_header)
    {
      perror ("Failed to open header file");
      return;
    }

  /* xcb dependencies */
  fwrite (parts[GXGEN_PART_XCB_DEPENDENCIES_H]->str, 1,
	  parts[GXGEN_PART_XCB_DEPENDENCIES_H]->len, xcb_dependencies_header);

  fclose (xcb_dependencies_header);

  tests_code = fopen ("gx-tests-gen.c", "w");
  if (!tests_code)
    {
      perror ("Failed to open header file");
      return;
    }

  /* Various auto generated tests */
  fwrite (parts[GXGEN_PART_TESTS_C]->str, 1,
	  parts[GXGEN_PART_TESTS_C]->len, tests_code);

  fclose (tests_code);


  for (i = 0; i < GXGEN_PART_COUNT; i++)
    g_string_free (parts[i], TRUE);
}

int
main (int argc, char **argv)
{
  int i;
  GList *files = NULL;
  XGenState *state;

  /* The regex used for splitting CamelCase names.
   * NB: this is taken from xcb so we get the same splitting */
  cname_regex =
    g_regex_new ("([A-Z0-9][a-z]+|[A-Z0-9]+(?![a-z])|[a-z]+)", 0, 0, NULL);

  for (i = 1; i < argc && argv[i]; i++)
    files = g_list_prepend (files, g_strdup (argv[i]));

  state = xgen_parse_xcb_proto_files (files);
  if (!state)
    g_error ("Failed to parse XCB proto files\n");

  g_list_foreach (files, (GFunc)g_free, NULL);
  g_list_free (files);

  output_all_gx_code (state);

  return 0;
}

