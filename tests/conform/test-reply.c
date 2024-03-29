
#include <gx.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "test-gx-common.h"

void
test_reply (TestGXSimpleFixture *fixture,
            gconstpointer data)
{
  GXConnection *connection;
  GXWindow *root;
  GXWindowQueryTreeReply *query_tree;
  GList *children;
  GList *tmp;
  GXWindow *child;

  g_type_init ();

  connection = gx_connection_new (NULL);
  if (gx_connection_has_error (connection))
    {
      g_printerr ("Error establishing connection to X server");
      exit (1);
    }

  root = gx_connection_get_default_root (connection);

  query_tree = gx_window_query_tree (root, NULL);

  children = gx_window_query_tree_get_children (query_tree);
  for (tmp = children; tmp != NULL; tmp = tmp->next)
    {
      child = tmp->data;
      g_print ("child of root (0x%08x)\n",
	       gx_drawable_get_xid (GX_DRAWABLE (child)));
    }
  gx_window_query_tree_free_children (children);

  gx_window_query_tree_reply_free (query_tree);

  /* FIXME - this test currently just tests things dont crash when issuing a
   * request and waiting for a reply. This test should actually verify the
   * value of some reply is correct */

  g_object_unref (root);
  g_object_unref (connection);

  return;
}

