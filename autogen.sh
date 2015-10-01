#!/bin/sh
# Run this to generate all the initial makefiles, etc.
test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.

olddir=`pwd`

cd $srcdir

aclocal --install || exit $?
glib-gettextize --force --copy || exit $?
intltoolize --force --copy --automake || exit $?
autoreconf --verbose --force --install -Wno-portability || exit $?

cd $olddir

test -n "$NOCONFIGURE" || "$srcdir/configure" --enable-compile-warnings=error --enable-debug=yes "$@"
