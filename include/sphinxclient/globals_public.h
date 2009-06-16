/*
 *
 * C++ sphinx search client library
 * Copyright (C) 2007  Seznam.cz, a.s.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Seznam.cz, a.s.
 * Radlicka 2, Praha 5, 15000, Czech Republic
 * http://www.seznam.cz, mailto:sphinxclient@firma.seznam.cz
 *
 *
 * $Id$
 *
 * DESCRIPTION
 * Public global enum declarations (command versions)
 *
 * AUTHOR
 * Jan Kirschner <jan.kirschner@firma.seznam.cz>
 *
 * HISTORY
 * 2007-01-03 (jan.kirschner)
 *            First draft.
 */

//! @file globals_public.h

#ifndef __SPHINX_GLOBALS_PUBLIC_H__
#define __SPHINX_GLOBALS_PUBLIC_H__

namespace Sphinx {

enum SearchCommandVersion_t { VER_COMMAND_SEARCH_0_9_6 = 0x101,
                              VER_COMMAND_SEARCH_0_9_7 = 0x104,
                              VER_COMMAND_SEARCH_0_9_7_1 = 0x107,
                              VER_COMMAND_SEARCH_0_9_8 = 0x113,
                              VER_COMMAND_SEARCH_0_9_9 = 0x116 };

enum UpdateCommandVersion_t { VER_COMMAND_UPADTE_0_9_8 = 0x101 };

enum KeywordsCommandVersion_t { VER_COMMAND_KEYWORDS_0_9_8 = 0x100 };

}//namespace

#endif

