#! /bin/sh
touch NEWS && touch AUTHORS
libtoolize --copy --force && aclocal && automake --gnu --add-missing --copy && autoconf

