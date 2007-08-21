/*
 * Copyright (c) 2006, Seznam.cz, a.s.
 * All Rights Reserved.
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

using namespace Sphinx;

//--------------------------------------------------------------------------------

Query_t::Query_t(unsigned int size)
{
    dataSize = size;
    data = new unsigned char[dataSize];
    dataStartPtr = dataEndPtr = 0;
    error= false;
    convertEndian = false;
}//konstruktor

Query_t::~Query_t()
{
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

Query_t &Query_t::operator= (const Query_t &val)
{
    delete [] data;
    data = val.data;
    dataSize = val.dataSize;
    dataEndPtr = val.dataEndPtr;
    dataStartPtr = val.dataStartPtr;

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
            }
            throw Sphinx::ConnectionError_t(
                strError("recv error"));
        }

        dataEndPtr += result;
        bytesToRead -= result;
    }

}
