#
# Copyright (C) 2006  Seznam.cz, a.s.
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
libsphinxclient_la_SOURCES = sphinxclient.cc sphinxclientquery.cc

libsphinxclient_la_LIBADD = -L.
libsphinxclient_la_DEPENDENCIES = 

# with these flags (version info etc.)
libsphinxclient_la_LDFLAGS = @VERSION_INFO@


