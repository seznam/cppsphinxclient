#
# C++ sphinx search client library
# Copyright (C) 2007  Seznam.cz, a.s.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Seznam.cz, a.s.
# Radlicka 2, Praha 5, 15000, Czech Republic
# http://www.seznam.cz, mailto:sphinxclient@firma.seznam.cz
#
#
# $Id$
#
# DESCRIPTION
# A makefile template for sphinxclient.
#
# AUTHORS
# Jan Kirschner <jan.kirschner@firma.seznam.cz>
#
# HISTORY
# 2006-12-14  (kirschner)
#             Created.
#


SUBDIRS = src include

# debian packaging scripts
EXTRA_DIST = test/testdata.sql doc/Doxyfile

noinst_PROGRAMS = sphinxtest sphinxtest2 keywordstest sphinxtest3 sphinxtest4 sphinxtest-mva64

# path to includes
AM_CPPFLAGS = -I ./include

sphinxtest_SOURCES = sphinxtest.cc
sphinxtest_LDADD = src/libsphinxclient.la

sphinxtest2_SOURCES = sphinxtest2.cc
sphinxtest2_LDADD = src/libsphinxclient.la

keywordstest_SOURCES = keywordstest.cc
keywordstest_LDADD = src/libsphinxclient.la

sphinxtest3_SOURCES = sphinxtest3.cc
sphinxtest3_LDADD = src/libsphinxclient.la

sphinxtest4_SOURCES = sphinxtest4.cc
sphinxtest4_LDADD = src/libsphinxclient.la

sphinxtest_mva64_SOURCES = sphinxtest-mva64.cc
sphinxtest_mva64_LDADD = src/libsphinxclient.la

