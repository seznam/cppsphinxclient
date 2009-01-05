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

//-----------------------------------------------------------------------------

void buildHeader(Sphinx::Command_t command, unsigned short version,
                 int queryLength, Sphinx::Query_t &data, int queryCount=1)
{
    data << (unsigned short) command;
    data << (unsigned short) version;

    if(command==Sphinx::SEARCHD_COMMAND_SEARCH && 
       version>=Sphinx::VER_COMMAND_SEARCH_0_9_8) {
        data << (uint32_t) (queryLength + sizeof(uint32_t));
        data << (uint32_t) queryCount;
    }
    else
        data << (uint32_t) queryLength;
}//konec fce

//------------------------------------------------------------------------------

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
        (*it)->dumpToBuff(data, Sphinx::VER_COMMAND_SEARCH_0_9_7_1);
    }

    //group by
    data << (uint32_t) attrs.groupFunction;
    data << attrs.groupBy;

    //max matches
    data << (uint32_t) attrs.maxMatches;

    // group sort criterion
    data << attrs.groupSort;
}//konec fce


void buildQuery_v0_9_8(const std::string &query,
                       const Sphinx::SearchConfig_t &attrs,
                       Sphinx::Query_t &data)
{
    /*
    int32_t offset, limit, matchMode, rankingMode, sortMode
    string sort_by
    string query
    int32_t number_of_weights {int32_t weight}
    string indexes
    int32_t 1 ... 64bit id range marker
    int64_t min_id, int64_t max_id
    
    int32_t total_filter_count
        {
        string attribute_name
        int32_t filter_type
        ( int32_t value_count {int32_t value} )
         | (int32_t min, max)
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
    */

    //limits, modes
    data << (uint32_t) attrs.msgOffset << (uint32_t) attrs.msgLimit;
    data << (uint32_t) attrs.matchMode;
    data << (uint32_t) attrs.rankingMode;
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
    data << (uint32_t) 1; //64bit id range marker
    data << (uint64_t)(attrs.minId) << (uint64_t)(attrs.maxId);

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
        Sphinx::RangeFilter_t filter(min->first, min->second, max->second);
        filter.dumpToBuff(data, Sphinx::VER_COMMAND_SEARCH_0_9_8);
    }//for

    /*
     * old filter interface - enum (value) filters
     */

    for (std::map<std::string, Sphinx::IntArray_t >::const_iterator filt
         = attrs.filter.begin() ; filt != attrs.filter.end() ; filt++)
    {
        Sphinx::EnumFilter_t filter(filt->first, filt->second);
        filter.dumpToBuff(data, Sphinx::VER_COMMAND_SEARCH_0_9_8);
    }//for

    /*
     * new filter interface
     */
    for (std::vector<Sphinx::Filter_t *>::const_iterator it= attrs.filters.begin();
         it != attrs.filters.end(); it++)
    {
        (*it)->dumpToBuff(data, Sphinx::VER_COMMAND_SEARCH_0_9_8);
    }

    //group by
    data << (uint32_t) attrs.groupFunction;
    data << attrs.groupBy;

    //max matches
    data << (uint32_t) attrs.maxMatches;

    // group sort criterion
    data << attrs.groupSort;
    
    //search cutoff, distributed search retry count and delay
    data << (uint32_t) attrs.searchCutOff
         << (uint32_t) attrs.distRetryCount
         << (uint32_t) attrs.distRetryDelay;

    //group distinct attribute
    data << attrs.groupDistinctAttribute;

    //geographical anchor points
    data << (uint32_t) attrs.anchorPoints.size();
    for (std::vector<Sphinx::GeoAnchorPoint_t>::const_iterator apI = \
         attrs.anchorPoints.begin() ;
         apI != attrs.anchorPoints.end() ; apI++)
    {
        data << apI->lattitudeAttributeName << apI->longitudeAttributeName;
        data << apI->lattitude << apI->longitude;
    }//for

    //per-index weights
    data << (uint32_t)attrs.indexWeights.size();
    for (std::map<std::string, uint32_t>::const_iterator iw=attrs.indexWeights.begin();
         iw != attrs.indexWeights.end() ; iw++)
    {
        data << iw->first << (uint32_t)iw->second;
    }//for

    //maximum query duration
    data << (uint32_t)attrs.maxQueryTime;

    //per-field weights
    data << (uint32_t)attrs.fieldWeights.size();
    for (std::map<std::string, uint32_t>::const_iterator fw=attrs.fieldWeights.begin();
         fw != attrs.fieldWeights.end() ; fw++)
    {
        data << fw->first << (uint32_t)fw->second;
    }//for

    //query comment
    data << attrs.queryComment;
}//konec fce


void parseResponse_v0_9_6(Sphinx::Query_t &data, Sphinx::Response_t &response)
{
    uint32_t matchCount;
    uint32_t wordCount;
    uint32_t docId;

    response.clear();

    //number of entries to fetch
    if (!(data >> matchCount))
        throw Sphinx::MessageError_t(
                         "Can't read any data. Probably zero-length response.");

    for (unsigned int i=0 ; i<matchCount ; i++)
    {
        Sphinx::ResponseEntry_t entry;
        data >> docId;
        entry.documentId = docId;
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
        uint32_t docId;
        data >> docId;
        entry.documentId = docId;
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
            } else {
                //process uint32_t and vector attributes
                uint32_t value;
                data >> value;

                //process multi-attributes
                if(attr->second & Sphinx::SPH_ATTR_MULTI) {
                    //get type of entries
                    int type = attr->second ^ Sphinx::SPH_ATTR_MULTI;
                    //value means number of attributes following
                    //parse multi-attributes
                    switch(type) {
                    case Sphinx::SPH_ATTR_FLOAT:
                        entry.attribute.insert(std::make_pair(attr->first,
                            parseMultiAttribute<float>(data, value)));
                        break;
                    default:
                        entry.attribute.insert(std::make_pair(attr->first,
                            parseMultiAttribute<uint32_t>(data, value)));
                        break;
                    }//switch
                } else {
                    //single uint32_t attribute
                    entry.attribute.insert(std::make_pair(attr->first, value));
                }//else
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
    switch (attrs.commandVersion)
    {
        case Sphinx::VER_COMMAND_SEARCH_0_9_6:
            buildQuery_v0_9_6(query, attrs, data);
            break;

        case Sphinx::VER_COMMAND_SEARCH_0_9_7:
            buildQuery_v0_9_7(query, attrs, data);
            break;

        case Sphinx::VER_COMMAND_SEARCH_0_9_7_1:
            buildQuery_v0_9_7_1(query, attrs, data);
            break;

        case Sphinx::VER_COMMAND_SEARCH_0_9_8:
            buildQuery_v0_9_8(query, attrs, data);
            break;
    }//switch
}//konec fce


void parseResponseVersion(Sphinx::Query_t &data,
                          Sphinx::SearchCommandVersion_t responseVersion,
                          Sphinx::Response_t &response)
{
    switch (responseVersion)
    {
        case Sphinx::VER_COMMAND_SEARCH_0_9_6:
            parseResponse_v0_9_6(data, response);
            response.commandVersion = Sphinx::VER_COMMAND_SEARCH_0_9_6;
            break;

        case Sphinx::VER_COMMAND_SEARCH_0_9_7:
        case Sphinx::VER_COMMAND_SEARCH_0_9_7_1:
            parseResponse_v0_9_7(data, response);
            response.commandVersion = Sphinx::VER_COMMAND_SEARCH_0_9_7;
            break;

        case Sphinx::VER_COMMAND_SEARCH_0_9_8:
            response.commandVersion = Sphinx::VER_COMMAND_SEARCH_0_9_8;
            parseResponse_v0_9_8(data, response);
            break;

        default:
            throw Sphinx::MessageError_t(
                    "Invalid response version (0x101, 0x104, 0x113 supported).");
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




