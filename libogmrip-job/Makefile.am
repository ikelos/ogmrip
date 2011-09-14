lib_LTLIBRARIES = \
  libogmrip-job.la

BUILT_SOURCES = \
  ogmjob-marshal.c \
  ogmjob-marshal.h

libogmrip_job_la_SOURCES = \
  ogmjob-bin.c       \
  ogmjob-container.c \
  ogmjob-exec.c      \
  ogmjob-list.c      \
  ogmjob-log.c       \
  ogmjob-marshal.c   \
  ogmjob-pipeline.c  \
  ogmjob-queue.c     \
  ogmjob-spawn.c

libogmrip_job_ladir = \
  $(includedir)/ogmrip-job

libogmrip_job_la_HEADERS =  \
  ogmjob-bin.h         \
  ogmjob-container.h   \
  ogmjob-exec.h        \
  ogmjob-list.h        \
  ogmjob-log.h         \
  ogmjob-marshal.h     \
  ogmjob-pipeline.h    \
  ogmjob-queue.h       \
  ogmjob-spawn.h       \
  ogmrip-job.h

libogmrip_job_la_LDFLAGS = \
  -version-info $(OGMRIP_JOB_LT_VERSION)

ogmjob-marshal.h: ogmjob-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=ogmjob_cclosure_marshal > $@

ogmjob-marshal.c: ogmjob-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"ogmjob-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=ogmjob_cclosure_marshal >> $@

if MAINTAINER_MODE
INCLUDES = \
  -DG_ENABLE_DEBUG
endif

EXTRA_DIST = \
  ogmjob-marshal.list

CLEANFILES = \
  $(BUILT_SOURCES)

