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
# 2006-04-14  (kirschner)
#             Created.
#


# warn on all
AM_CXXFLAGS = -Wall -g -O2

# path to includes
AM_CPPFLAGS = -I../include

# compile this library
lib_LTLIBRARIES = libsphinxclient.la

# from the these sources
libsphinxclient_la_SOURCES = sphinxclient.cc sphinxclientquery.cc value.cc \
        filter.cc queryversions.cc querymachine.cc

libsphinxclient_la_LIBADD = -L.
libsphinxclient_la_DEPENDENCIES = 

# with these flags (version info etc.)
libsphinxclient_la_LDFLAGS = @VERSION_INFO@


