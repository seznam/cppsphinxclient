/*
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
 * $Id: sphinxclient.cc 25 2012-06-27 13:01:10Z danfis $
 * 
 * DESCRIPTION
 * SphinxClient function definitions - connection to server
 * and communication over the sphinx searchd protocol including
 * search result parsing
 * 
 * AUTHOR
 * Jan Kirschner <jan.kirschner@firma.seznam.cz>
 * 
 * HISTORY
 * 2006-12-11 (jan.kirschner)
 *             First draft.
 */


#include <sphinxclient/sphinxclient.h>
#include <sphinxclient/globals.h>
#include <sphinxclient/error.h>

#include <algorithm>

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>

#include <stdarg.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <unistd.h>

#include "timer.h"
#include "querymachine.h"

#define INT32_MAX      (2147483647)

//---------------------------- QueryMachine_t ---------------------------------

void Sphinx::QueryMachine_t::addQuery(const Query_t &query)
{

    queries.push_back(query);


    Query_t dataEndian;
    dataEndian.convertEndian = true;
    // initialise data
    responses.push_back(Query_t(dataEndian));
    versions.push_back(Query_t());
    responseStatuses.push_back(0);
    responseVersions.push_back(0);
    qs.push_back(QS_WAIT_WR_CONNECT);
    bytesToRead.push_back(0);
    bytesWritten.push_back(0);
    connectRetries.push_back(cconfig.getConnectRetriesCount());
    /*
    printf("connect retries count %d, delay: %dms\n",
        cconfig.getConnectRetriesCount(), cconfig.getConnectRetryWait());
    */
    timeouts.push_back(cconfig.getConnectTimeout());

    // connect
    int socket_d = setupConnection(cconfig, ai, aip);

    // set state and input poll structure
    fdes.addQuery(socket_d, POLLOUT);
}

void Sphinx::QueryMachine_t::launch()
{
    _fer_timer_t timer;

    // poll main cycle
    while (!finished()) {
        //printf("polling %lu descriptors\n", fdes.getSize());
        ferTimerStart(&timer);
        int ret = poll(fdes.fds, fdes.getSize(), getMinTimeout());
        ferTimerStop(&timer);
        unsigned long timeElapsedMs = ferTimerElapsedInMs(&timer);

        // decrement timeouts
        decrementTimeouts(timeElapsedMs);

        if (ret > 0) {
            // success
            for (nfds_t f = 0; f < fdes.getSize(); f++) {

                if (fdes.fds[f].revents & POLLIN || fdes.fds[f].revents & POLLPRI) {
                    // can read something
                    handleRead(f);
                } else if (fdes.fds[f].revents & POLLOUT) {
                    // can write something
                    handleWrite(f);
                } else if (fdes.fds[f].revents != 0) {
                    // something went wrong
                    /*printf("Fd %d (%lu.) failed - revents=%u, setting failed\n",
                        fdes.fds[f].fd, f, fdes.fds[f].revents);
                    */
                    // query index
                    size_t q = fdes.getQueryIndex(f);
                    // close socket
                    std::ostringstream o;
                    o << q+1 << ". query, ";
                    switch (qs[q]) {
                        case QS_WAIT_WR_CONNECT :
                        case QS_WAIT_RD_VERSION :
                        case QS_WAIT_WR_VERSION :
                        case QS_WAIT_WR_REQUEST :
                        case QS_WAIT_RD_RESPONSE_HEADER :
                        case QS_WAIT_RD_RESPONSE :
                        case QS_FINISHED :
                        case QS_FAILED :
                        case QS_WAIT_RETRY_CONNECT :
                            throw ConnectionError_t(o.str() +
                                "error at state: " + getQueryStateString(q));
                            break;
                    }
                }
            }
        } else if (ret == 0) {
            // timeout exceeded - no revents occured
            for (size_t i = 0; i < qs.size(); i++) {
                if (timeouts[i] <= 1) {

                    switch (qs[i]) {
                    // timeout exceeded or is going to be exceeded
                        case QS_WAIT_WR_CONNECT :
                        {
                            if (connectRetries[i] > 0) {
                                //printf("%lu. query: connect timeout exceeded, waiting\n", i+1);
                                // set query to special waiting state
                                qs[i] = QS_WAIT_RETRY_CONNECT;
                                // add retry wait interval to wait for
                                setRetryWaitTimeout(i);
                                // close current socket
                                fdes.removeFd(fdes.getPollIndex(i));
                                // decrement connect retries
                                connectRetries[i] -= 1;
                            } else {
                                std::ostringstream o;
                                o << i+1 << ". query, ";
                                // no retries left -> fail
                                throw Sphinx::ConnectionError_t(o.str() +
                                    "connection timeout out, no retries left");
                            }
                            break;
                        }
                        case QS_WAIT_RD_VERSION :
                        case QS_WAIT_WR_VERSION :
                        case QS_WAIT_WR_REQUEST :
                        case QS_WAIT_RD_RESPONSE_HEADER :
                        case QS_WAIT_RD_RESPONSE :
                        case QS_FINISHED :
                        case QS_FAILED :
                        {
                        std::ostringstream o;
                            o << i+1 << ". query, ";
                            throw ConnectionError_t(o.str() +
                                "error at state: " + getQueryStateString(i));
                            break;
                        }
                        case QS_WAIT_RETRY_CONNECT:
                        {
                            //printf("%lu. query: waiting finsihed.\n", i+1);
                            // wait timer (between connect retries) expired
                            // setup connection
                            int socket_d = setupConnection(cconfig, ai, aip);

                            // set state, timeout and input poll structure
                            qs[i] = QS_WAIT_WR_CONNECT;
                            setConnectTimeout(i);
                            fdes.addQuery(socket_d, POLLOUT,i);
                            break;
                        }
                    }
                }
            }
            //printf("handling timeout finished...\n");
        } else {
            // error
            if (errno == EINTR) continue;
            // other error - log and go to error state (and break?)
            /*DBG3( 
                strError("::Error reading response while selecting"));
            */
            
        }
    }
}


int Sphinx::QueryMachine_t::setupConnection(
    const Sphinx::ConnectionConfig_t &cconfig,
    struct addrinfo *&ai, struct addrinfo *&aip)
{
    int socket_d = -1;
    if (!ai) {
        // address info not found
        std::ostringstream o;
        o << cconfig.getPort();

        struct addrinfo hints;

         memset(&hints, 0, sizeof(struct addrinfo));
         hints.ai_family = AF_UNSPEC;        /* Allow IPv4 or IPv6 */
         hints.ai_socktype = SOCK_STREAM;    /* TCP socket */
         hints.ai_flags = AI_PASSIVE;        /* For wildcard IP address */
         hints.ai_protocol = IPPROTO_TCP;    /* Any protocol */
         hints.ai_canonname = NULL;
         hints.ai_addr = NULL;
         hints.ai_next = NULL;

    
        int ret = getaddrinfo(cconfig.getHost().c_str(), o.str().c_str(), &hints, &ai);
        if (ret != 0) {
            throw Sphinx::ConnectionError_t(
                                      std::string("Cannot resolve host '")
                                      + cconfig.getHost() + std::string("'."));
        }
    } 

    for (aip = ai; aip != NULL; aip = aip->ai_next) {
        socket_d = ::socket(aip->ai_family, aip->ai_socktype,
                      aip->ai_protocol);
        if (socket_d == -1)
             continue;

        if (::fcntl(socket_d, F_SETFL, O_NONBLOCK) < 0)
        {
            throw Sphinx::ConnectionError_t(
                           std::string("Cannot set socket non-blocking: ")
                           + std::string(strerror(errno)));
        }
        break;
    }
    if (aip == NULL) {
        // oops! error (no address succeeded)
        throw Sphinx::ConnectionError_t(
                        std::string("Unable to create socket (")
                        + std::string(strerror(errno)) + std::string(")"));
    }

    if (::connect(socket_d, aip->ai_addr, aip->ai_addrlen) < 0)
    {
        switch (errno)
        {
        case EINPROGRESS:    // connect in progress
        case EALREADY:       // connect already called
        case EWOULDBLOCK:    // no errors
            break;

        default:
            throw Sphinx::ConnectionError_t(
                strError("Can't connect socket"));
        }

    } else {
        // immediate non-blocking success 
        //printf("immediate non-blocking success for socket %d", socket_d);
    }//if connect < 0
    return socket_d;
}


std::string Sphinx::QueryMachine_t::getQueryStateString(int i) const
{
    switch (qs[i]) {
        case QS_WAIT_WR_CONNECT :
            return std::string("connecting");
            break;
        case QS_WAIT_RD_VERSION :
            return std::string("reading server version");
            break;
        case QS_WAIT_WR_VERSION:
            return std::string("writing client version");
            break;
        case QS_WAIT_WR_REQUEST:
            return std::string("writing request");
            break;
        case QS_WAIT_RD_RESPONSE_HEADER:
            return std::string("reading response header");
            break;
        case QS_WAIT_RD_RESPONSE:
            return std::string("reading response body");
            break;
        case QS_FINISHED:
            return std::string("finished");
            break;
        case QS_FAILED:
            return std::string("failed");
            break;
        case QS_WAIT_RETRY_CONNECT:
            return std::string("waiting between connect retries");
            break;
    }
    return std::string("Unknown state");
}


void Sphinx::QueryMachine_t::handleWrite(nfds_t f)
{
    // query index
    size_t q = fdes.getQueryIndex(f);

    //printf("handle writable: qs[%lu]=%d\n", q, qs[q]);
    // do actions depended on query state
    switch (qs[q]) {
        case QS_WAIT_RD_VERSION :
        case QS_WAIT_RD_RESPONSE_HEADER :
        case QS_WAIT_RD_RESPONSE :
        case QS_FINISHED :
        case QS_FAILED :
        case QS_WAIT_RETRY_CONNECT :
        {
            // fail - throw exception
            /*
            qs[q] = QS_FAILED;
            // close socket
            fdes.removeFd(f);
            disableTimeout(q);
            */
            std::ostringstream msg;
            msg << "Unexpected status " << qs[q] << " of " << q+1 << ". query.";
            throw Sphinx::ConnectionError_t(msg.str());
            break;
        }
        case QS_WAIT_WR_CONNECT :
        {
            // socket writable after connect
    
            // check connect call status
            socklen_t len = sizeof(int);
            int status;

            if (::getsockopt(fdes.fds[f].fd, SOL_SOCKET, SO_ERROR,
                             &status, &len))
            {
                throw Sphinx::ConnectionError_t(
                    strError("Cannot get socket info"));
            }
            if (status)
            {
                throw Sphinx::ConnectionError_t(
                    strError("Cannot connect socket", status));
            }

            // prepare for reading server version
            qs[q] = QS_WAIT_RD_VERSION;
            bytesToRead[q] = 4;
            // we want read - wait for socket readable
            fdes.fds[f].events = 0x0 | POLLIN;
            // set read timeout
            setReadTimeout(q);
            break;
        }
        case QS_WAIT_WR_VERSION :
        {
            // socket writable, write version
            int ret = versions[q].writeOnWritable(fdes.fds[f].fd, bytesWritten[q],
                                            "write_version");

            if (ret == 0) {
                // all written, now send query
                // query already prepared at queries[q], just send

                // set status
                qs[q] = QS_WAIT_WR_REQUEST;
                // reset and set pollfd - wait for writable
                fdes.fds[f].events = 0x0 | POLLOUT;
                // reset bytes to write
                bytesWritten[q] = 0;
                // set write timeout
                setWriteTimeout(q);
            } else if (ret > 0) {
                // continue, not all written, something written, set timeout
                setWriteTimeout(q);
            } else {
                // continue - interrupted, etc - retry
            }
            break;
        }
        case QS_WAIT_WR_REQUEST :
        {
            // socket, writable, write request
            int ret = queries[q].writeOnWritable(fdes.fds[f].fd, bytesWritten[q],
                                                "write_request");

            if (ret == 0) {
                // all written, now read response header
                qs[q] = QS_WAIT_RD_RESPONSE_HEADER;
                // pollfds
                fdes.fds[f].events = 0x0 | POLLIN;
                // set expected header length
                bytesToRead[q] = 8;
                // set timeout
                setReadTimeout(q);
            } else if (ret > 0) {
                // continue, not all written, something written, set timeout
                setWriteTimeout(q);
            } else {
                // continue - interrupted, etc - retry
            }
            break;
        }
    } 
}

void Sphinx::QueryMachine_t::handleRead(nfds_t f)
{

    // query index
    size_t q = fdes.getQueryIndex(f);

    //printf("handle readable: qs[%lu]=%d\n", q, qs[q]);
    // do actions depended on query state
    switch (qs[q]) {
        case QS_WAIT_WR_CONNECT :
        case QS_WAIT_WR_VERSION :
        case QS_WAIT_WR_REQUEST :
        case QS_FINISHED :
        case QS_FAILED :
        case QS_WAIT_RETRY_CONNECT :
        {
            // fail - throw exception
            /*
            qs[q] = QS_FAILED;
            // close socket
            fdes.removeFd(f);
            disableTimeout(q);
            */

            std::ostringstream msg;
            msg << "Unexpected status " << qs[q] << " of " << q+1 << ". query.";
            throw Sphinx::ConnectionError_t(msg.str());
            break;
        }
        case QS_WAIT_RD_VERSION :
        {
            // read version
            int ret = versions[q].readOnReadable(fdes.fds[f].fd, bytesToRead[q],
                                                 "read_version");
            if (ret == 0) {
                // all read, process version
                uint32_t version = 0;
                versions[q] >> version;
                if (!versions[q] || version < 1)
                {
                    throw Sphinx::ServerError_t(
                                "Protocol version on the server is less than 1.");
                }//if
                // send our version to server
                versions[q].clear();
                versions[q] << (uint32_t) 1;
                // set state
                qs[q] = QS_WAIT_WR_VERSION;
                // reset and set pollfd - wait for writable
                fdes.fds[f].events = 0x0 | POLLOUT;
                // reset bytes to write
                bytesWritten[q] = 0;
                // set write timeout
                setWriteTimeout(q);
            } else if (ret > 0) {
                // continue - not all read, set timeout
                setReadTimeout(q);
            } else {
                // continue - interrupted, etc - retry
            }
            break;
        }
        case QS_WAIT_RD_RESPONSE_HEADER :
        {
            int ret = responses[q].readOnReadable(fdes.fds[f].fd, bytesToRead[q],
                                                "read_response_header");
            // read response header
            if (ret == 0) {
                // all read, process header
                uint32_t length;
                
                //read status, version, data length
                // already set
                // responses[q].convertEndian = true;

                if (!(responses[q] >> responseStatuses[q]))
                {
                    throw Sphinx::ServerError_t("Unable to read response status.");
                }//if

                if (!(responses[q] >> responseVersions[q]))
                {
                    throw Sphinx::ServerError_t("Unable to read response version.");
                }//if

                if (!(responses[q] >> length))
                {
                    throw Sphinx::ServerError_t("Unable to read response length.");
                }//if
            
                // debug
                /*printf("length=%d, status=%u, buffLength=%d\n", length,
                        responseStatuses[q], responses[q].getLength());
                */

                // prepare to receive response body
                qs[q] = QS_WAIT_RD_RESPONSE;
                bytesToRead[q] = length;
                // reset and set pollfd - wait for readable
                fdes.fds[f].events = 0x0 | POLLIN;
                // set read timeout
                setReadTimeout(q);
            } else if (ret > 0) {
                // continue - not all read, set timeout
                setReadTimeout(q);
            } else {
                // continue - interrupted, etc - retry
            }
            break;
        }
        case QS_WAIT_RD_RESPONSE :
        {
            // read response
            int ret = responses[q].readOnReadable(fdes.fds[f].fd, bytesToRead[q],
                                                 "read_response");
            if (ret == 0) {
                // all response has been read, finish
                qs[q] = QS_FINISHED;
                //printf("%lu. QS_FINISHED, datalen: %u\n", q, responses[q].dataEndPtr);

                if (responseStatuses[q] != SEARCHD_OK) {
                    std::ostringstream err;
                    unsigned char errBuff[200];
                    Query_t &data = responses[q];
                    int length = data.dataEndPtr - data.dataStartPtr - 4;
                    memcpy(errBuff, data.data + data.dataStartPtr + 4, length);
                    *(errBuff + length) = '\0';
                    err << "response status not OK ( " << responseStatuses[q] << " ), : " << errBuff;
                    throw Sphinx::MessageError_t(err.str());
                }

                // done
                fdes.removeFd(f);
                disableTimeout(q);
            } else if (ret > 0) {
                // continue - not all read
                setReadTimeout(q);
            } else {
                // continue - interrupted, etc - retry
            }
            break;
        }
    }
}

bool Sphinx::QueryMachine_t::finished() const
{
    bool finished = true;
    for (std::vector<QueryState_t>::const_iterator i = qs.begin();
            i != qs.end(); i++)
        // check all query states
        if (*i != QS_FINISHED && *i != QS_FAILED) {
            // query not finished
            finished = false;
            break;
        }
    return finished;
}

void Sphinx::QueryMachine_t::setReadTimeout(size_t index)
{
    timeouts[index] = cconfig.getReadTimeout();
}

void Sphinx::QueryMachine_t::setWriteTimeout(size_t index)
{
    timeouts[index] = cconfig.getWriteTimeout();
}

void Sphinx::QueryMachine_t::setConnectTimeout(size_t index)
{
    timeouts[index] = cconfig.getConnectTimeout();
}

void Sphinx::QueryMachine_t::setRetryWaitTimeout(size_t index)
{
    timeouts[index] = cconfig.getConnectRetryWait();
}

void Sphinx::QueryMachine_t::disableTimeout(size_t index)
{
    timeouts[index] = INT32_MAX;
}

void Sphinx::QueryMachine_t::decrementTimeouts(int ms) {
    for (std::vector<int>::iterator i = timeouts.begin();
        i != timeouts.end(); i++)
    {
        // decrement
        *i -= ms;
    }
}

int Sphinx::QueryMachine_t::getMinTimeout()
{
    return *std::min_element(timeouts.begin(), timeouts.end());
}


//--------------------------- FileDescriptots_t -------------------------------

Sphinx::FileDescriptors_t::FileDescriptors_t()
    : size(0)
{}

Sphinx::FileDescriptors_t::~FileDescriptors_t()
{
    while (size > 0) removeFd(size-1);
}

size_t Sphinx::FileDescriptors_t::getPollIndex(size_t queryIndex) const
{
    if (query2fds[queryIndex] == (size_t) -1) {
        throw ClientUsageError_t("poll index out of range.");
    }
    return query2fds[queryIndex];
}

size_t Sphinx::FileDescriptors_t::getQueryIndex(size_t pollIndex) const
{
    return fds2query[pollIndex];
}

void Sphinx::FileDescriptors_t::removeFd(size_t pollIndex)
{
    // close socket
    TEMP_FAILURE_RETRY(::close(fds[pollIndex].fd));
    //printf("socket closed\n");

    // unlink removed query
    query2fds[fds2query[pollIndex]] = (size_t) -1;

    // shift to left
    for (size_t i=pollIndex; i< (size-1); i++) {
        // shift fds left
        fds[i].fd = fds[i+1].fd;
        fds[i].events = fds[i+1].events;
        fds[i].revents = fds[i+1].revents;
        // shift fds2 query
        fds2query[i] = fds2query[i+1];
        // reindex query2fds
        query2fds[fds2query[i]] = i;
    }
    // shrink
    size--;
}

size_t Sphinx::FileDescriptors_t::addQuery(int socket_d, short events) {
    return addQuery(socket_d, events, size);
}

size_t Sphinx::FileDescriptors_t::addQuery(int socket_d, short events, size_t queryIndex) {
    if (size+1 > MAX_PARALLEL_CONNECTIONS) {
        throw ClientUsageError_t("MAX_PARALLEL_CONNECTIONS exceeded");
    }
    query2fds[queryIndex] = size;
    fds2query[size] = queryIndex;


    // set socket descriptor and initial events
    fds[size].fd = socket_d;
    fds[size].events = events;

    size++;
    return size;
}

