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
 * functions for parsing and building sphinx communication protocol messages
 * for various protocol versions
 * 
 * AUTHOR
 * Jan Kirschner <jan.kirschner@firma.seznam.cz>
 *
 * HISTORY
 * 2006-12-11 (jan.kirschner)
 *            First draft.
 */


#include <sphinxclient/sphinxclient.h>
#include <sphinxclient/sphinxclientquery.h>
#include <sphinxclient/error.h>
#include <sphinxclient/globals.h>
#include "filter.h"

//-----------------------------------------------------------------------------

void buildHeader(Sphinx::Command_t command, unsigned short version,
                 int queryLength, Sphinx::Query_t &data, int queryCount)
{
    data << (unsigned short) command;
    data << (unsigned short) version;

    if (command==Sphinx::SEARCHD_COMMAND_SEARCH) {
        data << (uint32_t) (queryLength + sizeof(uint32_t)*2);
        if (version == Sphinx::VER_COMMAND_SEARCH_0_9_9) {
            data << (uint32_t) queryCount;
        } else {
            data << (uint32_t) 0 << (uint32_t) queryCount;
        }
    } else {
        data << (uint32_t) queryLength;
    }
}//konec fce

//------------------------------------------------------------------------------


void buildQuery_v0_9_9(const std::string &query,
                       const Sphinx::SearchConfig_t &attrs,
                       Sphinx::Query_t &data)
{
    /*
    int32_t offset, limit, matchMode, rankingMode
    [string rankExpr]  - pro rankingMode = SHP_RANK_EXPR, pridano ve 2.0.5
    int32_t sortMode (deprecated)
    string sort_by
    string query
    int32_t number_of_weights {int32_t weight} - DEPRECATED
    string indexes
    int32_t 1 ... 64bit id range marker
    int64_t min_id, int64_t max_id - DEPRECATED
    
    int32_t total_filter_count
        {
        string attribute_name
        int32_t filter_type
        ( int32_t value_count {int64_t value} ) - zmena na 64bit
         | (int64_t min, max)                   - zmena na 64bit
         | (packed_float32_t min, max)
        int32_t excludeFlag
        }

    int32_t group_func, string group_by
    int32_t maxMatches
    string groupSort
    int32_t cutoff, retrycount, retrydelay
    string groupdistinct
    
    int32_t anchor_point_count
    {string attr_lattitude, string attr_longitude, packed_float32_t lattitude, packed_float32_t longitude}

    int32_t indexWeightCount {string indexName, int32_t weight}
    int32_t maxQueryTime
    int32_t fieldWeightCount {string fieldName, int32_t weight}
    string comment

    int32_t overrideCount
        {
        string attrName
        int32_t attrType
        int32_t documentCount
            {
                int64_t docId
                (int64_t|packed_float32_t|int32_t) attrValue
            }
        }

    string selectClause - pridano v 0.9.9
    */

    //limits, modes
    data << attrs.getPagingOffset() << attrs.getPagingLimit();
    data << (uint32_t) attrs.getMatchMode();
    data << (uint32_t) attrs.getRankingMode();
    // ranking expression
    if (attrs.getRankingMode() == Sphinx::SPH_RANK_EXPR &&
            attrs.getCommandVersion() >= Sphinx::VER_COMMAND_SEARCH_2_0_5)
        data << attrs.getRankingExpr();
    data << (uint32_t) attrs.getSortingMode();

    //sort_by
    data << attrs.getSortingExpr();

    //query
    data << query;

    //weights - DEPRECATED, use fieldWeights instead
    data << (uint32_t) 0;

    //indexes
    data << attrs.getSearchedIndexes();

    //id range
    data << (uint32_t) 1; //64bit id range marker
    // deprecated ID range, use attribute @id range filter
    data << (uint64_t)(0) << (uint64_t)(0);

    //filters
    unsigned filterCount = attrs.getFilterCount();
    data << (uint32_t)(filterCount);

    /*
     * new filter interface
     */
    for (unsigned i = 0; i < filterCount; ++i) {
        attrs.getFilter(i)->dumpToBuff(data);
    }

    //group by
    data << (uint32_t) attrs.getGroupingFunction();
    data << attrs.getGroupByExpr();

    //max matches
    data << (uint32_t) attrs.getMaxMatches();

    // group sort criterion
    data << attrs.getGroupSortExpr();

    //search cutoff, distributed search retry count and delay
    data << (uint32_t) attrs.getSearchCutoff()
         << (uint32_t) attrs.getDistRetryCount()
         << (uint32_t) attrs.getDistRetryDelay();

    //group distinct attribute
    data << attrs.getGroupDistinctAttribute();

    //geographical anchor points
    const std::vector<Sphinx::GeoAnchorPoint_t> &anchorPoints =
        attrs.getGeoAnchorPoints();
    data << (uint32_t) anchorPoints.size();
    for (std::vector<Sphinx::GeoAnchorPoint_t>::const_iterator
            apI = anchorPoints.begin() ; apI != anchorPoints.end() ; apI++)
    {
        data << apI->lattitudeAttributeName << apI->longitudeAttributeName;
        data << apI->lattitude << apI->longitude;
    }//for

    //per-index weights
    const std::map<std::string, uint32_t> &indexWeights = attrs.getIndexWeights();
    data << (uint32_t)indexWeights.size();
    for (std::map<std::string, uint32_t>::const_iterator iw=indexWeights.begin();
         iw != indexWeights.end() ; iw++)
    {
        data << iw->first << (uint32_t)iw->second;
    }//for

    //maximum query duration
    data << (uint32_t)attrs.getMaxQueryTime();

    //per-field weights
    const std::map<std::string, uint32_t> &fieldWeights = attrs.getFieldWeights();
    data << (uint32_t)fieldWeights.size();
    for (std::map<std::string, uint32_t>::const_iterator fw=fieldWeights.begin();
         fw != fieldWeights.end() ; fw++)
    {
        data << fw->first << (uint32_t)fw->second;
    }//for

    //query comment
    data << attrs.getQueryComment();

    //attribute overrides
    const Sphinx::SearchConfig_t::AttributeOverrides_t &attributeOverrides =
        attrs.getAttributeOverrides();
    data << (uint32_t) attributeOverrides.size();

    Sphinx::SearchConfig_t::AttributeOverrides_t::const_iterator
        ovrI = attributeOverrides.begin();

    for ( ; ovrI != attributeOverrides.end() ; ++ovrI) {
        // attribute name and type, count of docs overridden
        data << ovrI->first;
        data << (uint32_t) ovrI->second.first;
        data << (uint32_t) ovrI->second.second.size();

        // docs overridden
        for (std::map<uint64_t, Sphinx::Value_t>::const_iterator dI
                = ovrI->second.second.begin() ;
                dI != ovrI->second.second.end() ; ++dI)
        {
            // document id
            data << dI->first;
            //value
            switch (dI->second.getValueType()) {
                case Sphinx::VALUETYPE_UINT32:
                    data << (uint32_t) dI->second; break;
                case Sphinx::VALUETYPE_FLOAT:
                    data << (float) dI->second; break;
                case Sphinx::VALUETYPE_UINT64:
                    data << (uint64_t) dI->second; break;
                case Sphinx::VALUETYPE_VECTOR:
                default:
                    throw Sphinx::ClientUsageError_t("Attributes with some "
                            "value types (such as vector) can't be overriden.");
            }//switch
        }//for documents
    }//for attributes

    //select clause
    data << attrs.getSelectClause();
}//konec fce



template<class T>
std::vector<Sphinx::Value_t> parseMultiAttribute(Sphinx::Query_t &data, int value)
{
    std::vector<Sphinx::Value_t> vec;

    for(int i=0 ; i<value ; ++i) {
        T entVal;
        data >> entVal;
        //use conversion constructor to create instance of Value_t
        vec.push_back(entVal);
    }//for

    return vec;
}//konec fce


void parseResponse_v0_9_8(Sphinx::Query_t &data, Sphinx::Response_t &response)
{
    uint32_t matchCount;
    uint32_t wordCount;
    uint32_t fieldCount;
    uint32_t attrCount;
    uint32_t errorStatus;
    std::string errmsg;

    response.clear();

    //read error status
    if(!(data >> errorStatus))
        throw Sphinx::MessageError_t(
                "Can't read any data. Probably zero-length response.");
    if(errorStatus!=Sphinx::SEARCHD_OK) {
        errmsg = "Response status OK, but query status failed";
        std::string description;

        if (!(data >> description))
            errmsg += std::string(".");
        else
            errmsg += std::string(": ") + description;

        if (errorStatus != Sphinx::SEARCHD_WARNING)
            throw Sphinx::MessageError_t(errmsg);
    }//if

    //read fields
    if (!(data >> fieldCount))
        throw Sphinx::MessageError_t(
                "Can't read any field count. Probably too short response.");

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
        data >> type;

        response.attribute.push_back(std::make_pair(name, type));
    }//for

    // number of entries to fetch
    if (!(data >> matchCount))
        throw Sphinx::MessageError_t("Error parsing response.");
    // 64bit id ?
    data >> response.use64bitId;

    // fetch matches
    for (unsigned int i=0 ; i<matchCount ; i++)
    {
        Sphinx::ResponseEntry_t entry;
        if(response.use64bitId)
            data >> entry.documentId;
        else {
            uint32_t docId;
            data >> docId;
            entry.documentId = docId;
        }//else

        data >> entry.weight;
        entry.groupId = entry.timestamp = 0;

        //read attribute values
        for (Sphinx::AttributeTypes_t::iterator attr
             = response.attribute.begin() ; attr!=response.attribute.end() ;
             attr++)
        {
            if (attr->second == Sphinx::SPH_ATTR_FLOAT) {
                //process floating point attributes
                float value;
                data >> value;
                //doesn't matter, Value_t has conversion constructor from float
                entry.attribute.insert(std::make_pair(attr->first, value));
            } else if (attr->second == Sphinx::SPH_ATTR_BIGINT) {
                // process uint64_t attributes
                uint64_t value;
                data >> value;
                entry.attribute.insert(std::make_pair(attr->first, value));
            } else if (attr->second == Sphinx::SPH_ATTR_MULTI ||
                    attr->second == Sphinx::SPH_ATTR_MULTI_FLAG) {
                // 32 bit multi-value attribute
                uint32_t valueCount;
                data >> valueCount;
                //parse multi-attributes
                entry.attribute.insert(std::make_pair(attr->first,
                        parseMultiAttribute<uint32_t>(data, valueCount)));
            } else if (attr->second == Sphinx::SPH_ATTR_MULTI64) {
                // 64 bit multi-value attribute
                uint32_t valueCount;
                data >> valueCount;
                //parse multi-attributes
                entry.attribute.insert(std::make_pair(attr->first,
                        parseMultiAttribute<uint64_t>(data, valueCount)));
            } else if (attr->second == Sphinx::SPH_ATTR_STRING) {
                //process string attributes
                std::string value;
                data >> value;
                entry.attribute.insert(std::make_pair(attr->first, value));
            } else {
                //process uint32_t attributes
                uint32_t value;
                data >> value;
                entry.attribute.insert(std::make_pair(attr->first, value));
            }//else
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

    if (errorStatus == Sphinx::SEARCHD_WARNING)
        throw Sphinx::Warning_t(std::string("Warning: ") + errmsg);
}//konec fce

//------------------------------------------------------------------------------


void buildQueryVersion(const std::string &query,
                       const Sphinx::SearchConfig_t &attrs,
                       Sphinx::Query_t &data)
{
    switch (attrs.getCommandVersion())
    {
        case Sphinx::VER_COMMAND_SEARCH_0_9_9:
        case Sphinx::VER_COMMAND_SEARCH_2_0_5:
            buildQuery_v0_9_9(query, attrs, data);
            break;
    }//switch
}//konec fce


void parseResponseVersion(Sphinx::Query_t &data,
                          Sphinx::SearchCommandVersion_t responseVersion,
                          Sphinx::Response_t &response)
{
    switch (responseVersion)
    {
        case Sphinx::VER_COMMAND_SEARCH_0_9_9:
        case Sphinx::VER_COMMAND_SEARCH_2_0_5:
            response.commandVersion = responseVersion;
            parseResponse_v0_9_8(data, response);
            break;

        default:
            throw Sphinx::MessageError_t(
                    "Invalid response version (0x101, 0x104, "
                    "0x113, 0x116 supported).");
            break;
    }//switch
}//konec fce

//------------------------------------------------------------------------------

void buildUpdateRequest_v0_9_8(Sphinx::Query_t &data,
                               const std::string &index,
                               const Sphinx::AttributeUpdates_t &at)
{
    /*
    string indexName
    uint32_t attributeCount
    { string attributeName }
    uint32_t valueCount
    {
        uint64_t documentId
        { uint32_t value }
    }
    */

    //index to update
    data << index;

    // attribute names
    data << (uint32_t)at.attributes.size();
    for(std::vector<std::string>::const_iterator atI = at.attributes.begin() ;
        atI != at.attributes.end() ; atI++)
    {
        data << *atI;
    }//for

    // values
    data << (uint32_t)at.values.size();
    for(std::map<uint64_t, std::vector<Sphinx::Value_t> >::const_iterator docI =
                                                            at.values.begin() ;
        docI != at.values.end() ; docI++)
    {
        // document Id
        data << (uint64_t)(docI->first);
        // attribute values
        for(std::vector<Sphinx::Value_t>::const_iterator valI = docI->second.begin() ;
            valI != docI->second.end() ; valI++)
        {
            data << (uint32_t)(*valI);
        }//for
    }//for
    
}//konec fce

void parseUpdateResponse_v0_9_8(Sphinx::Query_t &data, uint32_t &updateCount)
{
    if(!(data >> updateCount))
        throw Sphinx::MessageError_t("Error parsing response.");
}//konec fce


void buildKeywordsRequest_v0_9_8(Sphinx::Query_t &data,
                                 const std::string &index,
                                 const std::string &query,
                                 bool fetchStats)
{
    /*
    string query
    string indexName
    uint32_t hits
    */
    // query to analyze
    data << query;
    // index to get settings
    data << index;
    // whether to etch additional statistics
    data << (uint32_t)fetchStats;
}//konec fce

void parseKeywordsResponse_v0_9_8(Sphinx::Query_t &data,
                                  std::vector<Sphinx::KeywordResult_t> &result,
                                  bool fetchStats)
{
    /*
    uint32_t nWords
    {
      string tokenized
      string normalized
      [ uint32_t docs, uint32_t hits ]
    }
    */
    uint32_t nWords;

    // prepare result
    result.clear();

    // get number of words
    if(!(data >> nWords))
        throw Sphinx::MessageError_t("Error parsing response - length mismatch.");

    // get word data
    for (unsigned i=0 ; i<nWords ; i++) {
        Sphinx::KeywordResult_t entry;

        data >> entry.tokenized;
        data >> entry.normalized;

        if (fetchStats) {
            data >> entry.statistics.docsHit;
            data >> entry.statistics.totalHits;
        } else {
            entry.statistics.docsHit = 0;
            entry.statistics.totalHits = 0;
        }//else

        result.push_back(entry);
    }//for
}//konec fce


