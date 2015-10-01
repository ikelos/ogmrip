lib_LTLIBRARIES = \
  libogmrip-module.la

libogmrip_module_la_SOURCES = \
  ogmrip-module-engine.c \
  ogmrip-module-object.c

libogmrip_module_la_HEADERS = \
  ogmrip-module-engine.h \
  ogmrip-module-object.h \
  ogmrip-module.h

libogmrip_module_la_CFLAGS = \
  $(WARN_CFLAGS)

libogmrip_module_la_LDFLAGS = \
  -version-info $(OGMRIP_MODULE_LT_VERSION) \
  $(WARN_LDFLAGS)

libogmrip_module_ladir = \
  $(includedir)/ogmrip-module

MAINTAINERCLEANFILES = \
  Makefile.in
