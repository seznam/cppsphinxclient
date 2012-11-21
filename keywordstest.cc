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
* $Id$
*
* DESCRIPTION
* A sphinxclient sample and testing program.
*
* AUTHORS
* Jan Kirschner <jan.kirschner@firma.seznam.cz>
*
* HISTORY
* 2006-12-14  (kirschner)
*             Created.
*
* Quick compile:
* g++  -g sphinxtest.cc -Iinclude/  -Lsrc/.libs/ -lsphinxclient -o sphinxtest
*/

#include <sphinxclient/sphinxclient.h>
#include <stdio.h>

//------------------------------------------------------------------------------


int main()
{
    Sphinx::ConnectionConfig_t config (
        "localhost", // hostname
        3312,        // port
        true,        // keepalive
        2000,        // connect timeout
        20000,       // read timeout
        2000         // write timeout
    );
       
    Sphinx::Client_t connection(config);
    std::vector<Sphinx::KeywordResult_t> result;

    printf("starting.....\n");

    std::string query = "ahoj bla morce";
    std::string index = "test1";

    try{
        //keywords
        result = connection.getKeywords(index, query, true);
        printf("query success.\n");
    }
    catch(Sphinx::Error_t e)
    {
        printf("query error:\n%s\n", e.errMsg.c_str());
        return 2;
    }

    printf("----------------- got %2lu words ---------------------------------\n",
           result.size());

    for(unsigned i=0 ; i<result.size() ; i++)
    {
        Sphinx::KeywordResult_t &entry = result[i];
        printf ("%2u. %11s -> %11s (docs: %u, hits: %u)\n",
                (i+1),
                entry.tokenized.c_str(),
                entry.normalized.c_str(),
                entry.statistics.docsHit,
                entry.statistics.totalHits);
    }//for response

    printf("------------------------------ end -----------------------------\n");

    return 0;
}//main

