
#include <gx.h>

#include <stdio.h>
#include <unistd.h>

/* This is just to try and catch a silly mistake with the internal ref counting
 * of cookies, since it's expected that a lot of cookies will be created and
 * destroyed over the lifetime of a program.
 *
 * This test does two asynchronous query window requests, and for one cookie it
 * immediatly demands the reply by calling gx_window_query_reply, and for the
 * other a signal handler is installed for the reply. Once both replies have
 * been recieved and we expect that those cookie should have internally been
 * unrefed we quite the main loop and check that they have been finalized.
 *
 * Finally just before we unref the connection we issue another request and
 * after unrefing the connection that last cookie should also have been
 * finalized.
 */

gboolean cookie0_finalized = FALSE;
gboolean cookie1_finalized = FALSE;

gboolean cookie2_finalized = FALSE;

static int
check_cookies_0_and_1_status (void)
{
  int error = 0;

  if (!cookie0_finalized)
    {
      g_print ("cookie0 was not finalized!\n");
      error = 1;
    }
  if (!cookie1_finalized)
    {
      g_print ("cookie1 was not finalized!\n");
      error = 1;
    }
  return error;
}

static int
check_cookies_2_status (void)
{
  if (!cookie2_finalized)
    {
      g_print ("cookie2 was not finalized!\n");
      return 1;
    }
  return 0;
}

static void
cookie_finalize_notify (gpointer data,
                        GObject *where_the_object_was)
{
  gboolean *cookie_finalized_status = data;
  *cookie_finalized_status = TRUE;
}

static void
query_tree_reply_handler (GXCookie *self, gpointer user_data)
{
  GXQueryTreeReply *query_tree;

  query_tree = gx_window_query_reply (self);
  /* SNIP processing the reply */
  gx_window_query_tree_reply_free (query_tree);

  /*
   * At this point we expect that both cookies should have been finalized
   */
  gx_main_quit ();
}

int
main(int argc, char **argv)
{
  GXConnection *connection;
  GXWindow *root;
  GXCookie *cookie0;
  GXCookie *cookie1;
  GXCookie *cookie2;
  GXQueryTreeReply *query_tree;
  GArray *array;
  GXWindow **children;
  GXWindow *child;
  int i;
  int error = 0;

  g_type_init ();

  connection = gx_connection_new (NULL);
  if (gx_connection_has_error (connection))
    {
      g_printerr ("Error establishing connection to X server");
      return 1;
    }

  root = gx_connection_get_root_window (connection);


  cookie0 = gx_window_query_tree_async (root);
  g_object_weak_ref (cookie0, cookie_finalize_notify, &cookie0_finalized);
  /* You could do work here to hide the request latency. */
  query_tree = gx_window_query_reply (cookie0);
  /* SNIP processing the reply */
  gx_window_query_tree_reply_free (query_tree);


  cookie1 = gx_window_query_tree_async (root);
  g_object_weak_ref (cookie1, cookie_finalize_notify, &cookie1_finalized);
  g_signal_connect (cookie1,
		    "reply",
		    query_tree_reply_handler,
		    NULL);

  gx_connection_flush (connection, FALSE);

  gx_main();

  error |= check_cookies_0_and_1_status ();

  /* Check that when the connection is unrefed then all internal references
   * to cookies are unrefed */
  cookie2 = gx_window_query_tree_async (root);
  g_object_weak_ref (cookie2, cookie_finalize_notify, &cookie2_finalized);

  g_object_unref (root);
  g_object_unref (connection);

  error |= check_cookies_2_status ();

  if (error)
    return EXIT_FAILURE;
  else
    return EXIT_SUCCESS;
}

