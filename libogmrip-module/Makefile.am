lib_LTLIBRARIES = \
  libogmrip-module.la

libogmrip_module_la_SOURCES = \
  ogmrip-module-object.c \
  ogmrip-module-engine.c

libogmrip_module_la_HEADERS = \
  ogmrip-module-object.h \
  ogmrip-module-engine.h \
  ogmrip-module.h

libogmrip_module_la_LDFLAGS = \
  -version-info $(OGMRIP_PLUGIN_LT_VERSION)

libogmrip_module_la_LIBADD = \
  $(top_builddir)/libogmrip-media/libogmrip-media.la

libogmrip_module_ladir = \
  $(includedir)/ogmrip-module

INCLUDES = \
  -I$(top_srcdir)/libogmrip-media

