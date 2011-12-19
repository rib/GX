
NB: gotchas:


X Properties
------------

The gx_window_get/set_property functions are awkward since gobjects use such
names to set object properties, but in X11 terms they would send and retrieve
X Window properties to/from the xserver.

Currently gx_window_get/set_property correspond to the gobject varient and we
have gx_window_get/set_xproperty for the X11 kind.

The xproto defines an enum that should be named GXWindowClass, but that
conflicts with the the class definition for GXWindow objects.


GXWindowClass
-------------
Currently the gobject variant has been renamed _GXWindowClass, but given that
the GXWindowClass enum will be used quite a bit, I wonder if that might be a
bit confusing?

GXPixmap
--------
The xproto defines an enum that should be named GXPixmap, but that conflicts with the class definition to GXPixmap objects. I changed gxgen to add a Type suffix to all enums; though I'm not sure this is the best solution.

Errors
------
The recomended way to handle errors is by passing in a GError pointer to the
request functions, or if you are using the *_async requests, then pass the
pointer to the corresponding *_reply call.

TODO:
To support Xlib event handling semantics though, you can instead pass NULL for
the GError pointers and connect to the error signal on the GXConnection object.

If you are using GErrors, then is also worth being aware that if you are using
the synchronous request APIs and in particular use a request that doesn't have a
corresponding reply, then passing a non NULL GError means that function will
block until it knows if the request generates an error. (Similar to those
requests that do have a reply, where it also has to block) If you don't care
about the error then by passing NULL, the function can return immediatly after
sending the request. If you don't want the blocking but do care about the error
you should be using the *_async API.


Polling for Events, Replys and Errors
-------------------------------------
There is a conflict between xcb_poll_for_reply and xcb_blah_reply, since if the
former is used to retrieve a reply from XCB, then the later wont also retrieve it
(not actually verified, but seems most likley) therefore we need to wrap how
we query XCB so that we have an oppertunity to push things into a queue if needs be.

This also raises the question of how we determine that we can free an event/reply
structure since we don't know if the user will use a gx_blah_reply call or use a signal handler.

If we could force the gx_blah_reply funcs to synchronously fire off any necissary signals then we'd know that the event is finished with, but there is still a problem if we call xcb_poll_for_reply first

