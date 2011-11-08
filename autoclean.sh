#!/bin/sh

rm -f missing
rm -f aclocal.m4
rm -f install-sh
rm -f intltool-extract
rm -f intltool-merge
rm -f intltool-update
rm -f intltool-extract.in
rm -f intltool-merge.in
rm -f intltool-update.in
rm -f configure
rm -f depcomp
rm -f config.h.in
rm -f config.sub
rm -f config.guess
rm -f gtk-doc.make
rm -f ltmain.sh
rm -f mkinstalldirs

rm -f po/Makefile.in.in

rm -f m4/*.m4

rm -rf autom4te.cache

find . -name "Makefile.in" -exec rm -f '{}' ';'
