/*
 * Copyright (c) 2006, Seznam.cz, a.s.
 * All Rights Reserved.
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

#include <sstream>

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <unistd.h>

//------------------------------------------------------------------------------

enum Command_t { SEARCHD_COMMAND_SEARCH = 0,
                 SEARCHD_COMMAND_EXCERPT = 1,
                 SEARCHD_COMMAND_UPDATE = 2 };

enum ExcerptCommandVersion_t { VER_COMMAND_EXCERPT = 0x100 };
enum UpdateCommandVersion_t { VER_COMMAND_UPDATE = 0x100 };

enum Status_t { SEARCHD_OK = 0,
                SEARCHD_ERROR = 1,
                SEARCHD_RETRY = 2,
                SEARCHD_WARNING = 3 };

//------------------------------------------------------------------------------

Sphinx::ConnectionConfig_t::ConnectionConfig_t(const std::string &host,
        unsigned short port, bool keepAlive, int32_t connectTimeout,
        int32_t readTimeout, int32_t writeTimeout) : host(host), port(port),
                                               keepAlive(keepAlive),
                                               connectTimeout(connectTimeout),
                                               readTimeout(readTimeout),
                                               writeTimeout(writeTimeout)
{}

Sphinx::SearchConfig_t::SearchConfig_t(SearchCommandVersion_t cmdVer):
    msgOffset(0), msgLimit(20), minId(0), maxId(0xFFFFFFFF), minTimestamp(0),
    maxTimestamp(0xFFFFFFFF), minGroupId(0), maxGroupId(0xFFFFFFFF),
    matchMode(SPH_MATCH_ALL), sortMode(SPH_SORT_RELEVANCE),
    sortBy(""), groupBy(""), groupFunction(SPH_GROUPBY_DAY), maxMatches(1000),
    groupSort("@group desc"), commandVersion(cmdVer), indexes("*")
{}

void Sphinx::SearchConfig_t::addRangeFilter(std::string attrName, uint32_t minValue,
                                       uint32_t maxValue,
                                       bool excludeFlag)
{
    this->filters.push_back(new RangeFilter_t(attrName, minValue,
                                          maxValue, excludeFlag));
}

void Sphinx::SearchConfig_t::addEnumFilter(std::string attrName,
                                      const IntArray_t &values,
                                      bool excludeFlag)
{
    this->filters.push_back(new EnumFilter_t(attrName, values,
                                         excludeFlag));
}


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
                          :connection(settings), socket_d(-1)
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

void buildQuery_v0_9_6(const std::string &query,
                       const Sphinx::SearchConfig_t &attrs,
                       Sphinx::Query_t &data)
{
    /*
    int32_t offset, limit, matchMode, sortMode
    int32_t number_of_groups {int32_t group_id}
    string query
    int32_t number_of_weights {int32_t weight}
    string indexes
    */

    //limits, modes
    data << (uint32_t) attrs.msgOffset << (uint32_t) attrs.msgLimit;
    data << (uint32_t) attrs.matchMode;
    data << (uint32_t) attrs.sortMode;

    //groups
    data << (uint32_t) attrs.groups.size();
    for (Sphinx::IntArray_t::const_iterator group = attrs.groups.begin() ;
         group != attrs.groups.end() ; group++)
    {
        data << (uint32_t) (*group);
    }//for

    //query
    data << query;

    //weights
    data << (uint32_t) attrs.weights.size();
    for (Sphinx::IntArray_t::const_iterator weight = attrs.weights.begin() ;
         weight != attrs.weights.end() ; weight++)
    {
        data << (uint32_t) (*weight);
    }//for

    //indexes
    data << attrs.indexes;

    //ranges
    data << attrs.minId << attrs.maxId;
    data << attrs.minTimestamp << attrs.maxTimestamp;
    data << attrs.minGroupId << attrs.maxGroupId;
}//konec fce


void buildQuery_v0_9_7(const std::string &query,
                       const Sphinx::SearchConfig_t &attrs,
                       Sphinx::Query_t &data)
{
    /*
    int32_t offset, limit, matchMode, sortMode
    string sort_by
    string query
    int32_t number_of_weights {int32_t weight}
    string indexes
    int32_t min_id, int32_t max_id
    int32_t (minmax_count+filers_count)
        {string attr, int32_t 0, int32_t min, int32_t max}
        {string attr, int32_t value_count, {int32_t value}}
    int32_t group_func, string group_by
    int32_t maxMatches
    */

    //limits, modes
    data << (uint32_t) attrs.msgOffset << (uint32_t) attrs.msgLimit;
    data << (uint32_t) attrs.matchMode;
    data << (uint32_t) attrs.sortMode;

    //sort_by
    data << attrs.sortBy;

    //query
    data << query;

    //weights
    data << (uint32_t) attrs.weights.size();
    for (Sphinx::IntArray_t::const_iterator weight = attrs.weights.begin() ;
         weight != attrs.weights.end() ; weight++)
    {
        data << (uint32_t) (*weight);
    }//for

    //indexes
    data << attrs.indexes;

    //id range
    data << attrs.minId << attrs.maxId;

    //filters
    data << (uint32_t)(attrs.minValue.size() + attrs.filter.size());

    std::map<std::string, uint32_t>::const_iterator min, max;
    for (min = attrs.minValue.begin(), max = attrs.maxValue.begin() ;
         min != attrs.minValue.end() ; min++, max++)
    {
        //string key
        data << min->first;
        //ranges
        data << (uint32_t) 0 << (uint32_t) min->second << (uint32_t) max->second;
    }//for

    for (std::map<std::string, Sphinx::IntArray_t >::const_iterator filt
         = attrs.filter.begin() ; filt != attrs.filter.end() ; filt++)
    {
        //string key, uint32_t value count
        data << filt->first;
        data << (uint32_t) filt->second.size();
        //values
        for (Sphinx::IntArray_t::const_iterator val = filt->second.begin() ;
             val != filt->second.end() ; val++)
        {
            data << (uint32_t)(*val);
        }//for
    }//for

    //group by
    data << (uint32_t) attrs.groupFunction;
    data << attrs.groupBy;

    //max matches
    data << (uint32_t) attrs.maxMatches;
}//konec fce


void buildQuery_v0_9_7_1(const std::string &query,
                       const Sphinx::SearchConfig_t &attrs,
                       Sphinx::Query_t &data)
{
    /*
    int32_t offset, limit, matchMode, sortMode
    string sort_by
    string query
    int32_t number_of_weights {int32_t weight}
    string indexes
    int32_t min_id, int32_t max_id 
    int32_t (range_filter_count+enum_filters_count+0.9.7.1_interface_filters)
        {string attr, int32_t 0, int32_t min, int32_t excludeFlag}
        {string attr, int32_t value_count, {int32_t value}, int32_t excludeFlag}
    int32_t group_func, string group_by, string groupSort
    int32_t maxMatches
    */

    //limits, modes
    data << (uint32_t) attrs.msgOffset << (uint32_t) attrs.msgLimit;
    data << (uint32_t) attrs.matchMode;
    data << (uint32_t) attrs.sortMode;

    //sort_by
    data << attrs.sortBy;

    //query
    data << query;

    //weights
    data << (uint32_t) attrs.weights.size();
    for (Sphinx::IntArray_t::const_iterator weight = attrs.weights.begin() ;
         weight != attrs.weights.end() ; weight++)
    {
        data << (uint32_t) (*weight);
    }//for

    //indexes
    data << attrs.indexes;

    //id range
    data << attrs.minId << attrs.maxId;

    //filters
    data << (uint32_t)(attrs.minValue.size() + attrs.filter.size()
                       + attrs.filters.size());

    /*
     *  old filter interface - range filters
     */
    std::map<std::string, uint32_t>::const_iterator min, max;
    for (min = attrs.minValue.begin(), max = attrs.maxValue.begin() ;
         min != attrs.minValue.end() ; min++, max++)
    {
        //string key
        data << min->first;
        //ranges
        data << (uint32_t) 0
             << (uint32_t) min->second
             << (uint32_t) max->second;
        // excludeFlag - false
        data << (uint32_t) false;
    }//for

    /*
     * old filter interface - enum (value) filters
     */

    for (std::map<std::string, Sphinx::IntArray_t >::const_iterator filt
         = attrs.filter.begin() ; filt != attrs.filter.end() ; filt++)
    {
        //string key, uint32_t value count
        data << filt->first;
        data << (uint32_t) filt->second.size();
        //values
        for (Sphinx::IntArray_t::const_iterator val = filt->second.begin() ;
             val != filt->second.end() ; val++)
            data << (uint32_t)(*val);
        // excludeFlag
        data << (uint32_t) false;
    }//for

    /*
     * new filter interface
     */
    for (std::vector<Sphinx::Filter_t *>::const_iterator it= attrs.filters.begin();
         it != attrs.filters.end(); it++)
    {
        (*it)->dumpToBuff(data);
    }
    //for (uint32_t i=0; i<attrs.filters.size(); i++) {
    //    attrs.filters[i]->dumpToBuff(data);
    // }

    //group by
    data << (uint32_t) attrs.groupFunction;
    data << attrs.groupBy;

    //max matches
    data << (uint32_t) attrs.maxMatches;

    // group sort criterion
    data << attrs.groupSort;
}//konec fce


void parseResponse_v0_9_6(Sphinx::Query_t &data, Sphinx::Response_t &response)
{
    uint32_t matchCount;
    uint32_t wordCount;

    response.clear();

    //number of entries to fetch
    if (!(data >> matchCount))
        throw Sphinx::MessageError_t(
                         "Can't read any data. Probably zero-length response.");

    for (unsigned int i=0 ; i<matchCount ; i++)
    {
        Sphinx::ResponseEntry_t entry;
        data >> entry.documentId;
        data >> entry.groupId;
        data >> entry.timestamp;
        data >> entry.weight;

        response.entry.push_back(entry);
    }//for

    //uint32_t totalGot, totalFound, timeConsumed;

    data >> response.entriesGot;
    data >> response.entriesFound;
    data >> response.timeConsumed;

    //number of words in query
    data >> wordCount;

    //read word statistics
    for (unsigned int i=0 ; i<wordCount ; i++)
    {
        Sphinx::WordStatistics_t entry;
        std::string word;

        data >> word;
        data >> entry.docsHit;
        data >> entry.totalHits;

        response.word[word] = entry;
    }//for
}//konec fce

void parseResponse_v0_9_7(Sphinx::Query_t &data, Sphinx::Response_t &response)
{
    uint32_t matchCount;
    uint32_t wordCount;
    uint32_t fieldCount;
    uint32_t attrCount;

    response.clear();

    //read fields
    if (!(data >> fieldCount))
        throw Sphinx::MessageError_t(
                         "Can't read any data. Probably zero-length response.");

    for (unsigned int i=0 ; i<fieldCount ; i++)
    {
        std::string name;
        data >> name;
        response.field.push_back(name);
    }//for

    //read attributes
    data >> attrCount;

    for (unsigned int i = 0 ; i<attrCount ; i++)
    {
        std::string name;
        uint32_t type;

        data >> name;
        data >>type;

        response.attribute.push_back(std::make_pair(name, type));
    }//for

    //number of entries to fetch
    if (!(data >> matchCount))
        throw Sphinx::MessageError_t(
                         "Error parsing response.");

    for (unsigned int i=0 ; i<matchCount ; i++)
    {
        Sphinx::ResponseEntry_t entry;
        data >> entry.documentId;
        data >> entry.weight;
        entry.groupId = entry.timestamp = 0;

        //read attribute values
        for (Sphinx::AttributeTypes_t::iterator attr
             = response.attribute.begin() ; attr!=response.attribute.end() ;
             attr++)
        {
            uint32_t value;
            data >> value;
            entry.attribute.insert(std::make_pair(attr->first, value));
        }//for

        response.entry.push_back(entry);
    }//for

    //uint32_t totalGot, totalFound, timeConsumed;

    data >> response.entriesGot;
    data >> response.entriesFound;
    data >> response.timeConsumed;

    //number of words in query
    data >> wordCount;

    //read word statistics
    for (unsigned int i=0 ; i<wordCount ; i++)
    {
        Sphinx::WordStatistics_t entry;
        std::string word;

        data >> word;
        data >> entry.docsHit;
        data >> entry.totalHits;

        response.word[word] = entry;
    }//for
}//konec fce

void Sphinx::Client_t::query(const std::string& query,
                             const SearchConfig_t &attrs,
                             Response_t &response)
{
    Query_t data, request;
    SocketCloser_t socketCloser(socket_d);
    unsigned short responseVersion;

    data.convertEndian = true;
    request.convertEndian = true;

    //-------------------build query---------------
    switch (attrs.commandVersion)
    {
        case VER_COMMAND_SEARCH_0_9_6:
            buildQuery_v0_9_6(query, attrs, data);
            break;

        case VER_COMMAND_SEARCH_0_9_7:
            buildQuery_v0_9_7(query, attrs, data);
            break;

        case VER_COMMAND_SEARCH_0_9_7_1:
            buildQuery_v0_9_7_1(query, attrs, data);
            break;
    }//switch

    //prepend header
    request << (unsigned short) SEARCHD_COMMAND_SEARCH;
    request << (unsigned short) attrs.commandVersion;
    request << (uint32_t) data.getLength();
    request << data;

    //if not connected, connect to the server
    connect();

    //send request
    sendData(request);

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

    //--------- parse response -------------------
    switch (attrs.commandVersion)
    {
        case VER_COMMAND_SEARCH_0_9_6:
            parseResponse_v0_9_6(data, response);
            response.commandVersion = VER_COMMAND_SEARCH_0_9_6;
            break;

        case VER_COMMAND_SEARCH_0_9_7:
        case VER_COMMAND_SEARCH_0_9_7_1:
            parseResponse_v0_9_7(data, response);
            response.commandVersion = VER_COMMAND_SEARCH_0_9_7;
            break;

        default:
            throw Sphinx::MessageError_t(
                    "Invalid response version (0x101, 0x104 supported).");
            break;
    }//switch
}//konec fce




void sphinxClientDummy()
{ }


Sphinx::Filter_t::Filter_t()
    : attrName(""), excludeFlag(false)
{}

Sphinx::RangeFilter_t::RangeFilter_t(std::string attrName, uint32_t minValue,
                                     uint32_t maxValue, bool excludeFlag)
                : minValue(minValue), maxValue(maxValue) 
{
    this->attrName = attrName;
    this->excludeFlag = excludeFlag;
}

void Sphinx::RangeFilter_t::dumpToBuff(Sphinx::Query_t &data) const
{
        data << attrName;
        data << (uint32_t) 0
             << (uint32_t) minValue
             << (uint32_t) maxValue;
        data << (uint32_t) excludeFlag;
}

Sphinx::EnumFilter_t::EnumFilter_t(std::string attrName,
                                   const IntArray_t &values,
                                   bool excludeFlag)
        : values(values)
{
        this->attrName = attrName;
        this->excludeFlag = excludeFlag;
}

void Sphinx::EnumFilter_t::dumpToBuff(Sphinx::Query_t &data) const
{
        data << attrName;
        data << (uint32_t) values.size();
        for (Sphinx::IntArray_t::const_iterator val = values.begin();
             val != values.end(); val++)
        {
            data << (uint32_t)(*val);
        }
        data << (uint32_t) excludeFlag;
};








