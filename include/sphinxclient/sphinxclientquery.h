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
 * SphinxClientQuery header file
 *
 * AUTHOR
 * Jan Kirschner <jan.kirschner@firma.seznam.cz>
 *
 * HISTORY
 * 2007-01-03 (jan.kirschner)
 *            First draft.
 */

//! @file sphinxclientquery.h

#ifndef __SPHINXAPIQUERY_H__
#define __SPHINXAPIQUERY_H__

#include <string>
#include <stdint.h>


namespace Sphinx
{

// forward declaration
class Client_t;

struct Query_t
{
    unsigned char *data;
    unsigned int dataEndPtr;
    unsigned int dataStartPtr;
    unsigned int dataSize;
    bool error;
    bool convertEndian;

    Query_t(unsigned int size=1024);
    ~Query_t();

    Query_t &operator << (unsigned short);
    Query_t &operator << (uint32_t);
    Query_t &operator << (uint64_t);
    Query_t &operator << (float);
    Query_t &operator << (const std::string &);
    Query_t &operator << (const Query_t &);

    Query_t &operator >> (uint32_t &);
    Query_t &operator >> (uint64_t &);
    Query_t &operator >> (float &);
    Query_t &operator >> (unsigned short &);
    Query_t &operator >> (std::string &);

    Query_t &operator = (const Query_t &);
    Query_t(const Query_t &source);

    bool operator ! () const { return error; }

    void doubleSizeBuffer();
    void clear();
    unsigned int getLength() const { return dataEndPtr-dataStartPtr; }

    void read(int socket_d, int bytesToRead, Client_t &connection,
              const std::string &stage);

    /** @brief reads data on readable socket
      *
      * @param socket_d socket to perform read on
      * @param bytesToRead bytes pending
      * @return 0 if read is done and there is nothing to read
      *         1 if something has been read, but some is remaining
      *         -1 if nothing has been read, try again
      */
    int readOnReadable(int socket_d, int &bytesToRead, const std::string &stage);

    /** @brief writes data on writable socket
      *
      * @param socket_d socket to perform write on
      * @param bytesSent bytes already sent
      * @return 0 if write is done and there is nothing to write
      *         1 if something has been written, but some is remaining
      *         -1 if nothing has been written, try again
      */
    int writeOnWritable(int socket_d, unsigned int &bytesSent, const std::string &stage);

};//struct

}//namespace

#endif

