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
#include <sphinxclient/globals_public.h>

#include <sstream>
#include <string>
#include <vector>
#include <list>
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
class Filter_t;

//------------------------------------------------------------------------------
#define DEFAULT_CONNECT_RETRIES 1
#define CONNECT_RETRY_WAIT_DEFAULT_MS 300
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
                     SPH_RANK_WORDCOUNT = 3,
                     //! @brief only phrase procimity relevance
                     SPH_RANK_PROXIMITY = 4,
                     //! @brief matchAny mode relevance
                     SPH_RANK_MATCHANY = 5,
                     //! @brief relevance is 32bit mask with bit for each field
                     //         set to 1 when a keyword is located there
                     SPH_RANK_FIELDMASK = 6
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
                       SPH_ATTR_BIGINT = 6,      //!< @brief uint64 attribute
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
 *  host, port, timeouts, keepalive flag and connection retry params.
 *  WARNING: setting of keepalive is currently ignored (will be implemented
 *           later)
 */

class ConnectionConfig_t
{
    /// connection config data (hidden from interface, ABI stability)
    struct PrivateData_t;
    /// pointer to the connection config data
    PrivateData_t *d;
public:
    /* @brief Contructor
     * @param host host where the searchd runs
     * @param port port what the searchd listens on
     * @param keepAlive ignored, currently not supported
     * @param connectTimeout max wait time for connection setup
     * @param readTimeout max wait time on read from socket
     *        (resets when something has been read)
     * @param writeTimeout max wait time on write from socket
     *        (resets when something has been written)
     * @param connectRetriesCount nr of connect attempts
     * @param connectRetryWait delay after failed connect attempt
     */
    ConnectionConfig_t(const std::string &host = "localhost",
                       unsigned short port = 3312,
                       bool keepAlive = true,
                       int32_t connectTimeout = 1000,
                       int32_t readTimeout = 3000,
                       int32_t writeTimeout = 3000,
                       int32_t connectRetriesCount = DEFAULT_CONNECT_RETRIES,
                       int32_t connectRetryWait = CONNECT_RETRY_WAIT_DEFAULT_MS);
    /* @brief copy constructor
     */
    ConnectionConfig_t(const ConnectionConfig_t &from);
    /* @brief assignment operator
     */
    ConnectionConfig_t &operator=(const ConnectionConfig_t &from);

    ~ConnectionConfig_t();
    /* @brief getters and setters
     */
    const std::string &getHost() const;
    unsigned short getPort() const;
    bool getKeepAlive() const;
    int32_t getConnectTimeout() const;
    int32_t getReadTimeout() const;
    int32_t getWriteTimeout() const;
    int32_t getConnectRetriesCount() const;
    int32_t getConnectRetryWait() const;
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
    /** Copy constructor
      */
    SearchConfig_t(const SearchConfig_t &from);

    /** Assignment op.
      */
    SearchConfig_t &operator= (const SearchConfig_t &from);

    /** Makes deep copy
      */
    void makeCopy(const SearchConfig_t &from);

    /* Clears allocated data
     */
    void clear();

    /** desctructor
      */
    ~SearchConfig_t();

    SearchConfig_t(SearchCommandVersion_t cmdVer = VER_COMMAND_SEARCH_0_9_9);

    /** @brief Adds range attribute filter to search config
      * 
      * @param excludeFlag invert filter (use values outside spcified range)
      */
    void addRangeFilter(std::string attrName, uint64_t minValue,
                  uint64_t maxValue, bool excludeFlag=false);
    /** @brief Adds enumeration filter to search config
      * 
      * @param excludeFlag invert filter (use all values except enumerated)
      */
    void addEnumFilter(std::string attrName, const Int64Array_t &values,
                  bool excludeFlag=false);
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

    /** @brief Attribute value override for specified document.
     *  @param attrName name of attribute to override
     *  @param attrType overriden attribute type
     *  @param docId document id where to override the attribute
     *  @param value new value of the attribute
     */
    void addAttributeOverride(const std::string &attrName,
                              AttributeType_t attrType,
                              uint64_t docId, const Value_t &value);
    /** @brief Attribute value override for specified documents.
     *  @param attrName name of attribute to override
     *  @param attrType overriden attribute type
     *  @param values values of specified attribute by document IDs
     */
    void addAttributeOverride(const std::string &attrName,
                              AttributeType_t attrType,
                              const std::map<uint64_t, Value_t> &values);

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

    //! @brief Command version - allowed values are 0x101, 0x104, 0x107, 0x113, 0x116
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

    //----- od verze query 116 -----
    //! @brief select columns to fetch in SQL-like select syntax. Available
    //         aggregation functions are MIN, MAX, SUM, AVG. AS is required
    std::string selectClause;
    //! @brief attribute overrides - map attrName->(attrType, map docId->attrValue)
    std::map<std::string,
             std::pair<AttributeType_t, std::map<uint64_t, Value_t> > >
                 attributeOverrides;
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

/** @brief keywords request result data for one word query */
struct KeywordResult_t {
    //! @brief tokenized word form
    std::string tokenized;
    //! @brief normalized word form
    std::string normalized;
    //! @brief word statistics - total hits/docs
    WordStatistics_t statistics;
};

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
    /** @brief Constructor of multiquery
     * 
     * @param cv cmdVersion command version of multiquery
     */
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


// ---------------------------- MultiqueryOpt_t --------------------------------

/* @brief Sphinx::Query_t with possibility identify whether the query is 
 *        composable into multiquery with another query
 */
class SourceQuery_t {
public:
    /* Constructor
     * 
     * @param query query string
     * @param queryAttr sorting, grouping, etc...
     * @param seqNo incoming sequence number of query
     */
    SourceQuery_t(const std::string &query, const SearchConfig_t &queryAttr,
                  int seqNo);
    /* @brief compute hash from filters, etc. that must be same within one
     *        efficient multiquery
     * @return hash
     */ 
    const std::string &getHash() const {return hash;}
    /* @brief get sequence nr. of input query
     * @return sequence nr (starting grom 0.)
     */
    int getInputSeqNo() const {return inputSeqNo;}
    /* @brief get encapsulated Sphinx::Query_t
     * @return the query
     */
    const Query_t &getQuery() const {return serializedQuery;}

private:
    /// serialized query object
    Query_t serializedQuery;
    /// hash identifies whether the SourceQuery could be joined with other one
    /// into sphinx multiquery
    std::string hash;
    /// sequence number (starting from 0.)
    int inputSeqNo;
};


/** @brief Optimisation-enabled multi query data structure
  *
  * This class provides methods and storage for creating multi-queries
  * enabled for efficient processing
  *
  * Sphinx search daemon sometimes doesn't process multiqueries 
  * efficient way. Efficient processing means that matching stage of query
  * is done only once and there are number of sorters or groupers for
  * each specific query. When multiquery is consisted of queries
  * impossible to have the same matching stage, queries are processed
  * one by one separately and processing takes long time to finish.
  * 
  * This version of multiquery analyses input queries and group them
  * to groups efficient for sphinx one-pass multi-query processing.
  * method optimise() do that. If calling optimise is ommited,
  * multiquery is sent in traditional manner as one big multiquery.
  * 
  * Query groups are then sent in Clinent_t::query() to sphinx searchd
  * simultaneously by passing them to QueryMachine_t. After collecting
  * replies from server responses are parsed and ordered as they came
  * to server.
  *
  * @see QueryMachine_t
  * @see Client_t
  */
class MultiQueryOpt_t
{
public:
    /** @brief Constructor of multiquery
     * 
     * @param cv cmdVersion command version of multiquery
     */
    MultiQueryOpt_t(SearchCommandVersion_t cmdVersion = VER_COMMAND_SEARCH_0_9_8);

    /* @brief groups input queries into groups efficient for sphinx multiquery
     *        processing.
     *
     * Optimisation is disabled for command version < 0.9.8.
     */
    void optimise();

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

    friend class Sphinx::Client_t;
protected:
    SearchCommandVersion_t commandVersion;
    /// source queries ordered as they come from client
    std::list<SourceQuery_t> sourceQueries;
    /// queries sorted by hash (queries with same hash are suitable
    /// for efficient processing within one sphinx multiquery)
    std::vector<const SourceQuery_t *> sortedQueries;
    /// maps <source seqNo -> sortedQuery position in order to order responses
    std::vector<std::pair<int, int> > responseIndex;
    /// groupQuery specification
    /// indexes of first queries within group queries in sortedQueries
    std::vector<int> groupQueries;

    //! @brief resets query data and multi-query command version
    void initQuery(SearchCommandVersion_t commandVersion);

    /* @brief get query group concatenated to one multiquery sphinx request
     * @param groupIndex index of query group
     * @return multiquery request
     */
    Sphinx::Query_t getGroupQuery(size_t groupIndex) const;

    /* @brief get count of query groups, that are efficient to process by
     *        sphinx multiquery mechanism
     * @return query group count
     */
    size_t getGroupQueryCount() const;

    //!< @brief returns input query count
    int getQueryCount() const; 
    
    /** @brief get count of input queries in query group
      * @param groupIndex index of queryGroup
      * @return count of input queries in group
      */
    size_t getQueryCountAtGroup(size_t groupIndex) const;

    /** @brief Find out where to place response of query
      *        specified by position at sorted index
      * @param sortedIndex position of query at sorted index      
      * @return index where to place the response
      */
    size_t getResponseIndex(size_t sortedIndex) const;

    //! @brief returns command version
    SearchCommandVersion_t getCommandVersion() const;
};//class

// ------------------------ Attribute updates ---------------------------

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

    /** @brief send a search multi-query optimised to the searchd
      *
      * Sends an optimized multi-query to the sphinx searchd and fills the response
      * structure vector. On exception the content of the response is undefined,
      * any previous structure content is invalidated anyway.
      *
      * Multiquery divides queries into similar multiquery-efficient groups. These
      * groups are sent simoultanously in parrallel connections to searchd.
      * @see QueryMachine_t
      *
      * @param query initialized MultiQueryOpt_t object witg queries added
      * @param response output parameter - response structure
      * @throws SphinxClientError_t on any communication or parsing error
      * @see MultiQueryOpt_t
      */
    void query(const MultiQueryOpt_t &query, std::vector<Response_t> &response);
    
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

    /** @brief send a keywords request to searchd
      *
      * Send a request to analyze query words. Run tokenizer and normalization.
      *
      * @param index name of index to use tokenizer settings from. Does not
      *              support wildcards (*).
      * @param query query to analyze.
      * @param getWordStatistics when true, fetch also total word docs/hits
      * @returns tokenized and normalized words
      */
    std::vector<KeywordResult_t> getKeywords(
        const std::string &index,
        const std::string &query,
        bool getWordStatistics = false);

protected:
    // -------- connection settings -----------------
    ConnectionConfig_t connection;
};//class

}//namespace

extern "C" {
    void sphinxClientDummy();
}

#endif

