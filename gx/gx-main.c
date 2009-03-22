#include <gx/gx-main.h>
#include <gx/gx-event.h>

#include <glib.h>
#include <glib-object.h>

extern GXEventDetails _gx_xproto_event_details[];

static GMainLoop *loop;

/**
 * gx_init:
 * @argc: Address of the argc parameter of your main() function. Changed
 * if any arguments were handled.
 * @argv: Address of the argv parameter of main(). Any parameters understood
 * by gx_init() are stripped before return.
 *
 * Call this function before any other GX functions. It will initialise
 * everything need for the GX API to operate and parses some standard
 * command line options. argc and argv are adjusted accordingly so your
 * own code will never see those standard arguments.
 */
void
gx_init (int *argc, char ***argv)
{
  g_type_init ();

  /* TODO: parse standard arguments */

  g_atexit (_gx_event_details_hash_free);

  gx_event_details_add_extension (_gx_xproto_event_details);
}

void
gx_main (void)
{

#if 0
  if (!gx_is_initialized)
    {
      g_warning ("Called gx_main() but GX wasn't initialised.  "
		 "You must call gx_init() first.");
      return;
    }
#endif

  loop = g_main_loop_new (NULL, TRUE);

  g_main_loop_run (loop);

}

void
gx_main_quit (void)
{
  g_main_loop_quit (loop);
}

