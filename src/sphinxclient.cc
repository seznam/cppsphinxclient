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
#include <filter.h>

#include <sstream>
#include <algorithm>

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <poll.h>

#include <stdarg.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <unistd.h>

#include "querymachine.h"
#include "timer.h"


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

struct Sphinx::ConnectionConfig_t::PrivateData_t
{
    std::string host;
    unsigned short port;

    bool keepAlive;
    int32_t connectTimeout;
    int32_t readTimeout;
    int32_t writeTimeout;
    int32_t connectRetriesCount;
    int32_t connectRetryWait;

    void makeCopy(const Sphinx::ConnectionConfig_t::PrivateData_t &from)
    {
        host = from.host;
        port = from.port;
        keepAlive = from.keepAlive;
        connectTimeout = from.connectTimeout;
        readTimeout = from.readTimeout;
        writeTimeout = from.writeTimeout;
        connectRetriesCount = from.connectRetriesCount;
        connectRetryWait = from.connectRetryWait;
    }
};

Sphinx::ConnectionConfig_t::ConnectionConfig_t(const std::string &host,
        unsigned short port, bool keepAlive, int32_t connectTimeout,
        int32_t readTimeout, int32_t writeTimeout,
        int32_t connectRetriesCount, int32_t connectRetryWait)
{
    d = new PrivateData_t;
    d->host = host;
    d->port = port;
    d->keepAlive = keepAlive;
    d->connectTimeout = connectTimeout;
    d->readTimeout = readTimeout;
    d->writeTimeout = writeTimeout;
    d->connectRetriesCount = connectRetriesCount;
    d->connectRetryWait = connectRetryWait;
}
    
Sphinx::ConnectionConfig_t::ConnectionConfig_t(
        const Sphinx::ConnectionConfig_t &from)
{
    d = new PrivateData_t;
    d->makeCopy(*from.d);
}

Sphinx::ConnectionConfig_t &Sphinx::ConnectionConfig_t::operator=(
    const Sphinx::ConnectionConfig_t &from)
{
    if (this != &from)
    {
        d->makeCopy(*from.d);
    }
    return *this;
}

Sphinx::ConnectionConfig_t::~ConnectionConfig_t()
{
    delete d;
}

const std::string &Sphinx::ConnectionConfig_t::getHost() const
{
    return d->host;
}
unsigned short Sphinx::ConnectionConfig_t::getPort() const
{
    return d->port;
}
bool Sphinx::ConnectionConfig_t::getKeepAlive() const
{
    return d->keepAlive;
}
int32_t Sphinx::ConnectionConfig_t::getConnectTimeout() const
{
    return d->connectTimeout;
}
int32_t Sphinx::ConnectionConfig_t::getReadTimeout() const
{
    return d->readTimeout;
}
int32_t Sphinx::ConnectionConfig_t::getWriteTimeout() const
{
    return d->writeTimeout;
}
int32_t Sphinx::ConnectionConfig_t::getConnectRetriesCount() const
{
    return d->connectRetriesCount;
}
int32_t Sphinx::ConnectionConfig_t::getConnectRetryWait() const
{
    return d->connectRetryWait;
}

//------------------------------------------------------------------------------


namespace Sphinx {
    struct SearchConfig_t::Dptr_t
    {
        void makeCopy(const Dptr_t &from)
        {
            msgOffset = from.msgOffset;
            msgLimit = from.msgLimit;
            matchMode = from.matchMode;
            sortMode = from.sortMode;
            rankingMode = from.rankingMode;
            sortBy = from.sortBy;
            groupBy = from.groupBy;
            groupFunction = from.groupFunction;
            maxMatches = from.maxMatches;
            groupSort = from.groupSort;
            commandVersion = from.commandVersion;
            indexes = from.indexes;
            searchCutOff = from.searchCutOff;
            distRetryCount = from.distRetryCount;
            distRetryDelay = from.distRetryDelay;
            groupDistinctAttribute = from.groupDistinctAttribute;
            anchorPoints = from.anchorPoints;
            indexWeights = from.indexWeights;
            maxQueryTime = from.maxQueryTime;
            fieldWeights = from.fieldWeights;
            queryComment = from.queryComment;
            selectClause = from.selectClause;
            attributeOverrides = from.attributeOverrides;
            /* handle filters*/
            for (std::vector<Filter_t *>::const_iterator i = from.filters.begin();
                i != from.filters.end(); ++i)
            {
                filters.push_back((*i)->clone());
            }
            rankingExpr = from.rankingExpr;
        }

        void clear() {
            // handle filters
            for (std::vector<Filter_t *>::const_iterator i = filters.begin();
                i != filters.end(); i++)
            {
                delete *i;
            }
        }

        Dptr_t(SearchCommandVersion_t cmdVer) :
            msgOffset(0), msgLimit(20),
            matchMode(SPH_MATCH_ALL), sortMode(SPH_SORT_RELEVANCE),
            rankingMode(SPH_RANK_PROXIMITY_BM25),
            sortBy(""), groupBy(""), groupFunction(SPH_GROUPBY_DAY), maxMatches(1000),
            groupSort("@group desc"), commandVersion(cmdVer), indexes("*"),
            searchCutOff(0), distRetryCount(0), distRetryDelay(0), maxQueryTime(0),
            selectClause("*")
        {}

        Dptr_t(const Dptr_t &from) {
            makeCopy(from);
        }

        ~Dptr_t() {
            clear();
        }

        Dptr_t &operator = (const Dptr_t &from) {
            if (&from != this) {
                clear();
                makeCopy(from);
            }
            return *this;
        }


        uint32_t msgOffset; //!< @brief specifies, how many matches to skip
        uint32_t msgLimit;  //!< @brief specifies, how many matches to fetch

        //! @brief query matching mode (default SPH_MATCH_ALL)
        MatchMode_t matchMode;
        //! @brief result sorting mode (default SPH_SORT_RELEVANCE)
        SortMode_t sortMode;
        //! @brief fulltext ranking mode (default SPH_RANK_PROXIMITY_BM25)
        //         since version 113
        RankingMode_t rankingMode;

        std::string sortBy;          //!< @brief which columns to sort the result by
        std::string groupBy;        //!< @brief which columns to group the result by
        //! @brief group function - fetch the match with greatest relevance per
        //!        group
        GroupFunction_t groupFunction;
        int maxMatches;  //!< @brief maximum matches to search for

        std::string groupSort; //!< $brief group-by sorting clause (to sort groups in result set with)
        std::vector<Filter_t *> filters; //!< $brief array to hold attribute filter (range, enum)

        //! @brief Command version - allowed values are 0x101, 0x104, 0x107, 0x113, 0x116
        SearchCommandVersion_t commandVersion;
        //! @brief Index file names to search in
        std::string indexes;

        //----- od verze query 113 -----
        //! @brief stop searching after cutoff matches (default 0 - disabled)
        uint32_t searchCutOff;
        //! @brief distributed search retry count and delay
        uint32_t distRetryCount, distRetryDelay;
        //! @brief count-distinct attribute (its name) for group-by query
        std::string groupDistinctAttribute;

        //! @brief anchor points to do distance sorting
        std::vector<GeoAnchorPoint_t> anchorPoints;

        //! @brief per index weights (map of index name to index weight)
        std::map<std::string, uint32_t> indexWeights;
        //! @brief max query duration, milliseconds (default is 0, do not limit)
        uint32_t maxQueryTime;
        //! @brief per field weights (map of field name to field weight)
        std::map<std::string, uint32_t> fieldWeights;
        //! @brief query comment
        std::string queryComment;

        //! @brief select columns to fetch in SQL-like select syntax. Available
        //         aggregation functions are MIN, MAX, SUM, AVG. AS is required
        std::string selectClause;
        //! @brief attribute overrides - map attrName->(attrType, map docId->attrValue)
        AttributeOverrides_t attributeOverrides;

        //! @brief Expression for match ranking. Used with SPH_RANK_EXPR only.
        std::string rankingExpr;
    };
}//namespace



Sphinx::SearchConfig_t::SearchConfig_t(SearchCommandVersion_t cmdVer)
    : dptr(new Dptr_t(cmdVer))
{}


Sphinx::SearchConfig_t::SearchConfig_t(const Sphinx::SearchConfig_t &from)
    : dptr(new Dptr_t(*(from.dptr)))
{}


Sphinx::SearchConfig_t &Sphinx::SearchConfig_t::operator= (
    const Sphinx::SearchConfig_t &from)
{
    if (&from != this) {
        delete dptr;
        dptr = new Dptr_t(*(from.dptr));
    }
    return *this;
}


Sphinx::SearchConfig_t::~SearchConfig_t() {
    delete dptr;
}


void Sphinx::SearchConfig_t::addRangeFilter(const std::string &attrName,
        uint64_t minValue, uint64_t maxValue, bool excludeFlag)
{
    dptr->filters.push_back(new RangeFilter_t(attrName, minValue,
                                          maxValue, excludeFlag));
}

void Sphinx::SearchConfig_t::addEnumFilter(const std::string &attrName,
        const Int64Array_t &values, bool excludeFlag)
{
    dptr->filters.push_back(new EnumFilter_t(attrName, values,
                                         excludeFlag));
}

void Sphinx::SearchConfig_t::addEnumFilter(const std::string &attrName,
        const IntArray_t &values, bool excludeFlag)
{
    dptr->filters.push_back(new EnumFilter_t(attrName, values,
                                         excludeFlag));
}

void Sphinx::SearchConfig_t::addFloatRangeFilter(const std::string &attrName,
        float minValue, float maxValue, bool excludeFlag)
{
    dptr->filters.push_back(new FloatRangeFilter_t(attrName, minValue,
                                                   maxValue, excludeFlag));
}

void Sphinx::SearchConfig_t::addAttributeOverride(
                                 const std::string &attrName,
                                 AttributeType_t attrType,
                                 const std::map<uint64_t, Value_t> &values)
{
    dptr->attributeOverrides[attrName] = std::make_pair(attrType, values);
}//konec fce

void Sphinx::SearchConfig_t::addAttributeOverride(
                                 const std::string &attrName,
                                 AttributeType_t attrType,
                                 uint64_t docId, const Value_t &value)
{
    // get or insert attribute entry
    std::pair<AttributeType_t, std::map<uint64_t, Value_t> >
        &override = dptr->attributeOverrides[attrName];

    // update the entry
    override.first = attrType;
    override.second[docId] = value;
}//konec fce



bool Sphinx::SearchConfig_t::getFilter(int index, std::string &attrname,
        bool &exclude, float &minValue, float &maxValue) const
{
    // check index limits
    if (index < 0 || (unsigned)index >= dptr->filters.size())
        throw ClientUsageError_t("Filter index out of range.");

    // check filter type
    FloatRangeFilter_t *flt = dynamic_cast<FloatRangeFilter_t*>(dptr->filters[index]);
    if (!flt)
        return false;

    // fetch data
    attrname = flt->attrName;
    exclude = flt->excludeFlag;
    minValue = flt->minValue;
    maxValue = flt->maxValue;
    return true;
}

bool Sphinx::SearchConfig_t::getFilter(int index, std::string &attrname,
        bool &exclude, uint64_t &minValue, uint64_t &maxValue) const
{
    // check index limits
    if (index < 0 || (unsigned)index >= dptr->filters.size())
        throw ClientUsageError_t("Filter index out of range.");

    // check filter type
    RangeFilter_t *flt = dynamic_cast<RangeFilter_t*>(dptr->filters[index]);
    if (!flt)
        return false;

    // fetch data
    attrname = flt->attrName;
    exclude = flt->excludeFlag;
    minValue = flt->minValue;
    maxValue = flt->maxValue;
    return true;
}

bool Sphinx::SearchConfig_t::getFilter(int index, std::string &attrname,
        bool &exclude, Int64Array_t &values) const
{
    // check index limits
    if (index < 0 || (unsigned)index >= dptr->filters.size())
        throw ClientUsageError_t("Filter index out of range.");

    // check filter type
    EnumFilter_t *flt = dynamic_cast<EnumFilter_t*>(dptr->filters[index]);
    if (!flt)
        return false;

    // fetch data
    attrname = flt->attrName;
    exclude = flt->excludeFlag;
    values = flt->values;
    return true;
}


unsigned Sphinx::SearchConfig_t::getFilterCount() const {
    return dptr->filters.size();
}

const Sphinx::Filter_t *Sphinx::SearchConfig_t::getFilter(int index) const {
    // check index limits
    if (index < 0 || (unsigned)index >= dptr->filters.size())
        throw ClientUsageError_t("Filter index out of range.");

    return dptr->filters[index];
}

Sphinx::SearchCommandVersion_t Sphinx::SearchConfig_t::getCommandVersion() const {
    return dptr->commandVersion;
}

void Sphinx::SearchConfig_t::setPaging(uint32_t msgOffset, uint32_t msgLimit) {
    dptr->msgLimit = msgLimit;
    dptr->msgOffset = msgOffset;
}

void Sphinx::SearchConfig_t::setMatchMode(MatchMode_t matchMode) {
    dptr->matchMode = matchMode;
}

void Sphinx::SearchConfig_t::setSorting(
        SortMode_t sortMode, const std::string &sortBy) {
    dptr->sortMode = sortMode;
    dptr->sortBy = sortBy;
}

void Sphinx::SearchConfig_t::setRanking(
        RankingMode_t rankingMode, const std::string &rankExpr) {
    dptr->rankingMode = rankingMode;
    dptr->rankingExpr = rankExpr;
}

void Sphinx::SearchConfig_t::setGrouping(
        GroupFunction_t groupFunction,
        const std::string &groupBy,
        const std::string &groupSort) {
    dptr->groupFunction = groupFunction;
    dptr->groupBy = groupBy;
    dptr->groupSort = groupSort;
}

void Sphinx::SearchConfig_t::setGroupDistinctAttribute(
        const std::string &attributeName) {
    dptr->groupDistinctAttribute = attributeName;
}

void Sphinx::SearchConfig_t::setMaxMatches(int maxMatches) {
    dptr->maxMatches = maxMatches;
}

void Sphinx::SearchConfig_t::setMaxQueryTime(uint32_t maxQueryTime) {
    dptr->maxQueryTime = maxQueryTime;
}

void Sphinx::SearchConfig_t::setSearchedIndexes(const std::string &indexNames) {
    dptr->indexes = indexNames;
}

void Sphinx::SearchConfig_t::setIndexWeight(
        const std::string &indexName, uint32_t weight) {
    dptr->indexWeights[indexName] = weight;
}

void Sphinx::SearchConfig_t::setFieldWeight(
        const std::string &fieldName, uint32_t weight) {
    dptr->fieldWeights[fieldName] = weight;
}

void Sphinx::SearchConfig_t::setSearchCutoff(uint32_t searchCutOff) {
    dptr->searchCutOff = searchCutOff;
}

void Sphinx::SearchConfig_t::setRetries(
        uint32_t distRetryCount, uint32_t distRetryDelay) {
    dptr->distRetryCount = distRetryCount;
    dptr->distRetryDelay = distRetryDelay;
}

void Sphinx::SearchConfig_t::setGeoAnchorPoints(
        const std::vector<GeoAnchorPoint_t> &anchorPoints) {
    dptr->anchorPoints = anchorPoints;
}

void Sphinx::SearchConfig_t::setQueryComment(const std::string &queryComment) {
    dptr->queryComment = queryComment;
}

void Sphinx::SearchConfig_t::setSelectClause(const std::string &selectClause) {
    dptr->selectClause = selectClause;
}


uint32_t Sphinx::SearchConfig_t::getPagingOffset() const {
    return dptr->msgOffset;
}

uint32_t Sphinx::SearchConfig_t::getPagingLimit() const {
    return dptr->msgLimit;
}

Sphinx::MatchMode_t Sphinx::SearchConfig_t::getMatchMode() const {
    return dptr->matchMode;
}

Sphinx::SortMode_t Sphinx::SearchConfig_t::getSortingMode() const {
    return dptr->sortMode;
}

const std::string &Sphinx::SearchConfig_t::getSortingExpr() const {
    return dptr->sortBy;
}

Sphinx::RankingMode_t Sphinx::SearchConfig_t::getRankingMode() const {
    return dptr->rankingMode;
}

const std::string &Sphinx::SearchConfig_t::getRankingExpr() const {
    return dptr->rankingExpr;
}

Sphinx::GroupFunction_t Sphinx::SearchConfig_t::getGroupingFunction() const {
    return dptr->groupFunction;
}
const std::string &Sphinx::SearchConfig_t::getGroupByExpr() const {
    return dptr->groupBy;
}
const std::string &Sphinx::SearchConfig_t::getGroupSortExpr() const {
    return dptr->groupSort;
}
const std::string &Sphinx::SearchConfig_t::getGroupDistinctAttribute() const {
    return dptr->groupDistinctAttribute;
}

int Sphinx::SearchConfig_t::getMaxMatches() const {
    return dptr->maxMatches;
}
uint32_t Sphinx::SearchConfig_t::getMaxQueryTime() const {
    return dptr->maxQueryTime;
}

const std::string &Sphinx::SearchConfig_t::getSearchedIndexes() const {
    return dptr->indexes;
}
const std::map<std::string, uint32_t> &
Sphinx::SearchConfig_t::getIndexWeights() const {
    return dptr->indexWeights;
}
const std::map<std::string, uint32_t> &
Sphinx::SearchConfig_t::getFieldWeights() const {
    return dptr->fieldWeights;
}

uint32_t Sphinx::SearchConfig_t::getSearchCutoff() const {
    return dptr->searchCutOff;
}
uint32_t Sphinx::SearchConfig_t::getDistRetryCount() const {
    return dptr->distRetryCount;
}
uint32_t Sphinx::SearchConfig_t::getDistRetryDelay() const {
    return dptr->distRetryDelay;
}

const std::vector<Sphinx::GeoAnchorPoint_t> &
Sphinx::SearchConfig_t::getGeoAnchorPoints() const {
    return dptr->anchorPoints;
}

const std::string &Sphinx::SearchConfig_t::getQueryComment() const {
    return dptr->queryComment;
}

const std::string &Sphinx::SearchConfig_t::getSelectClause() const {
    return dptr->selectClause;
}

const Sphinx::SearchConfig_t::AttributeOverrides_t &
Sphinx::SearchConfig_t::getAttributeOverrides() const {
    return dptr->attributeOverrides;
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
    : connection(settings)
{}//konstruktor

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
    if(commandVersion != queryAttr.getCommandVersion())
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

Sphinx::MultiQueryOpt_t::MultiQueryOpt_t(SearchCommandVersion_t cv)
    : commandVersion(cv)
{}//konec fce

void Sphinx::MultiQueryOpt_t::initQuery(SearchCommandVersion_t cv)
{
    commandVersion = cv;

    sortedQueries.clear();
    sourceQueries.clear();
    responseIndex.clear();
    groupQueries.clear();

}//konec fce


void Sphinx::MultiQueryOpt_t::addQuery(const std::string& query,
                                    const SearchConfig_t &queryAttr)
{
    // query attr command version must match query command version
    if(commandVersion != queryAttr.getCommandVersion())
        throw ClientUsageError_t("multiQuery version does not match "
                                 "added query version.");


    size_t queryCount = sourceQueries.size();
    // fill source queries
    sourceQueries.push_back(SourceQuery_t(query, queryAttr, queryCount));

    // fill sorted queries
    sortedQueries.push_back(&sourceQueries.back());

    // fill response index
    responseIndex.push_back(std::pair<int,int>(queryCount, queryCount));

    // fill groupQueries
    if (groupQueries.size()) {
        groupQueries[0] = 0;
    } else {
        groupQueries.push_back(0);
    }

    // increase query count
    queryCount++;
}//konec fce

Sphinx::Query_t Sphinx::MultiQueryOpt_t::getGroupQuery(size_t i) const
{
    Sphinx::Query_t out;
    out.convertEndian = true;


    size_t queryCount = getQueryCountAtGroup(i);
    //printf("Getting group query %lu, subquery count=%lu\n", i, queryCount);
    int start = groupQueries[i];
    int end = start + queryCount;
    //printf("index into sorted queries: Start: %d, end: %d\n", start, end); 
    // concatenate queries into group
    for (int j = start; j < end; j++) {
        /*printf("sortedQueries[%d]: filters: %s, query addr=%p data=%p\n", j,
                sortedQueries[j]->getHash().c_str(),
                sortedQueries[j],
                sortedQueries[j]->getQuery().data);
        */
        out << sortedQueries[j]->getQuery();
    }
    return out;
}

size_t Sphinx::MultiQueryOpt_t::getGroupQueryCount() const
{
    return groupQueries.size();
}


size_t Sphinx::MultiQueryOpt_t::getResponseIndex(size_t sortedIndex) const
{
    return responseIndex[sortedIndex].second;
}


size_t Sphinx::MultiQueryOpt_t::getQueryCountAtGroup(size_t i) const
{
    if (i >= groupQueries.size()) {
        std::ostringstream o;
        o << i << " >= " << groupQueries.size();
        throw ClientUsageError_t("Group query index out of range: " + o.str());
    }
    int start = groupQueries[i];
    int end;
    if (i+1 >= groupQueries.size())
    {
        // last
        end = sortedQueries.size();
    } else 
    {
        end = groupQueries[i+1];
    }
    return end-start;
}

struct QuerySorter_t {
    bool operator()(const Sphinx::SourceQuery_t *lhs,
                    const Sphinx::SourceQuery_t *rhs)
    {
        return lhs->getHash() < rhs->getHash();
    }
};


void Sphinx::MultiQueryOpt_t::optimise() {

    // disable optimisation for older protocols
    if (commandVersion < VER_COMMAND_SEARCH_0_9_9) {
        //printf("optimisation disabled, old command version\n");
        return;
    }
    //printf("optimisation enabled\n");

    //printf("going to sort sorted queries ...\n");
    // sort queries by hash
    std::sort(sortedQueries.begin(), sortedQueries.end(), QuerySorter_t());
    //printf("sorting done.\n");

    // build response index and groupQueries
    responseIndex.clear();
    groupQueries.clear();
    // fill from sortedQueries
    int seqNo = 0;
    std::string lastHash;

    for (std::vector<const SourceQuery_t *>::const_iterator i
            = sortedQueries.begin(); i != sortedQueries.end(); i++)
    {
        // build response index
        responseIndex.push_back(std::pair<int,int>(seqNo,
                                                   (*i)->getInputSeqNo()));
        // build group Queries
        if (i==sortedQueries.begin() || (*i)->getHash() != lastHash) {
            // hash changed, start of group 
            groupQueries.push_back(seqNo);
            lastHash = (*i)->getHash();
        }
        seqNo++;
    }
    // sort by first indexes
    std::sort(responseIndex.begin(), responseIndex.end());
}

int Sphinx::MultiQueryOpt_t::getQueryCount() const
{
    return sourceQueries.size();
}

Sphinx::SearchCommandVersion_t Sphinx::MultiQueryOpt_t::getCommandVersion() const {
    return commandVersion;
}//konec fce

//-------------------------------------------------------------------------

Sphinx::SourceQuery_t::SourceQuery_t(const std::string &query,
                             const SearchConfig_t &attr,
                             int seqNo)
    : inputSeqNo(seqNo)
{
    // serialize query and store
    serializedQuery.convertEndian = true;
    buildQueryVersion(query, attr, serializedQuery);

    // compute hash
    std::ostringstream o;
    o << query << "\t"
      << attr.getSelectClause() << "\t"
      << attr.getMatchMode() << "\t";
    // add filters to hash
    for (unsigned i = 0; i < attr.getFilterCount(); ++i) {
        o << *attr.getFilter(i) << "\t";
    }
    hash = o.str();
}


//-------------------------------------------------------------------------




//-------------------------------------------------------------------------

void Sphinx::Client_t::query(const std::string& query,
                             const SearchConfig_t &attrs,
                             Response_t &response)
{
    Query_t data, request;
    data.convertEndian = true;
    request.convertEndian = true;

    //-------------------build query---------------
    buildQueryVersion(query, attrs, data);
    buildHeader(SEARCHD_COMMAND_SEARCH, attrs.getCommandVersion(),
            data.getLength(), request);
    request << data;

    // initialize query polling machine
    Sphinx::QueryMachine_t queryMachine(connection);

    // put query into query machine
    queryMachine.addQuery(request);

    // launch query machine
    queryMachine.launch();

    Query_t responseData = queryMachine.getResponse(0);

    //--------- parse response -------------------
    parseResponseVersion(responseData, attrs.getCommandVersion(), response);
}//konec fce

void Sphinx::Client_t::query(const MultiQueryOpt_t &mq,
                             std::vector<Response_t> &response)
{
    SearchCommandVersion_t cmdVer = mq.getCommandVersion();

    // test, if multiquery initialized, get the query
    size_t groupCount = mq.getGroupQueryCount();
    if (!groupCount) 
        throw ClientUsageError_t("multiQuery not initialised or zero length.");

    // initialize query polling machine
    Sphinx::QueryMachine_t queryMachine(connection);

    // put queries into query machine
    for (size_t i=0; i<groupCount; i++) {
        Sphinx::Query_t groupQuery = mq.getGroupQuery(i);
        size_t queryCount = mq.getQueryCountAtGroup(i);

        if (!groupQuery.getLength() || !queryCount) {
            throw ClientUsageError_t("multiQuery not initialised "
                                      " or zero length.");
        }

        Query_t request;
        request.convertEndian = true;
        // create request header
        buildHeader(SEARCHD_COMMAND_SEARCH, cmdVer,
                    groupQuery.getLength(),
                    request, queryCount);
            
        // add body to request
        request << groupQuery;

        //printf("launching group query, %lu subqueries\n", queryCount);
        queryMachine.addQuery(request);
    }
    // launch query machine
    queryMachine.launch();

    // prepare response vector
    response.clear();
    response.resize(mq.getQueryCount());

    // parse responses
    std::string lastQueryWarning;

    // step thru group responses
    size_t seqNo = 0;
    for (size_t i=0; i<groupCount; i++)
    {
        Query_t &data = queryMachine.getResponse(i);
        size_t queryCount = mq.getQueryCountAtGroup(i);
        // step thru queries within one response
        for (size_t j = 0; j < queryCount; j++) {
            Response_t resp;
            try {
                parseResponseVersion(data, cmdVer, resp);
            } catch (const Warning_t &wt) {
                std::ostringstream msg;
                msg << "Query " << (i+1) << "," << (j+1) << ": " << wt.what();
                lastQueryWarning = msg.str();
            }
            // place response into output
            /*printf("%lu. response, getResponseIndex = %lu\n",
                    seqNo, mq.getResponseIndex(seqNo));
            */
            response[mq.getResponseIndex(seqNo)] = resp;
            seqNo++;
        }
        if (!lastQueryWarning.empty())
            throw Warning_t(lastQueryWarning);
    }
} 

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

    // initialize query polling machine
    Sphinx::QueryMachine_t queryMachine(connection);

    // put query into query machine
    queryMachine.addQuery(request);

    // launch query machine
    queryMachine.launch();

    // get response data from machine
    data = queryMachine.getResponse(0);

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
    uint32_t updatedCount;

    // build request
    data.convertEndian = request.convertEndian = true;
    buildUpdateRequest_v0_9_8(data, index, at);

    // prepend header
    buildHeader(SEARCHD_COMMAND_UPDATE, at.commandVersion, data.getLength(),
                request);
    request << data;

    // initialize query polling machine
    Sphinx::QueryMachine_t queryMachine(connection);

    // put query into query machine
    queryMachine.addQuery(request);

    // launch query machine
    queryMachine.launch();

    // get response data from machine
    data = queryMachine.getResponse(0);

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
    std::vector<KeywordResult_t> result;

    // build request
    data.convertEndian = request.convertEndian = true;
    buildKeywordsRequest_v0_9_8(data, index, query, getWordStatistics);

    // prepend header
    buildHeader(SEARCHD_COMMAND_KEYWORDS, VER_COMMAND_KEYWORDS_0_9_8,
                data.getLength(), request);
    request << data;

    // initialize query polling machine
    Sphinx::QueryMachine_t queryMachine(connection);

    // put query into query machine
    queryMachine.addQuery(request);

    // launch query machine
    queryMachine.launch();

    // get response data from machine
    data = queryMachine.getResponse(0);

    //parse response
    parseKeywordsResponse_v0_9_8(data, result, getWordStatistics);
    
    return result;
}//konec fce

//-----------------------------------------------------------------------------

void sphinxClientDummy()
{ }


