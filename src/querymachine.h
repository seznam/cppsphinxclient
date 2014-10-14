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
 * $Id: sphinxclient.h 21 2009-07-03 14:06:35Z honkir $
 *
 * DESCRIPTION
 * SphinxClient header file - communication library for the sphinx
 * search server
 *
 * AUTHOR
 * Jan Kirschner <vaclav.urbanek@firma.seznam.cz>
 *
 * HISTORY
 * 2011-09-11 (Vaclav Urbanek)
 *             First draft.
 */

//! @file querymachine.h

#ifndef __QUERYMACHINE_H__
#define __QUERYMACHINE_H__

#include <sstream>
#include <string>
#include <vector>
#include <stdint.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "error.h"

namespace Sphinx
{

#define MAX_PARALLEL_CONNECTIONS 10

// forward
class ConnectionConfig_t;


/* @brief Holds active socket descriptors, mapping between 
 *        queries (queryindexes) and socket descriptors
 */
class FileDescriptors_t {
public:
    /* @brief constructor
     *
     * @param cconfig connection config
     */
    FileDescriptors_t();

    /* @brief desctructor removes all file descriptors
     *
     */
    ~FileDescriptors_t();

    /** @brief Gets index of active descriptor
      *        for given query
      *
      * @param queryIndex query identifier
      * @return index to socket descriptor
      */
    size_t getPollIndex(size_t queryIndex) const;

     /** @brief Gets index of query
      *        for given socket descriptor
      *
      * @param pollIndex index to socket descriptor
      * @return index to query
      */
    size_t getQueryIndex(size_t pollIndex) const;

    /* @brief Removes active socket from desciptor set,
     *        closes connection, and update internal structures
     * 
     * @param pollIndex index to socket descriptor
     */
    void removeFd(size_t pollIndex);

    /** @brief Adds query to machine (allocate socket, update internal structs)
      *
      * @return index of newly allocated query
      */
    size_t addQuery(int socket_d, short events, size_t queryIndex);
    size_t addQuery(int socket_d, short events);

    /** @brief get count of active socket descriptors 
      * @return count of active socket descriptors
      */
    size_t getSize() const { return size; }

    /// polling fd set
    struct pollfd fds[MAX_PARALLEL_CONNECTIONS];
private:
    /// query->fds map
    /// index of query is the index in array,
    /// value is index into pollfd array
    size_t query2fds[MAX_PARALLEL_CONNECTIONS];

    /// fds->query map
    /// index of pollfd is the index in array,
    /// value of query
    size_t fds2query[MAX_PARALLEL_CONNECTIONS];

    // overall count of diledescriptors
    size_t size;

};

/* @brief State machine to send queries and receive
 *        responses with sphinx-searchd.
 *
 * After adding several queries by addQuery(), the state machine
 * is launched (by launch()). Machine maintains connection to searchd
 * for each query by non-blocking asynchronous i/o. When all responses are
 * successfully received, the machine is finished and responses
 * can be fetched by getResponse() method.
 * 
 * Even when single connection fails, all fails (throws ConnectionError_t)
 *
 * QueryMachine for now doesn't reuse connection. It resolves host
 * only once by one QueryMachine and then caches addressinfo.
 *
 * QueryMachine is not designed for repeatable use for now. Don't call
 * addQuery() after launch has been called.
 *
 */
class QueryMachine_t {


public:
    /* @brief constructor
     *
     * @param cconfig connection config
     */
    QueryMachine_t(const ConnectionConfig_t &cconfig)
        : cconfig(cconfig), ai(0x0), aip(0x0)
    {}

    /* @brief desctructor frees address info structures
     *
     */
    ~QueryMachine_t() 
    {
        freeaddrinfo(ai);
    }

    /** @brief adds query request for parralel processing
      *
      * Adds query, initialize next slot in fdes, 
      * setup connection and go to QS_WAIT_WR_CONNECT state
      *
      * @param query to add
      */
    void addQuery(const Query_t &query);

    /** @brief launch query machine - start query processing
      *
      * query machine takes over program control
      */
    void launch();

    /** Gets response (call after launch successfully finished)
      * @param i query index
      * @return response for query with index i
      */
    Sphinx::Query_t & getResponse(int i) {return responses[i];}


private:
    /** @brief decrement current timeout for all active descriptors
      * @param ms miliseconds to decrement
      */
    void decrementTimeouts(int ms);

    /** @brief get the minimal timeout form all connections (descriptors)
      * @return minimal timeout (ms)
      */
    int getMinTimeout();

    /** @brief set timeout to initial read timeout
      * @param index query index
      */
    void setReadTimeout(size_t index);

    /** @brief set timeout to initial write timeout
      * @param index query index
      */
    void setWriteTimeout(size_t index);

    /** @brief set timeout to initial connect timeout
      * @param index query index
      */
    void setConnectTimeout(size_t index);

    /** Disable timeout for query (sets timeout to INT32_MAX seconds)
      * @param index query index
      */
    void disableTimeout(size_t index);

    /** 
      * @param index query index
      */
    void setRetryWaitTimeout(size_t index);

    /* @short Get textual representation of the i-th query state
     *
     * @param i query index
     * @return textual representation of the i-th query state
     */
    std::string getQueryStateString(int i) const;

    static int setupConnection(
        const Sphinx::ConnectionConfig_t &cconfig,
        struct addrinfo *&ai, struct addrinfo *&aip);

    /// Connect using unix domain socket. Expected to call this just
    /// if ConnectionConfig_t::isDomainSocketUsed() == true.
    static int setupLocalConnection(
        const Sphinx::ConnectionConfig_t &cconfig);

    /* @brief Internal state of query state machine
     */
    enum QueryState_t {
        /// waiting for connect to finish
        QS_WAIT_WR_CONNECT          = 0,
        /// waiting to read the protocol version from server
        QS_WAIT_RD_VERSION          = 1,
        /// waiting for write the protocol version to server
        QS_WAIT_WR_VERSION          = 2,
        /// wating to write reguest to server
        QS_WAIT_WR_REQUEST          = 3,
        /// waiting to read the response header from server
        QS_WAIT_RD_RESPONSE_HEADER  = 4,
        /// waiting to read the response body from server
        QS_WAIT_RD_RESPONSE         = 5,
        /// query is successfully processed
        QS_FINISHED                 = 6,
        /// query processing failed
        QS_FAILED                   = 7,
        QS_WAIT_RETRY_CONNECT       = 8,
    };

    /** handle socket writable on filedescriptor
      *
      * Do some action, change state and set new poll event on connection
      *
      * @param fdIndex identifier of connection (not a query!)
      */
    void handleWrite(nfds_t fdIndex);

    /** handle socket readable on filedescriptor
      *
      * Do some action, change state and set new poll event on connection
      *
      * @param fdIndex identifier of connection (not a query!)
      */
    void handleRead(nfds_t fdIndex);

    /** @brief returns true when all processing is done
      *
      */
    bool finished() const;

    /// file (socket) descriptors 
    FileDescriptors_t fdes;

    /// registered queries
    std::vector<Query_t> queries;
    /// responses of queries
    std::vector<Query_t> responses;
    /// readed server versions
    std::vector<Query_t> versions;

    // response statuses
    std::vector<unsigned short> responseStatuses;
    // response versions
    std::vector<unsigned short> responseVersions;
    
    /// states of machine (for each query)
    std::vector<QueryState_t> qs;

    /// pending bytes (current read) for each query
    std::vector<int> bytesToRead;
    /// pendind bytes (current write) for each query
    std::vector<unsigned int> bytesWritten;

    /// timeouts - current timeout (r/w/conn,timer) for query
    std::vector<int32_t> timeouts;

    /// connection config (address of searchd, timeouts)
    const ConnectionConfig_t &cconfig;

    /// cached addressinfo
    struct addrinfo *ai;
    /// pointer to selected address within address info
    struct addrinfo *aip;

    /// nr of retries in case od connect timeout occured (for each query)
    /// 0 == disabled
    std::vector<int> connectRetries;
};

}//namespace

#endif

