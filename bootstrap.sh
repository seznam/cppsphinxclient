#!/bin/sh
#
# Copyright (C) 2006  Seznam.cz, a.s.
#
# $Id$
#
# DESCRIPTION
# bootstrap.
#
# AUTHORS
# Jan Kirschner <jan.kirschner@firma.seznam.cz>
#
# HISTORY
# 2006-12-14  (kirschner)
#             Created.
#

set -ex

aclocal
libtoolize --force
automake --foreign --add-missing
autoconf
