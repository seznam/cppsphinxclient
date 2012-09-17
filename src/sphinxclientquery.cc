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
 * SphinxClient::Query function definitions - implements operators
 * >> and << and dynamic memory reallocation
 * 
 * AUTHOR
 * Jan Kirschner <jan.kirschner@firma.seznam.cz>
 *
 * HISTORY
 * 2006-12-11 (jan.kirschner)
 *            First draft.
 */

#include <sphinxclient/sphinxclientquery.h>
#include <sphinxclient/sphinxclient.h>

#include <sstream>
#include <netdb.h>

//--------------------------------------------------------------------------------
// 64bit network byte order workaround

#include <bits/byteswap.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN
    #define sphinx_hton64l(x) __bswap_64(x)
#elif __BYTE_ORDER == __BIG_ENDIAN
    #define sphinx_hton64l(x) (x)
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
    #define sphinx_ntoh64l(x) __bswap_64(x)
#elif __BYTE_ORDER == __BIG_ENDIAN
    #define sphinx_ntoh64l(x) (x)
#endif

//--------------------------------------------------------------------------------

using namespace Sphinx;

//--------------------------------------------------------------------------------

Query_t::Query_t(unsigned int size)
{
    dataSize = size;
    data = new unsigned char[dataSize];
    //printf("%p constructor with size, created data: %p\n", this, data);
    dataStartPtr = dataEndPtr = 0;
    error= false;
    convertEndian = false;
}//konstruktor

Query_t::Query_t(const Query_t &source)
{
    // copy data
    dataSize = source.dataSize;
    dataStartPtr = source.dataStartPtr;
    dataEndPtr = source.dataEndPtr;
    error = source.error;
    convertEndian = source.convertEndian;
    data = new unsigned char[source.dataSize];
    //printf("%p copy const from %p, created data: %p ...\n", this, &source,data);
    //printf("data cpy %p->%p ... \n", source.data, data);
    memcpy(data, source.data, source.dataSize);
} //copy contructor

Query_t &Query_t::operator= (const Query_t &val)
{

    if (&val != this) {

        //printf("ggg3");
        data = val.data;
        dataSize = val.dataSize;
        dataEndPtr = val.dataEndPtr;
        dataStartPtr = val.dataStartPtr;
        convertEndian = val.convertEndian;

        delete [] data;
        data = new unsigned char[val.dataSize];
        memcpy(data, val.data, val.dataSize);
    }
    return *this;
}//konec fce

Query_t::~Query_t()
{
    //printf("%p destructor\n", this);
    delete [] data;
}//konstruktor

void Query_t::doubleSizeBuffer()
{
    unsigned char *bkup;

    bkup = new unsigned char[dataSize];
    memcpy(bkup, data, dataEndPtr);
    delete [] data;

    dataSize *= 2;
    data = new unsigned char[dataSize];
    memcpy(data, bkup, dataEndPtr);
    delete [] bkup;
}//konec fce

void Query_t::clear()
{
    dataEndPtr = 0;
    dataStartPtr = 0;
}//konec fce

Query_t &Query_t::operator << (unsigned short val)
{
    if ((dataEndPtr+sizeof(short)) >= dataSize)
        doubleSizeBuffer();
    //printf("<< short 0x%X\n", val);

    if (convertEndian)
        val = htons(val);

    memcpy(data+dataEndPtr, &val, sizeof(short));
    dataEndPtr += sizeof(short);
    return *this;
}//konec fce

Query_t &Query_t::operator << (uint32_t val)
{
    if ((dataEndPtr+sizeof(int32_t)) >= dataSize)
        doubleSizeBuffer();
    //printf("<< int32_t 0x%lX\n", val);

    if (convertEndian)
        val = htonl(val);

    memcpy(data+dataEndPtr, &val, sizeof(int32_t));
    dataEndPtr += sizeof(int32_t);
    return *this;
}//konec fce

Query_t &Query_t::operator << (uint64_t val)
{
    if ((dataEndPtr+sizeof(int64_t)) >= dataSize)
        doubleSizeBuffer();
    //printf("<< int64_t 0x%lX\n", val);

    if (convertEndian)
        val = sphinx_hton64l(val);

    memcpy(data+dataEndPtr, &val, sizeof(int64_t));
    dataEndPtr += sizeof(int64_t);
    return *this;
}//konec fce

Query_t &Query_t::operator << (float val)
{
    error = false;
    // float is transferred as a 4-byte dword. if size of float doesn't match
    // 4 bytes, it is an error
    if (sizeof(float) != sizeof(uint32_t)) {
        error = true;
        return *this;
    }//if

    if ((dataEndPtr+sizeof(int32_t)) >= dataSize)
        doubleSizeBuffer();
    //printf("<< int64_t 0x%lX\n", val);
    uint32_t nVal;
    memcpy(&nVal, &val, sizeof(uint32_t));

    if (convertEndian)
        nVal = htonl(nVal);

    memcpy(data+dataEndPtr, &nVal, sizeof(int32_t));
    dataEndPtr += sizeof(int32_t);
    return *this;
}//konec fce

Query_t &Query_t::operator << (const std::string &val)
{
    while ((dataEndPtr+val.size()+sizeof(int32_t)) >= dataSize)
        doubleSizeBuffer();
    //printf("<< string '%s'\n", val.c_str());
    (*this) << (uint32_t)val.size();
    memcpy(data+dataEndPtr, val.c_str(), val.size());
    dataEndPtr += val.size();
    return *this;
}//konec fce

Query_t &Query_t::operator << (const Query_t &val)
{
    while ((dataEndPtr+val.getLength()) >= dataSize)
        doubleSizeBuffer();

    memcpy(data+dataEndPtr, val.data+val.dataStartPtr, val.getLength());
    dataEndPtr += val.getLength();
    return *this;
}//konec fce

Query_t &Query_t::operator >> (uint32_t &val)
{
    error=false;

    if ((dataEndPtr-dataStartPtr) >= sizeof(int32_t))
    {
        memcpy(&val, data+dataStartPtr, sizeof(int32_t));
        dataStartPtr += sizeof(int32_t);
    }//if
    else
        error=true;

    if (convertEndian)
        val = ntohl(val);

    //printf(">> int32_t 0x%lX\n", val);
    return *this;
}//konec fce

Query_t &Query_t::operator >> (uint64_t &val)
{
    error=false;

    if ((dataEndPtr-dataStartPtr) >= sizeof(int64_t))
    {
        memcpy(&val, data+dataStartPtr, sizeof(int64_t));
        dataStartPtr += sizeof(int64_t);
    }//if
    else
        error=true;

    if (convertEndian)
        val = sphinx_ntoh64l(val);

    //printf(">> int32_t 0x%lX\n", val);
    return *this;
}//konec fce

Query_t &Query_t::operator >> (float &val)
{
    error=false;

    if ((dataEndPtr-dataStartPtr) >= sizeof(int32_t) &&
       sizeof(float)==sizeof(int32_t))
    {
        uint32_t nVal;
        memcpy(&nVal, data+dataStartPtr, sizeof(int32_t));
        dataStartPtr += sizeof(int32_t);

        if (convertEndian)
            nVal = ntohl(nVal);
        memcpy(&val, &nVal, sizeof(float));
    }//if
    else
        error=true;

    //printf(">> float hex 0x%lX\n", val);
    return *this;
}//konec fce

Query_t &Query_t::operator >> (unsigned short &val)
{
    error=false;

    if ((dataEndPtr-dataStartPtr) >= sizeof(short))
    {
        memcpy(&val, data+dataStartPtr, sizeof(short));
        dataStartPtr += sizeof(short);
    }//if
    else
        error=true;

    if (convertEndian)
        val = ntohs(val);

    //printf(">> short 0x%X\n", val);
    return *this;
}//konec fce

Query_t &Query_t::operator >> (std::string &val)
{
    uint32_t len=0;
    error=false;

    if (!((*this) >> len))
        return *this;

    if ((dataEndPtr-dataStartPtr) >= len)
    {
        std::stringstream sstr;

        for (unsigned int i=0;i<len;i++)
            sstr << data[dataStartPtr++];

        val = sstr.str();
    }//if
    else
        error=true;

    //printf(">> string %s (%ld)\n", val.c_str(), len);
    return *this;
}//konec fce

   
   
void Query_t::read(int socket_d, int bytesToRead, Client_t &connection,
                   const std::string &stage)
{
    // clear the buffer
    clear();

    while (bytesToRead > 0) {

        // chceck the free space
        int free_space = dataSize - dataEndPtr;
        if (free_space <= 0) {
            //message to large for this buffer, double the size
            doubleSizeBuffer();
            // recompute free space
            free_space = dataSize - dataEndPtr;
        }

        // wait for readable socket, throws exception Sphinx::ConnectionError_t
        connection.waitSocketReadable(stage);

        // read data
        int result = recv(socket_d, data + dataEndPtr,
            (bytesToRead > free_space ? free_space : bytesToRead) ,0);

        // debug print
        //printf("result: %d bytes read, old endPtr=%d, new endPtr=%d, size=%d, space before=%d, space after=%d\n",
        //        result, dataEndPtr, dataEndPtr+result, dataSize, free_space, free_space-result );

        // readable socket, byt nothing to read (conenction closed) 
        if (result == 0) {
            throw Sphinx::ConnectionError_t(
                stage +
                strError("::recv error: connection closed"));
        }
        if (result < 0) {
            if (errno == EINTR) {
                // interrupted - try again
                continue;
            } else if (errno == EINPROGRESS) {
                // blocking operation still in progress, try again after
                // a few microsecs
                usleep(10);
                continue;
            }

            throw Sphinx::ConnectionError_t(
                strError("recv error"));
        }

        dataEndPtr += result;
        bytesToRead -= result;
    }


}


int Query_t::readOnReadable(int socket_d, int &bytesToRead, const std::string &stage) {

    // check the free space
    int free_space = dataSize - dataEndPtr;
    if (free_space <= 0) {
        //message to large for this buffer, double the size
        doubleSizeBuffer();
        // recompute free space
        free_space = dataSize - dataEndPtr;
    }

    // read data
    int result = recv(socket_d, data + dataEndPtr,
            (bytesToRead > free_space ? free_space : bytesToRead) ,0);

    // debug print
    //printf("result: %d bytes read, old endPtr=%d, new endPtr=%d, size=%d, space before=%d, space after=%d\n",
    //        result, dataEndPtr, dataEndPtr+result, dataSize, free_space, free_space-result );

    // readable socket, byt nothing to read (conenction closed) 
    if (result == 0) {
        throw Sphinx::ConnectionError_t(
            stage +
            strError("::recv error: connection closed"));
    }
    if (result < 0) {
        if (errno == EINTR) {
            // interrupted - try again
            return -1;
        } else if (errno == EINPROGRESS) {
            // blocking operation still in progress, try again after
            // a few microsecs
            usleep(10);
            return -1;
        }

        throw Sphinx::ConnectionError_t(
            strError("recv error"));
    }

    dataEndPtr += result;
    bytesToRead -= result;

    // not all done
    if (bytesToRead > 0) return 1;

    // all done 
    return 0;

    
}



int Query_t::writeOnWritable(int socket_d, unsigned int &bytesSent, const std::string &stage)
{

    errno = 0;
    int result = send(socket_d, data + bytesSent,
                      dataEndPtr - bytesSent, 0);

    if (result < 0) {
        // error
        if (errno == EAGAIN) {
            // socket was writable, but can't write => error
            throw Sphinx::ConnectionError_t(stage +
                strError("::send error: can't write on writable"));
        } else if (errno == EINTR) {
            // interrupted - try again
            return -1;
        } else {
            throw Sphinx::ConnectionError_t(stage +
                strError("::send error: can't write"));
        }
    } else if (result == 0) {
        // nothing written
        throw Sphinx::ConnectionError_t(stage +
            strError("::send error: written 0 bytes write on writable"));
    } else {
        // ok, something written
        bytesSent += result;

        if (bytesSent < dataEndPtr) {
            // something remaining for write
            return 1;
        }
        // all written
        return 0;
    }
}

