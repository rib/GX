lib_LTLIBRARIES = libgx-@GX_MAJOR_VERSION@.@GX_MINOR_VERSION@.la
#noinst_PROGRAMS = gxgen-tests

EXTENSION_XML = xproto.xml
# FIXME
#EXTENSION_XML += bigreq.xml
if BUILD_SHAPE
EXTENSION_XML += shape.xml
endif
if BUILD_RENDER
EXTENSION_XML += render.xml
endif
if BUILD_XFIXES
# depends on shape + render
EXTENSION_XML += xfixes.xml
endif
# depends on fixes
if BUILD_COMPOSITE
EXTENSION_XML += composite.xml
endif
if BUILD_DAMAGE
EXTENSION_XML += damage.xml
endif
if BUILD_DPMS
EXTENSION_XML += dpms.xml
endif
##EXTENSION_XML += glx.xml
if BUILD_RANDR
EXTENSION_XML += randr.xml
endif
if BUILD_RECORD
EXTENSION_XML += record.xml
endif
if BUILD_RES
EXTENSION_XML += res.xml
endif
if BUILD_SCREENSAVER
EXTENSION_XML += screensaver.xml
endif
if BUILD_SHM
EXTENSION_XML += shm.xml
endif
if BUILD_SYNC
EXTENSION_XML += sync.xml
endif
#EXTENSION_XML += xc_misc.xml
if BUILD_XEVIE
EXTENSION_XML += xevie.xml
endif
#EXTENSION_XML += xf86dri.xml
#EXTENSION_XML += xinerama.xml
#EXTENSION_XML += xprint.xml
#EXTENSION_XML += xtest.xml
#EXTENSION_XML += xv.xml
#Depends on xv
#EXTENSION_XML += xvmc.xml

# Generate this list via:
# cd gx/generated-code && \
# find ./ -maxdepth 1 -iname 'gx-*-gen.*' |cut -d'/' -f2|xargs printf '\t$(GEN_DIR)/%s \\\n'
GEN_DIR=generated-code
GENERATED_CODE = \
	$(GEN_DIR)/gx-window-screensaver-gen.c \
	$(GEN_DIR)/gx-window-xproto-gen.h \
	$(GEN_DIR)/gx-gcontext-shm-gen.c \
	$(GEN_DIR)/gx-shape-main-gen.c \
	$(GEN_DIR)/gx-connection-xevie-gen.h \
	$(GEN_DIR)/gx-window-dpms-gen.h \
	$(GEN_DIR)/gx-connection-sync-gen.c \
	$(GEN_DIR)/gx-drawable-composite-gen.h \
	$(GEN_DIR)/gx-shm-event-details-gen.c \
	$(GEN_DIR)/gx-connection-dpms-gen.c \
	$(GEN_DIR)/gx-connection-screensaver-gen.h \
	$(GEN_DIR)/gx-window-sync-gen.h \
	$(GEN_DIR)/gx-gcontext-sync-gen.h \
	$(GEN_DIR)/gx-pixmap-render-gen.h \
	$(GEN_DIR)/gx-connection-xevie-gen.c \
	$(GEN_DIR)/gx-window-shm-gen.c \
	$(GEN_DIR)/gx-drawable-dpms-gen.h \
	$(GEN_DIR)/gx-connection-res-gen.h \
	$(GEN_DIR)/gx-drawable-record-gen.c \
	$(GEN_DIR)/gx-xproto-event-codes-gen.h \
	$(GEN_DIR)/gx-pixmap-res-gen.c \
	$(GEN_DIR)/gx-gcontext-randr-gen.c \
	$(GEN_DIR)/gx-shape-event-details-gen.c \
	$(GEN_DIR)/gx-connection-composite-gen.c \
	$(GEN_DIR)/gx-gcontext-composite-gen.h \
	$(GEN_DIR)/gx-shm-protocol-error-codes-gen.h \
	$(GEN_DIR)/gx-pixmap-composite-gen.c \
	$(GEN_DIR)/gx-window-xproto-gen.c \
	$(GEN_DIR)/gx-gcontext-screensaver-gen.c \
	$(GEN_DIR)/gx-window-record-gen.c \
	$(GEN_DIR)/gx-connection-shape-gen.h \
	$(GEN_DIR)/gx-connection-shm-gen.c \
	$(GEN_DIR)/gx-window-shape-gen.h \
	$(GEN_DIR)/gx-gcontext-shape-gen.c \
	$(GEN_DIR)/gx-window-xevie-gen.c \
	$(GEN_DIR)/gx-drawable-damage-gen.h \
	$(GEN_DIR)/gx-xcb-dependencies-gen.h \
	$(GEN_DIR)/gx-drawable-res-gen.c \
	$(GEN_DIR)/gx-damage-event-details-gen.c \
	$(GEN_DIR)/gx-connection-record-gen.c \
	$(GEN_DIR)/gx-drawable-res-gen.h \
	$(GEN_DIR)/gx-window-render-gen.c \
	$(GEN_DIR)/gx-pixmap-composite-gen.h \
	$(GEN_DIR)/gx-connection-screensaver-gen.c \
	$(GEN_DIR)/gx-connection-xfixes-gen.h \
	$(GEN_DIR)/gx-render-protocol-error-codes-gen.h \
	$(GEN_DIR)/gx-gcontext-shm-gen.h \
	$(GEN_DIR)/gx-window-dpms-gen.c \
	$(GEN_DIR)/gx-dpms-main-gen.c \
	$(GEN_DIR)/gx-pixmap-screensaver-gen.c \
	$(GEN_DIR)/gx-connection-damage-gen.c \
	$(GEN_DIR)/gx-composite-main-gen.c \
	$(GEN_DIR)/gx-xfixes-main-gen.c \
	$(GEN_DIR)/gx-pixmap-screensaver-gen.h \
	$(GEN_DIR)/gx-randr-main-gen.c \
	$(GEN_DIR)/gx-connection-shape-gen.c \
	$(GEN_DIR)/gx-record-main-gen.c \
	$(GEN_DIR)/gx-drawable-dpms-gen.c \
	$(GEN_DIR)/gx-drawable-composite-gen.c \
	$(GEN_DIR)/gx-gcontext-dpms-gen.c \
	$(GEN_DIR)/gx-gcontext-res-gen.c \
	$(GEN_DIR)/gx-pixmap-randr-gen.h \
	$(GEN_DIR)/gx-drawable-screensaver-gen.h \
	$(GEN_DIR)/gx-connection-damage-gen.h \
	$(GEN_DIR)/gx-pixmap-xproto-gen.h \
	$(GEN_DIR)/gx-window-res-gen.h \
	$(GEN_DIR)/gx-pixmap-damage-gen.c \
	$(GEN_DIR)/gx-window-damage-gen.h \
	$(GEN_DIR)/gx-window-render-gen.h \
	$(GEN_DIR)/gx-drawable-xfixes-gen.c \
	$(GEN_DIR)/gx-gcontext-randr-gen.h \
	$(GEN_DIR)/gx-xproto-event-details-gen.c \
	$(GEN_DIR)/gx-render-protocol-error-details-gen.c \
	$(GEN_DIR)/gx-drawable-randr-gen.h \
	$(GEN_DIR)/gx-drawable-render-gen.c \
	$(GEN_DIR)/gx-drawable-randr-gen.c \
	$(GEN_DIR)/gx-drawable-record-gen.h \
	$(GEN_DIR)/gx-randr-event-codes-gen.h \
	$(GEN_DIR)/gx-shm-main-gen.c \
	$(GEN_DIR)/gx-sync-event-codes-gen.h \
	$(GEN_DIR)/gx-pixmap-res-gen.h \
	$(GEN_DIR)/gx-window-sync-gen.c \
	$(GEN_DIR)/gx-screensaver-event-details-gen.c \
	$(GEN_DIR)/gx-damage-protocol-error-codes-gen.h \
	$(GEN_DIR)/gx-pixmap-shape-gen.h \
	$(GEN_DIR)/gx-pixmap-shm-gen.c \
	$(GEN_DIR)/gx-gcontext-damage-gen.c \
	$(GEN_DIR)/gx-pixmap-randr-gen.c \
	$(GEN_DIR)/gx-window-damage-gen.c \
	$(GEN_DIR)/gx-gcontext-record-gen.h \
	$(GEN_DIR)/gx-xfixes-protocol-error-codes-gen.h \
	$(GEN_DIR)/gx-res-main-gen.c \
	$(GEN_DIR)/gx-pixmap-xevie-gen.h \
	$(GEN_DIR)/gx-window-res-gen.c \
	$(GEN_DIR)/gx-shm-protocol-error-details-gen.c \
	$(GEN_DIR)/gx-connection-render-gen.h \
	$(GEN_DIR)/gx-sync-main-gen.c \
	$(GEN_DIR)/gx-pixmap-record-gen.c \
	$(GEN_DIR)/gx-sync-protocol-error-details-gen.c \
	$(GEN_DIR)/gx-gcontext-screensaver-gen.h \
	$(GEN_DIR)/gx-gcontext-xproto-gen.c \
	$(GEN_DIR)/gx-pixmap-xevie-gen.c \
	$(GEN_DIR)/gx-drawable-xproto-gen.c \
	$(GEN_DIR)/gx-gcontext-render-gen.c \
	$(GEN_DIR)/gx-drawable-screensaver-gen.c \
	$(GEN_DIR)/gx-gcontext-damage-gen.h \
	$(GEN_DIR)/gx-drawable-shm-gen.c \
	$(GEN_DIR)/gx-gcontext-xevie-gen.h \
	$(GEN_DIR)/gx-connection-res-gen.c \
	$(GEN_DIR)/gx-pixmap-damage-gen.h \
	$(GEN_DIR)/gx-gcontext-render-gen.h \
	$(GEN_DIR)/gx-sync-protocol-error-codes-gen.h \
	$(GEN_DIR)/gx-gcontext-xproto-gen.h \
	$(GEN_DIR)/gx-drawable-shm-gen.h \
	$(GEN_DIR)/gx-window-randr-gen.c \
	$(GEN_DIR)/gx-connection-sync-gen.h \
	$(GEN_DIR)/gx-shm-event-codes-gen.h \
	$(GEN_DIR)/gx-record-protocol-error-codes-gen.h \
	$(GEN_DIR)/gx-xevie-main-gen.c \
	$(GEN_DIR)/gx-window-xfixes-gen.h \
	$(GEN_DIR)/gx-gcontext-composite-gen.c \
	$(GEN_DIR)/gx-connection-xfixes-gen.c \
	$(GEN_DIR)/gx-render-main-gen.c \
	$(GEN_DIR)/gx-drawable-xfixes-gen.h \
	$(GEN_DIR)/gx-xfixes-event-codes-gen.h \
	$(GEN_DIR)/gx-drawable-render-gen.h \
	$(GEN_DIR)/gx-gcontext-xevie-gen.c \
	$(GEN_DIR)/gx-screensaver-event-codes-gen.h \
	$(GEN_DIR)/gx-xfixes-event-details-gen.c \
	$(GEN_DIR)/gx-connection-dpms-gen.h \
	$(GEN_DIR)/gx-window-record-gen.h \
	$(GEN_DIR)/gx-drawable-xevie-gen.h \
	$(GEN_DIR)/gx-gcontext-xfixes-gen.c \
	$(GEN_DIR)/gx-drawable-sync-gen.c \
	$(GEN_DIR)/gx-connection-composite-gen.h \
	$(GEN_DIR)/gx-gcontext-record-gen.c \
	$(GEN_DIR)/gx-drawable-shape-gen.c \
	$(GEN_DIR)/gx-window-xfixes-gen.c \
	$(GEN_DIR)/gx-connection-xproto-gen.c \
	$(GEN_DIR)/gx-window-xevie-gen.h \
	$(GEN_DIR)/gx-pixmap-dpms-gen.h \
	$(GEN_DIR)/gx-randr-protocol-error-details-gen.c \
	$(GEN_DIR)/gx-connection-shm-gen.h \
	$(GEN_DIR)/gx-connection-render-gen.c \
	$(GEN_DIR)/gx-damage-protocol-error-details-gen.c \
	$(GEN_DIR)/gx-damage-event-codes-gen.h \
	$(GEN_DIR)/gx-drawable-sync-gen.h \
	$(GEN_DIR)/gx-window-randr-gen.h \
	$(GEN_DIR)/gx-window-composite-gen.c \
	$(GEN_DIR)/gx-xfixes-protocol-error-details-gen.c \
	$(GEN_DIR)/gx-gcontext-dpms-gen.h \
	$(GEN_DIR)/gx-connection-randr-gen.c \
	$(GEN_DIR)/gx-gcontext-xfixes-gen.h \
	$(GEN_DIR)/gx-pixmap-shape-gen.c \
	$(GEN_DIR)/gx-connection-xproto-gen.h \
	$(GEN_DIR)/gx-drawable-damage-gen.c \
	$(GEN_DIR)/gx-pixmap-record-gen.h \
	$(GEN_DIR)/gx-window-shm-gen.h \
	$(GEN_DIR)/gx-pixmap-shm-gen.h \
	$(GEN_DIR)/gx-pixmap-xfixes-gen.c \
	$(GEN_DIR)/gx-drawable-xevie-gen.c \
	$(GEN_DIR)/gx-shape-event-codes-gen.h \
	$(GEN_DIR)/gx-record-protocol-error-details-gen.c \
	$(GEN_DIR)/gx-xproto-protocol-error-details-gen.c \
	$(GEN_DIR)/gx-gcontext-res-gen.h \
	$(GEN_DIR)/gx-pixmap-render-gen.c \
	$(GEN_DIR)/gx-randr-event-details-gen.c \
	$(GEN_DIR)/gx-randr-protocol-error-codes-gen.h \
	$(GEN_DIR)/gx-pixmap-sync-gen.h \
	$(GEN_DIR)/gx-window-composite-gen.h \
	$(GEN_DIR)/gx-drawable-shape-gen.h \
	$(GEN_DIR)/gx-cookie-gen.h \
	$(GEN_DIR)/gx-xproto-protocol-error-codes-gen.h \
	$(GEN_DIR)/gx-pixmap-dpms-gen.c \
	$(GEN_DIR)/gx-pixmap-xfixes-gen.h \
	$(GEN_DIR)/gx-window-shape-gen.c \
	$(GEN_DIR)/gx-connection-randr-gen.h \
	$(GEN_DIR)/gx-window-screensaver-gen.h \
	$(GEN_DIR)/gx-connection-record-gen.h \
	$(GEN_DIR)/gx-sync-event-details-gen.c \
	$(GEN_DIR)/gx-damage-main-gen.c \
	$(GEN_DIR)/gx-gcontext-shape-gen.h \
	$(GEN_DIR)/gx-screensaver-main-gen.c \
	$(GEN_DIR)/gx-pixmap-xproto-gen.c \
	$(GEN_DIR)/gx-gcontext-sync-gen.c \
	$(GEN_DIR)/gx-drawable-xproto-gen.h \
	$(GEN_DIR)/gx-pixmap-sync-gen.c

$(GENERATED_CODE): $(top_builddir)/tools/gx-gen
	echo "generating $@"
	-mkdir -p $(GEN_DIR)/extensions
	$(top_builddir)/tools/gx-gen $(EXTENSION_XML)
	indent generated-code/*{c,h}
	-rm generated-code/*~
BUILT_SOURCES = $(GENERATED_CODE)

#.PHONY: generated-code
#BUILT_SOURCES=generated-code
##BUILT_SOURCES=gx-connection-gen.c
##gx-connection-gen.c: $(top_builddir)/tools/gx-gen
#generated-code: $(EXTENSION_XML)
#	$(top_builddir)/tools/gx-gen $(EXTENSION_XML)
##	$(top_srcdir)/tools/post_process.sh

#libgx_@GX_MAJOR_VERSION@_@GX_MINOR_VERSION@_la_SOURCES = \
#gx-connection.c gx-drawable.c gx-pixmap.c gx-window.c
libgx_@GX_MAJOR_VERSION@_@GX_MINOR_VERSION@_la_SOURCES = \
	gx-main.c \
	gx-mask-value-item.c \
	gx-connection.c \
	gx-connection.h \
	gx-drawable.c \
	gx-drawable.h \
	gx-window.c \
	gx-window.h \
	gx-pixmap.c \
	gx-pixmap.h \
	gx-gcontext.c \
	gx-gcontext.h \
	gx-screen.c \
	gx-screen.h \
	gx-cookie.c \
	gx-cookie.h \
	gx-protocol-error.c \
	gx-protocol-error.h \
	gx-event.c \
	gx-event.h \
	$(GEN_DIR)/gx-window-xproto-gen.h \
        $(GEN_DIR)/gx-window-xproto-gen.c \
        $(GEN_DIR)/gx-pixmap-xproto-gen.h \
        $(GEN_DIR)/gx-gcontext-xproto-gen.c \
        $(GEN_DIR)/gx-drawable-xproto-gen.c \
        $(GEN_DIR)/gx-gcontext-xproto-gen.h \
        $(GEN_DIR)/gx-connection-xproto-gen.c \
        $(GEN_DIR)/gx-connection-xproto-gen.h \
        $(GEN_DIR)/gx-pixmap-xproto-gen.c \
        $(GEN_DIR)/gx-drawable-xproto-gen.h \
	$(GEN_DIR)/gx-xproto-event-details-gen.c
#Note: The above list of xproto files can be got using:
# find ./ -iname 'gx-*-xproto*' |cut -d'/' -f2|xargs printf '\t$(GEN_DIR)/%s \\\n'

#libgx_@GX_MAJOR_VERSION@_@GX_MINOR_VERSION@_la_LDADD =
libgx_@GX_MAJOR_VERSION@_@GX_MINOR_VERSION@_la_LDFLAGS = \
	@GX_DEP_LIBS@ \
  	-version-info $(GX_LT_CURRENT):$(GX_LT_REVISION):$(GX_LT_AGE) \
	-export-dynamic \
	-export-symbols-regex "^gx_.*"
libgx_@GX_MAJOR_VERSION@_@GX_MINOR_VERSION@_la_CFLAGS = \
	@EXTRA_CFLAGS@ @GX_DEP_CFLAGS@
libgx_@GX_MAJOR_VERSION@_@GX_MINOR_VERSION@_la_CPPFLAGS = \
	@EXTRA_CPPFLAGS@ \
	-I$(top_srcdir)/ \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src

#gxgen_tests_SOURCES = generated-code/gxgen-tests-gen.c
#gxgen_tests_CFLAGS = @EXTRA_CFLAGS@ @GX_DEP_CFLAGS@
#gxgen_tests_CPPFLAGS = @EXTRA_CPPFLAGS@
#gxgen_tests_LDADD = @GX_DEP_LIBS@

#EXTRA_DIST =

#CLEANFILES =

#DISTCLEANFILES =

gxincludedir = \
	$(includedir)/gx-$(GX_MAJOR_VERSION).$(GX_MINOR_VERSION)
gxinternalincludedir = \
	$(includedir)/gx-$(GX_MAJOR_VERSION).$(GX_MINOR_VERSION)/gx
gxinternalgenincludedir = \
	$(includedir)/gx-$(GX_MAJOR_VERSION).$(GX_MINOR_VERSION)/gx/generated-code

gxinclude_HEADERS = gx.h
gxinternalinclude_HEADERS = \
	gx-event.h \
	gx-drawable.h \
	gx-pixmap.h \
	gx-main.h \
	gx-cookie.h \
	gx-screen.h \
	gx-protocol-error.h \
	gx-mask-value-item.h \
	gx-types.h \
	gx-gcontext.h \
	gx-window.h \
	gx-connection.h
gxinternalgeninclude_HEADERS = \
	$(GEN_DIR)/gx-window-xproto-gen.h \
        $(GEN_DIR)/gx-pixmap-xproto-gen.h \
        $(GEN_DIR)/gx-gcontext-xproto-gen.h \
        $(GEN_DIR)/gx-connection-xproto-gen.h \
        $(GEN_DIR)/gx-drawable-xproto-gen.h

