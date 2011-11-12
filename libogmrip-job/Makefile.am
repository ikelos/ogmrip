lib_LTLIBRARIES = \
  libogmrip-job.la

libogmrip_job_la_SOURCES = \
  ogmjob-bin.c       \
  ogmjob-container.c \
  ogmjob-pipeline.c  \
  ogmjob-queue.c     \
  ogmjob-spawn.c     \
  ogmjob-task.c

libogmrip_job_ladir = \
  $(includedir)/ogmrip-job

libogmrip_job_la_HEADERS = \
  ogmjob-bin.h       \
  ogmjob-container.h \
  ogmjob-pipeline.h  \
  ogmjob-queue.h     \
  ogmjob-spawn.h     \
  ogmjob-task.h      \
  ogmrip-job.h

libogmrip_job_la_LDFLAGS = \
  -version-info $(OGMRIP_JOB_LT_VERSION)

libogmrip_job_la_LIBADD = \
  $(top_builddir)/libogmrip-base/libogmrip-base.la

INCLUDES = \
  -I$(top_srcdir)/libogmrip-base

if MAINTAINER_MODE
INCLUDES += \
  -DG_ENABLE_DEBUG
endif

