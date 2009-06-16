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
 * $Id$
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
#include <sphinxclient/sphinxclientquery.h>
#include <sphinxclient/error.h>
#include <sphinxclient/globals.h>

#include <sstream>

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

//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// query version handlers declarations
//------------------------------------------------------------------------------

void buildQueryVersion(const std::string &, const Sphinx::SearchConfig_t &,
                       Sphinx::Query_t &);

void parseResponseVersion(Sphinx::Query_t &, Sphinx::SearchCommandVersion_t,
                          Sphinx::Response_t &);

void buildHeader(Sphinx::Command_t, unsigned short, int, Sphinx::Query_t &,
                 int queryCount=1);

void buildUpdateRequest_v0_9_8(Sphinx::Query_t &data,
                               const std::string &index,
                               const Sphinx::AttributeUpdates_t &at);

void parseUpdateResponse_v0_9_8(Sphinx::Query_t &data, uint32_t &updateCount);

void buildKeywordsRequest_v0_9_8(Sphinx::Query_t &data,
                                 const std::string &index,
                                 const std::string &query,
                                 bool fetchStats);

void parseKeywordsResponse_v0_9_8(Sphinx::Query_t &data,
                                  std::vector<Sphinx::KeywordResult_t> &result,
                                  bool fetchStats);

//------------------------------------------------------------------------------
// Connection configuration
//------------------------------------------------------------------------------
/*
namespace Sphinx {

struct PrivateConnectionConfig_t
{
    std::string host;
    unsigned short port;

    bool keepAlive;
    int32_t connectTimeout;
    int32_t readTimeout;
    int32_t writeTimeout;

    PrivateConnectionConfig_t(const std::string &host, unsigned short port,
                              bool kAlive, int32_t cTm, int32_t rTm,
                              int32_t wTm)
        : host(host), port(port), keepAlive(kAlive), connectTimeout(cTm),
          readTimeout(rTm), writeTimeout(wTm) {}
};//struct

}//namespace


Sphinx::ConnectionConfig_t::ConnectionConfig_t(const std::string &host,
        unsigned short port, bool keepAlive, int32_t connectTimeout,
        int32_t readTimeout, int32_t writeTimeout)
    : cfgData(new Sphinx::PrivateConnectionConfig_t(
                      host, port, keepAlive, connectTimeout, readTimeout,
                      writeTimeout))
{}

Sphinx::ConnectionConfig_t::~ConnectionConfig_t()
{
    delete cfgData;
}//konec fce
*/

Sphinx::ConnectionConfig_t::ConnectionConfig_t(const std::string &host,
        unsigned short port, bool keepAlive, int32_t connectTimeout,
        int32_t readTimeout, int32_t writeTimeout) : host(host), port(port),
                                               keepAlive(keepAlive),
                                               connectTimeout(connectTimeout),
                                               readTimeout(readTimeout),
                                               writeTimeout(writeTimeout)
{}

//------------------------------------------------------------------------------

Sphinx::SearchConfig_t::SearchConfig_t(SearchCommandVersion_t cmdVer):
    msgOffset(0), msgLimit(20), minId(0), maxId(0), minTimestamp(0),
    maxTimestamp(0xFFFFFFFF), minGroupId(0), maxGroupId(0xFFFFFFFF),
    matchMode(SPH_MATCH_ALL), sortMode(SPH_SORT_RELEVANCE),
    rankingMode(SPH_RANK_PROXIMITY_BM25),
    sortBy(""), groupBy(""), groupFunction(SPH_GROUPBY_DAY), maxMatches(1000),
    groupSort("@group desc"), commandVersion(cmdVer), indexes("*"),
    searchCutOff(0), distRetryCount(0), distRetryDelay(0), maxQueryTime(0),
    selectClause("*")
{}

void Sphinx::SearchConfig_t::addRangeFilter(std::string attrName, uint64_t minValue,
                                       uint64_t maxValue,
                                       bool excludeFlag)
{
    this->filters.push_back(new RangeFilter_t(attrName, minValue,
                                          maxValue, excludeFlag));
}

void Sphinx::SearchConfig_t::addEnumFilter(std::string attrName,
                                      const Int64Array_t &values,
                                      bool excludeFlag)
{
    this->filters.push_back(new EnumFilter_t(attrName, values,
                                         excludeFlag));
}

void Sphinx::SearchConfig_t::addFloatRangeFilter(std::string attrName,
    float minValue, float maxValue, bool excludeFlag)
{
    this->filters.push_back(new FloatRangeFilter_t(attrName, minValue,
                                                   maxValue, excludeFlag));
}

void Sphinx::SearchConfig_t::addAttributeOverride(
                                 const std::string &attrName,
                                 AttributeType_t attrType,
                                 const std::map<uint64_t, Value_t> &values)
{
    attributeOverrides[attrName] = std::make_pair(attrType, values);
}//konec fce

void Sphinx::SearchConfig_t::addAttributeOverride(
                                 const std::string &attrName,
                                 AttributeType_t attrType,
                                 uint64_t docId, const Value_t &value)
{
    // get or insert attribute entry
    std::pair<AttributeType_t, std::map<uint64_t, Value_t> >
        &override = attributeOverrides[attrName];

    // update the entry
    override.first = attrType;
    override.second[docId] = value;
}//konec fce

//------------------------------------------------------------------------------

struct SocketCloser_t
{
    SocketCloser_t(int &fd)
            : fd(fd), doClose(true)
    {}

    ~SocketCloser_t()
    {
        if (doClose && (fd > -1))
        {
            TEMP_FAILURE_RETRY(::close(fd));
            fd = -1;
        }
    }

    int &fd;
    bool doClose;
};

//------------------------------------------------------------------------------

void Sphinx::Response_t::clear()
{
    entry.clear();
    word.clear();
    field.clear();
    attribute.clear();
    entriesGot = 0;
    entriesFound = 0;
    timeConsumed = 0;
}//konec fce

//------------------------------------------------------------------------------

Sphinx::Client_t::Client_t(const ConnectionConfig_t &settings)
    : connection(settings), socket_d(-1)
{}//konstruktor

Sphinx::Client_t::~Client_t()
{
    if (socket_d > -1)
    {
        TEMP_FAILURE_RETRY(::close(socket_d));
        socket_d = -1;
    }//if
}//destruktor

//------------------------------------------------------------------------------


void Sphinx::Client_t::connect()
{
    // reconnect socket when connected
    if (!connection.keepAlive && (socket_d > -1))
    {
        TEMP_FAILURE_RETRY(::close(socket_d));
        socket_d = -1;
    }

    // initialize closer (initially closes socket when valid)
    SocketCloser_t closer(socket_d);

    // test the current connection
    if (socket_d > -1)
    {
        closer.doClose = false;

        fd_set rfdset;
        FD_ZERO(&rfdset);
        FD_SET(socket_d, &rfdset);
        // wait for no time
        struct timeval timeout = { 0, 0 };

        switch (::select(socket_d + 1, &rfdset, NULL, NULL, &timeout))
        {
        case 0:
            // passed test
            break;

        case -1:
            // test failed - close socket
            TEMP_FAILURE_RETRY(::close(socket_d));
            socket_d = -1;
            break;

        default:
            // are there any data available?
            char buff;
            switch (recv(socket_d, &buff, 1, MSG_PEEK))
            {
            case -1:
            case 0:
                // close the socket
                TEMP_FAILURE_RETRY(::close(socket_d));
                socket_d = -1;
                break;

            default:
                // OK
                break;
            }
        }
    }

    // socket closed
    if (socket_d < 0)
    {
        // new socket
        if ((socket_d = ::socket(PF_INET, SOCK_STREAM, 0)) < 0)
        {
            // oops! error
            throw Sphinx::ConnectionError_t(
                            std::string("Unable to create socket (")
                            + std::string(strerror(errno)) + std::string(")"));
        }

        if (::fcntl(socket_d, F_SETFL, O_NONBLOCK) < 0)
        {
            throw Sphinx::ConnectionError_t(
                           std::string("Cannot set socket non-blocking: ")
                           + std::string(strerror(errno)));
        }

        // nastavíme okam¾ité odesílání packetù po TCP
        // disabluje Nagle algoritmus, zdrojáky Apache øíkají, ¾e je to OK
        /*int just_say_no = 1;
        if (::setsockopt(socket_d, IPPROTO_TCP, TCP_NODELAY,
                         (char*) &just_say_no, sizeof(int)) < 0)
        {
            throw HTTPError_t(HTTP_SYSCALL,
                              "Cannot set socket non-delaying: "
                              "<%d, %s>.", ERRNO, STRERROR(ERRNO));
        }*/

        // reslove host name
        struct hostent *he = gethostbyname(connection.host.c_str());
        if (!he) {
            // oops
            throw Sphinx::ConnectionError_t(
                                      std::string("Cannot resolve host '")
                                      + connection.host + std::string("'."));
        }//if

        //get the IP address
        struct in_addr ipaddr;
        ipaddr = *reinterpret_cast<in_addr*>(he->h_addr_list[0]);

        // update remote address struct
        struct sockaddr_in addrStruct;

        addrStruct.sin_family = AF_INET;
        addrStruct.sin_addr = ipaddr;
        addrStruct.sin_port = htons(connection.port);
        memset(addrStruct.sin_zero, 0x0, 8);

        // connect to the server
        if (::connect(socket_d, (struct sockaddr *) &addrStruct,
                      sizeof(struct sockaddr)) < 0 )
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

            // descriptor set
            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(socket_d, &wfds);

            // create timeout
            struct timeval timeout =
                {
                    connection.connectTimeout / 1000,
                    (connection.connectTimeout % 1000) * 1000
                };

            // connect finished ?
            int ready = ::select(socket_d + 1, NULL, &wfds, NULL,
                            (connection.connectTimeout < 0) ? NULL : &timeout);

            if (ready <= 0)
            {
                switch (ready)
                {
                case 0:
                    throw Sphinx::ConnectionError_t(
                        "Can't connect socket: connect timeout");
                default:
                    throw Sphinx::ConnectionError_t(
                        strError("Can't select on socket"));
                }
            }

            // connect call status
            socklen_t len = sizeof(int);
            int status;

            if (::getsockopt(socket_d, SOL_SOCKET, SO_ERROR,
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

            // connect OK => do not close socket!
            closer.doClose = false;

            //test server version
            Sphinx::Query_t buff;
            buff.read(socket_d, 4, *this, "connect handshake");

            if(!buff.getLength())
                throw Sphinx::ServerError_t(
                                    "Unable to determine the server version.");

            uint32_t version=0;
            buff >> version;
            if (!buff || version < 1)
            {
                throw Sphinx::ServerError_t(
                            "Protocol version on the server is less than 1.");
            }//if

            buff.clear();
            buff << (uint32_t) 1;
            sendData(buff);
        } else {
            // immediate non-blocking success -> do not close :-)
            closer.doClose = false;
        }//if connect < 0
    }//if socket closed
}//konec fce

void Sphinx::Client_t::close()
{
    if(socket_d > -1)
    {
        TEMP_FAILURE_RETRY(::close(socket_d));
        socket_d = -1;
    }//if
}//konec fce


void Sphinx::Client_t::waitSocketReadable(const std::string &stage) {

    struct timeval tstruct, *ptstruct;
    fd_set rfds;
    int ready;

    // descriptor set
    FD_ZERO(&rfds);
    FD_SET(socket_d, &rfds);

    // create timeout
    tstruct.tv_sec = connection.readTimeout/1000;
    tstruct.tv_usec = (connection.readTimeout % 1000) * 1000;
    ptstruct = (connection.readTimeout<0) ? NULL : &tstruct;

    while ((ready = ::select(socket_d + 1, &rfds, NULL, NULL, ptstruct)) <= 0)
    {
        //no data available or other error
        if (ready == 0) {
            // read timeout expired
            throw Sphinx::ConnectionError_t(
                stage +
                std::string("::Error reading response (read timeout expired)"));
        } else if (ready < 0) {
            // EINTR - select was interrupted, restart select
            if (errno == EINTR) continue;
            throw Sphinx::ConnectionError_t(
                stage +
                strError("::Error reading response while selecting"));
        }
    }
    // ready to read here
}

int Sphinx::Client_t::receiveResponse(Query_t &buff, unsigned short &version)
{

    //receive the first part - 8B header
    buff.read(socket_d, 8, *this, "query header");

    unsigned short status;
    uint32_t length;

    //read status, version, data length
    buff.convertEndian = true;

    if (!(buff >> status))
    {
        throw Sphinx::ServerError_t("Unable to read response status.");
    }//if

    if (!(buff >> version))
    {
        throw Sphinx::ServerError_t("Unable to read response version.");
    }//if

    if (!(buff >> length))
    {
        throw Sphinx::ServerError_t("Unable to read response length.");
    }//if


    // debug
    //printf("length=%d, buffLength=%d\n", length, buff.getLength());
    
    // read whole message to buffer
    buff.read(socket_d, length, *this, "query response");

    return status;
}//konec fce

void Sphinx::Client_t::sendData(const Query_t &buff)
{
    struct timeval tstruct, *ptstruct;
    fd_set wfds;
    int result;

    // descriptor set
    FD_ZERO(&wfds);
    FD_SET(socket_d, &wfds);

    // create timeout
    tstruct.tv_sec = connection.writeTimeout/1000;
    tstruct.tv_usec = (connection.writeTimeout % 1000) * 1000;
    ptstruct = (connection.writeTimeout<0) ? NULL : &tstruct;

    int ready = ::select(socket_d + 1, NULL, &wfds, NULL, ptstruct);

    // not managed to send
    if (ready <= 0)
        throw Sphinx::ConnectionError_t(
                       "Client to server connection error while sending data.");

    unsigned int bytesSent=buff.dataStartPtr;

    do {
        result = send(socket_d, buff.data + bytesSent,
                      buff.dataEndPtr - bytesSent, 0);

        if (result <= 0)
            throw Sphinx::ConnectionError_t(
                       "Client to server connection error while sending data.");

        bytesSent += result;
    }
    while (bytesSent < buff.dataEndPtr);
}//konec fce


//-------------------------------------------------------------------------


Sphinx::MultiQuery_t::MultiQuery_t(SearchCommandVersion_t cv)
    : commandVersion(cv), queryCount(0)
{}//konec fce

void Sphinx::MultiQuery_t::initQuery(SearchCommandVersion_t cv)
{
    queryCount = 0;
    commandVersion = cv;
    queries.clear();
}//konec fce


void Sphinx::MultiQuery_t::addQuery(const std::string& query,
                                    const SearchConfig_t &queryAttr)
{
    // query attr command version must match query command version
    if(commandVersion != queryAttr.commandVersion)
        throw ClientUsageError_t("multiQuery version does not match "
                                 "added query version.");

    // increase query count
    queryCount++;

    // append to multiquery
    queries.convertEndian = true;
    buildQueryVersion(query, queryAttr, queries);
}//konec fce

int Sphinx::MultiQuery_t::getQueryCount() const { return queryCount; }

const Sphinx::Query_t &Sphinx::MultiQuery_t::getQueries() const {
    return queries;
}

Sphinx::SearchCommandVersion_t Sphinx::MultiQuery_t::getCommandVersion() const {
    return commandVersion;
}//konec fce

//-------------------------------------------------------------------------

unsigned short Sphinx::Client_t::processRequest(
    const Query_t &query,
    Query_t &data)
{
    unsigned short responseVersion;
    SocketCloser_t socketCloser(socket_d);

    //if not connected, connect to the server
    connect();

    //send request
    sendData(query);

    //receive response
    if (int status = receiveResponse(data, responseVersion) != SEARCHD_OK)
    {
        std::ostringstream err;
        unsigned char errBuff[200];
        int length = data.dataEndPtr - data.dataStartPtr - 4;
        memcpy(errBuff, data.data + data.dataStartPtr + 4, length);
        *(errBuff + length + 1) = '\0';
        err << "response status not OK ( " << status << " ), : " << errBuff;
        throw Sphinx::MessageError_t(err.str());
    }//if

    //don't close the connection, when keepalive is set
    if (connection.keepAlive)
        socketCloser.doClose = false;

    return responseVersion;
}//konec fce

void Sphinx::Client_t::query(const std::string& query,
                             const SearchConfig_t &attrs,
                             Response_t &response)
{
    Query_t data, request;
    unsigned short responseVersion;
    data.convertEndian = true;
    request.convertEndian = true;

    //-------------------build query---------------
    buildQueryVersion(query, attrs, data);
    buildHeader(SEARCHD_COMMAND_SEARCH, attrs.commandVersion, data.getLength(),
                request);
    request << data;

    //---------------process request--------------
    responseVersion = processRequest(request, data);    

    //--------- parse response -------------------
    parseResponseVersion(data, attrs.commandVersion, response);
}//konec fce


void Sphinx::Client_t::query(const MultiQuery_t &query,
                                  std::vector<Response_t> &response)
{
    // test, if multiquery initialized, get the query
    const Query_t &queries = query.getQueries();
    int queryCount = query.getQueryCount();
    int queriesLength = queries.getLength();
    SearchCommandVersion_t cmdVer = query.getCommandVersion();

    if(!queryCount || !queriesLength)
        throw ClientUsageError_t("multiQuery not initialised or zero length.");

    // prepare whole command and response buffer
    Query_t data, request;
    data.convertEndian = true;
    request.convertEndian = true;

    buildHeader(SEARCHD_COMMAND_SEARCH, cmdVer, queries.getLength(),
                request, queryCount);
    request << queries;

    // send request and receive response
    unsigned short responseVersion;
    responseVersion = processRequest(request, data);    

    //parse responses and return
    std::string lastQueryWarning;

    for(int i=0 ; i<queryCount ; i++) {
        Response_t resp;

        try {
            parseResponseVersion(data, cmdVer, resp);
        } catch (const Warning_t &wt) {
            std::ostringstream msg;
            msg << "Query " << (i+1) << ": " << wt.what();
            lastQueryWarning = msg.str();
        }//try

        response.push_back(resp);
    }//for

    if (!lastQueryWarning.empty())
        throw Warning_t(lastQueryWarning);
}//konec fce

//-----------------------------------------------------------------------------


Sphinx::AttributeUpdates_t::AttributeUpdates_t()
    : commandVersion(Sphinx::VER_COMMAND_UPADTE_0_9_8)
{}

void Sphinx::AttributeUpdates_t::setAttributeList(
                                         const std::vector<std::string> &attr)
{
    attributes = attr;
    values.clear();
}//konec fce

void Sphinx::AttributeUpdates_t::addAttribute(const std::string &attrName)
{
    attributes.push_back(attrName);
    values.clear();
}//konec fce

void Sphinx::AttributeUpdates_t::addDocument(uint64_t docId,
                                         const std::vector<Value_t> &values)
{
    if(values.size() != attributes.size())
        throw ClientUsageError_t("Attribute name count must match value count "
                                 "set in document.");

    this->values[docId] = values;
}//konec fce

void Sphinx::AttributeUpdates_t::addDocument(uint64_t docId, ValueType_t t, ...)
{
    va_list ap;
    std::vector<Value_t> values;
    int valueCount = attributes.size();

    va_start(ap, t);

    for (int i=0 ; i<valueCount ; i++)
    {
        switch(t) {
            case VALUETYPE_UINT32: {
                uint32_t value = va_arg(ap, uint32_t);
                values.push_back(value);
                break; }
            case VALUETYPE_FLOAT: {
                float value = (float)va_arg(ap, double);
                values.push_back(value);
                break; }
            default:
                break;
        }//switch
    }//for
    va_end(ap);
    
    this->values[docId] = values;
}//konec fce

void Sphinx::AttributeUpdates_t::setCommandVersion(UpdateCommandVersion_t v)
{
    commandVersion = v;
}//konec fce

void Sphinx::Client_t::updateAttributes(const std::string &index,
                                        const AttributeUpdates_t &at)
{
    Query_t data, request;
    unsigned short responseVersion;
    uint32_t updatedCount;

    // build request
    data.convertEndian = request.convertEndian = true;
    buildUpdateRequest_v0_9_8(data, index, at);

    // prepend header
    buildHeader(SEARCHD_COMMAND_UPDATE, at.commandVersion, data.getLength(),
                request);
    request << data;

    // process request
    responseVersion = processRequest(request, data);    

    //parse response
    parseUpdateResponse_v0_9_8(data, updatedCount);
    
    if(updatedCount != at.values.size())
        throw ClientUsageError_t("Some documents weren't updated "
                                 "- probably invalid id");
}//konec fce

//-----------------------------------------------------------------------------

std::vector<Sphinx::KeywordResult_t> Sphinx::Client_t::getKeywords(
    const std::string &index,
    const std::string &query,
    bool getWordStatistics)
{
    Query_t data, request;
    unsigned short responseVersion;
    std::vector<KeywordResult_t> result;

    // build request
    data.convertEndian = request.convertEndian = true;
    buildKeywordsRequest_v0_9_8(data, index, query, getWordStatistics);

    // prepend header
    buildHeader(SEARCHD_COMMAND_KEYWORDS, VER_COMMAND_KEYWORDS_0_9_8,
                data.getLength(), request);
    request << data;

    // process request
    responseVersion = processRequest(request, data);    

    //parse response
    parseKeywordsResponse_v0_9_8(data, result, getWordStatistics);
    
    return result;
}//konec fce

//-----------------------------------------------------------------------------

void sphinxClientDummy()
{ }








