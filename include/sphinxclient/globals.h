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
 * Global enum and types declaration
 *
 * AUTHOR
 * Jan Kirschner <jan.kirschner@firma.seznam.cz>
 *
 * HISTORY
 * 2007-01-03 (jan.kirschner)
 *            First draft.
 */

//! @file globals.h

#ifndef __SPHINX_GLOBALS_H__
#define __SPHINX_GLOBALS_H__

#include <sphinxclient/globals_public.h>

namespace Sphinx {

enum Command_t { SEARCHD_COMMAND_SEARCH = 0,
                 SEARCHD_COMMAND_EXCERPT = 1,
                 SEARCHD_COMMAND_UPDATE = 2 };

enum ExcerptCommandVersion_t { VER_COMMAND_EXCERPT = 0x100 };

enum Status_t { SEARCHD_OK = 0,
                SEARCHD_ERROR = 1,
                SEARCHD_RETRY = 2,
                SEARCHD_WARNING = 3 };

}//namespace

#endif

