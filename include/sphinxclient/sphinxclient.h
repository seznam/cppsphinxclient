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
 * SphinxClient header file - communication library for the sphinx
 * search server
 *
 * AUTHOR
 * Jan Kirschner <jan.kirschner@firma.seznam.cz>
 *
 * HISTORY
 * 2006-12-11 (jan.kirschner)
 *            First draft.
 */

//! @file sphinxclient.h

#ifndef __SPHINXAPI_H__
#define __SPHINXAPI_H__

#include <sphinxclient/sphinxclientquery.h>
#include <sphinxclient/value.h>
#include <sphinxclient/filter.h>

#include <string>
#include <vector>
#include <map>
#include <stdint.h>

#include "error.h"

namespace Sphinx
{

//------------------------------------------------------------------------------
// Private data structures forward declarations
//------------------------------------------------------------------------------

class Query_t;
class Client_t;

//------------------------------------------------------------------------------
// --------------------- configuration -----------------------------------------


/** @brief Sphinx matching mode
  */

enum MatchMode_t { //! @brief document must contain all specified words
                   SPH_MATCH_ALL = 0,
                   //! @brief document must contain at least one word
                   SPH_MATCH_ANY = 1,
                   //! @brief words in document must be in the same phrase
                   SPH_MATCH_PHRASE = 2,
                   //! @brief supports logical operators &, |, ... in query
                   SPH_MATCH_BOOLEAN = 3,
                   //! @brief supports query syntax like "@title ahoj | @content
                   //!        lidi"
                   SPH_MATCH_EXTENDED = 4,
                   //! @brief fullscan - return all entries
                   SPH_MATCH_FULLSCAN = 5,
                   //! @brief temporary extended mode, will be removed in future
                   //         versions of sphinx
                   SPH_MATCH_EXTENDED2 = 6,
                   //! @brief szn sorting mode
                   SPH_MATCH_SZN = 7};

/** @brief Sphinx result sorting mode
  */

enum SortMode_t { SPH_SORT_RELEVANCE = 0, //!< @brief relevance
                  SPH_SORT_DATE_DESC = 1, //!< @brief date descending
                  //SPH_SORT_ATTR_DESC = 1,
                  SPH_SORT_DATE_ASC = 2,  //!< @brief date ascending
                  //SPH_SORT_ATTR_ASC = 2,
                  SPH_SORT_TIME_SEGMENTS = 3,
                  //! @brief supports sorting by multiple columns (@weight ASC)
                  SPH_SORT_EXTENDED = 4,
                  //! @brief sorting mode, which supports expressions
                  SPH_SORT_EXPR = 5,
                  SPH_SORT_SZN = 100
                };

/** @brief Sphinx fulltext ranking mode. Valid only for extended2 matching mode
  */

enum RankingMode_t { //! @brief default mode, phrase proximity major factor and
                     //         BM25 minor one
                     SPH_RANK_PROXIMITY_BM25 = 0,
                     //! @brief statistical mode, BM25 ranking only (faster but
                     //         worse quality)
                     SPH_RANK_BM25 = 1,
                     //! @brief no ranking, all matches get a weight of 1
                     SPH_RANK_NONE = 2,
                     //! @brief simple word-count weighting, rank is a weighted
                     //         sum of per-field keyword occurence counts
                     SPH_RANK_WORDCOUNT = 3
                   };

/** @brief Result attribute types
 *
 *  Types of attributes returned in result of search command v. 0x104
 */

enum AttributeType_t { SPH_ATTR_INTEGER = 1,     //!< @brief int attribute type
                       SPH_ATTR_TIMESTAMP = 2,   //!< @brief timestamp attribute
                       SPH_ATTR_ORDINAL = 3,     //!< @brief 
                       SPH_ATTR_BOOL = 4,        //!< @brief boolean attribute
                       SPH_ATTR_FLOAT = 5,       //!< @brief float attribute
                       SPH_ATTR_MULTI = 0x40000000 }; //!< @brief

/** @brief Grouping functions
 *
 *  specifies grouping granularity
 */

enum GroupFunction_t { SPH_GROUPBY_DAY = 0,
                       SPH_GROUPBY_WEEK = 1,
                       SPH_GROUPBY_MONTH = 2,
                       SPH_GROUPBY_YEAR = 3,
                       SPH_GROUPBY_ATTR = 4,
                       SPH_GROUPBY_ATTRPAIR = 5 };


/** @brief Connection configuration object
 *
 *  keeps configuration data needed to connect to the searchd server, such as
 *  host, port, timeouts and keepalive flag
 */

class ConnectionConfig_t
{
public:
    std::string host;
    unsigned short port;

    bool keepAlive;
    int32_t connectTimeout;
    int32_t readTimeout;
    int32_t writeTimeout;

    ConnectionConfig_t(const std::string &host = "localhost",
                       unsigned short port = 3312,
                       bool keepAlive = true,
                       int32_t connectTimeout = 5000,
                       int32_t readTimeout = 3000,
                       int32_t writeTimeout = 3000);
};


/** @brief geographical anchor point
 */

struct GeoAnchorPoint_t {
    GeoAnchorPoint_t() {}
    GeoAnchorPoint_t(const std::string &laAtt, const std::string &loAtt,
                     float lattitude, float longitude)
        : lattitudeAttributeName(laAtt), longitudeAttributeName(loAtt),
          lattitude(lattitude), longitude(longitude) {}

    //! @brief names of attributes, where lattitude and longitude are stored
    std::string lattitudeAttributeName, longitudeAttributeName;
    //! @brief lattitude and longitude to sort by
    float lattitude, longitude;
};

/** @brief Search query configuration object
 *
 *  holds information about sorting, grouping and filtering results
 *  and the query version. Versions supported are 0x101 and 0x104
 *  as in enum SearchCommandVersion_t.
 */

struct SearchConfig_t
{
    SearchConfig_t(SearchCommandVersion_t cmdVer = VER_COMMAND_SEARCH_0_9_8);

    /** @brief Adds range attribute filter to search config
      * 
      * @param excludeFlag invert filter (use values outside spcified range)
      */  
    void addRangeFilter(std::string attrName, uint32_t minValue,
                  uint32_t maxValue, bool excludeFlag=false);
    /** @brief Adds enumeration filter to search config
      * 
      * @param excludeFlag invert filter (use all values except enumerated)
      */  
    void addEnumFilter(std::string attrName, const IntArray_t &values,
                  bool excludeFlag=false);
    /** @brief Adds float range attribute filter to search config
      * 
      * @param excludeFlag invert filter (use values outside spcified range)
      */  
    void addFloatRangeFilter(std::string attrName, float minValue,
                   float maxValue, bool excludeFlag=false);

    uint32_t msgOffset; //!< @brief specifies, how many matches to skip
    uint32_t msgLimit;  //!< @brief specifies, how many matches to fetch

    uint32_t minId, maxId;  //!< @brief set document id limits

    //----- pouziva se pouze ve verzi query 101 -----
    //! @brief set minimum and maximum of timestamp
    uint32_t minTimestamp, maxTimestamp;
    //! @brief set minimum and maximum of group id
    uint32_t minGroupId, maxGroupId;
    //-----------------------------------------------

    //! @brief query matching mode (default SPH_MATCH_ALL)
    MatchMode_t matchMode;
    //! @brief result sorting mode (default SPH_SORT_RELEVANCE)
    SortMode_t sortMode;
    //! @brief fulltext ranking mode (default SPH_RANK_PROXIMITY_BM25)
    //         since version 113
    RankingMode_t rankingMode;

    //! @brief per field weights
    IntArray_t weights;

    //----- pouze ve verzi 101 -----
    //! @brief groups of documents to search in
    IntArray_t groups;
    //------------------------------

    
    //----- od verze query 104 -----
    std::string sortBy;          //!< @brief which columns to sort the result by
    //! @brief filters for specified attributes. Range (Min, max) and list of
    //!        allowed values (enum). maxValue and minValue must have same size.
    std::map<std::string, uint32_t> minValue;
    std::map<std::string, uint32_t> maxValue;
    std::map<std::string, IntArray_t > filter;
    std::string groupBy;        //!< @brief which columns to group the result by
    //! @brief group function - fetch the match with greatest relevance per
    //!        group
    GroupFunction_t groupFunction;
    int maxMatches;  //!< @brief maximum matches to search for
    //------------------------------

    //----- od verze query 107 -----
    std::string groupSort; //!< $brief group-by sorting clause (to sort groups in result set with)
    std::vector<Filter_t *> filters; //!< $brief array to hold attribute filter (range, enum)

    //! @brief Command version - allowed values are 0x101, 0x104, 0x107 and 0x113
    SearchCommandVersion_t commandVersion;
    //! @brief Index file names to search in
    std::string indexes;

    //----- od verze query 113 -----
    //! @brief stop searching after cutoff matches (default 0 - disabled)
    int32_t searchCutOff;
    //! @brief distributed search retry count and delay
    int32_t distRetryCount, distRetryDelay;
    //! @brief count-distinct attribute (its name) for group-by query
    std::string groupDistinctAttribute;
 
    //! @brief anchor points to do distance sorting
    std::vector<GeoAnchorPoint_t> anchorPoints;

    //! @brief per index weights (map of index name to index weight)
    std::map<std::string, uint32_t> indexWeights;
    //! @brief max query duration, milliseconds (default is 0, do not limit)
    int32_t maxQueryTime;
    //! @brief per field weights (map of field name to field weight)
    std::map<std::string, uint32_t> fieldWeights;
    //! @brief query comment
    std::string queryComment;
};

// ------------------------ Client_t -----------------------

// ------------ response data structures --------------

/** @brief One response match entry
  *
  * @see SphinxClient_t::Response_t
  */

struct ResponseEntry_t
{
    uint64_t documentId;   //!< @brief database ID of the document
    uint32_t groupId;      //!< @brief group ID (only v. 101)
    uint32_t timestamp;    //!< @brief creation/modification time (v. 101)
    uint32_t weight;       //!< @brief matching weight (relevance)
    //! @brief attribute values (only since v. 104)
    std::map<std::string, Value_t> attribute;

    ResponseEntry_t()
        : documentId(0), groupId(0), timestamp(0), weight(0) {}
};//struct

/** @brief Searched word statistics returned by searchd
  *
  * @see SphinxClient_t::Response_t
  */

struct WordStatistics_t
{
    uint32_t docsHit;   //!< @brief number of documents containing word
    uint32_t totalHits; //!< @brief number of word occurences
};//struct

typedef std::vector<std::pair<std::string, uint32_t> > AttributeTypes_t;

/** @brief Search query result data
  *
  * @see SphinxClient_t::query
  */

struct Response_t
{
    //! @brief list of searched fields (only v. 104)
    std::vector<std::string> field;

    //! @brief list of returned attributes and their types (only v. 104)
    AttributeTypes_t attribute;

    //! @brief list of matches found and returned by the searchd
    std::vector<ResponseEntry_t> entry;

    //! @brief list of word statistics
    std::map<std::string, WordStatistics_t> word;

    // global statistics
    uint32_t entriesGot;    //!< @brief total number of matches found
    uint32_t entriesFound;  //!< @brief total number of documents matched
    uint32_t timeConsumed;  //!< @brief time consumed by the query
    uint32_t use64bitId;    //!< @brief whether to use 32 or 64 bits for doc id

    SearchCommandVersion_t commandVersion; //!< @brief search command version

    void clear();
};//struct

/** @brief multi query data structure
  *
  * This class provides methods and storage for creating multi-queries.
  */

class MultiQuery_t
{
protected:
    SearchCommandVersion_t commandVersion;
    Query_t queries;
    int queryCount;

public:
    //! @brief Constructor, which sets multi-query command version
    MultiQuery_t(SearchCommandVersion_t cmdVersion = VER_COMMAND_SEARCH_0_9_8);

    //! @brief resets query data and multi-query command version
    void initQuery(SearchCommandVersion_t commandVersion);

    /** @brief adds a query to multi-query
      *
      * Adds a query to multi-query and checks command version against
      * multi-query command version
      *
      * @param query words in string to search for
      * @param queryAttr query attributes
      * @throws ClientUsageError_t
      * @see Client_t::query
      * @see SearchConfig_t
      */
    void addQuery(const std::string& query, const SearchConfig_t &queryAttr);

    int getQueryCount() const; //!< @brief returns query count
    const Query_t &getQueries() const; //!< @brief returns concatenated queries
    //! @brief returns command version
    SearchCommandVersion_t getCommandVersion() const;
};//class

/** @brief structure that holds attribute names and their values in
  *        documents to be updated
  */
struct AttributeUpdates_t
{
    UpdateCommandVersion_t commandVersion;
    std::vector<std::string> attributes;
    std::map<uint64_t, std::vector<Value_t> > values;

    //! @brief constructor, default command version is VER_COMMAND_UPDATE_0_9_8
    AttributeUpdates_t();

    //! @brief sets attribute list, clears value list
    void setAttributeList(const std::vector<std::string> &attr
                                             = std::vector<std::string>());
    //! @brief adds attribute to attribute list, clears value list
    void addAttribute(const std::string &);
    
    /** @brief Add document to update attributes at and their values in vector.
      * @param id document id
      * @param vals vector of attribute values, size must match attribute count
      * @throws ClientUsageError_t when attribute count doesn't match value count
      */
    void addDocument(uint64_t id, const std::vector<Value_t> &vals);
    
    /** @brief Add a document to update attributes at and their values
      *        if type t in variable-length argument list. Doesn't check
      *        the argument count.
      * @param id document id
      * @param t type of following arguments (uint32_t and float supported)
      * @param ... argument values of previously defined type
      */
    void addDocument(uint64_t id, ValueType_t t, ...);
   
    //! @brief set update command version
    void setCommandVersion(UpdateCommandVersion_t v);
};//struct



// ------------ main class --------------

/** @brief Communication interface to the Sphinx searchd
  *
  * this is the main class. It contains communication methods and
  * response structure
  */

class Client_t
{
public:
    Client_t(const ConnectionConfig_t &connectionSettings);
    ~Client_t();

    //! @brief close connection to the searchd
    void close();

    /** @brief send a search query to the searchd
      *
      * Sends a search query to the sphinx searchd and fills the response
      * structure. On exception the content of the response is undefined,
      * any previous structure content is invalidated anyway.
      *
      * @param query list of words to search for
      * @param queryAttr query configuration
      * @param response output parameter - response structure
      * @throws SphinxClientError_t on any communication or parsing error
      */
    void query(const std::string& query,
               const SearchConfig_t &queryAttr,
               Response_t &response);

    /** @brief send a search multi-query to the searchd
      *
      * Sends a search multi-query to the sphinx searchd and fills the response
      * structure vector. On exception the content of the response is undefined,
      * any previous structure content is invalidated anyway.
      *
      * @param query initialized MultiQuery_t object witg queries added
      * @param response output parameter - response structure
      * @throws SphinxClientError_t on any communication or parsing error
      * @see MultiQuery_t
      */
    void query(const MultiQuery_t &query, std::vector<Response_t> &response);

    
    /** @brief send a update command to searchd
      *
      * Sends a command to update attributes in sphinx searchd loaded
      * indexes. Works only for externally stored docinfo. Currently
      * supported attribute types are uint32_t and float.
      *
      * @param index name of index to be updated. Does not support wildcard (*)
      * @param at list of attributes and their values to update in documents
      * @throws SphinxClientError_t on any communication or parsing error
      * @see AttributeUpdates_t
      */
    void updateAttributes(const std::string &index, const AttributeUpdates_t &at);

    void waitSocketReadable(const std::string &stage); 

protected:
    // ------- connect to / disconnect from the server
    /** @brief create connection to the searchd and check protocol version
      * @throws SphinxClientError_t on connection error or version mismatch
      */

    // -------- communication methods ----------
    void receiveData(Query_t &buff);
    int receiveResponse(Query_t &buff, unsigned short &version);
    void sendData(const Query_t &buff);
    void connect();

    unsigned short processRequest(const Query_t &query, Query_t &response);
    // -------- connection settings -----------------
    ConnectionConfig_t connection;

    int socket_d;
};//class

}//namespace

extern "C" {
    void sphinxClientDummy();
}

#endif

