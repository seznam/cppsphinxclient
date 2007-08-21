/*
 * Copyright (c) 2006, Seznam.cz, a.s.
 * All Rights Reserved.
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
    Query_t &operator << (const std::string &);
    Query_t &operator << (const Query_t &);

    Query_t &operator >> (uint32_t &);
    Query_t &operator >> (unsigned short &);
    Query_t &operator >> (std::string &);

    Query_t &operator = (const Query_t &);

    bool operator ! () const { return error; }

    void doubleSizeBuffer();
    void clear();
    unsigned int getLength() const { return dataEndPtr-dataStartPtr; }

    void read(int socket_d, int bytesToRead, Client_t &connection,
              const std::string &stage);
};//struct

}//namespace

#endif

