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
 * SphinxClient Filters header file - communication library for the sphinx
 * search server
 *
 * AUTHOR
 * Jan Kirschner <jan.kirschner@firma.seznam.cz>
 *
 * HISTORY
 * 2007-01-03 (jan.kirschner)
 *            First draft.
 */

//! @file filter.h

#ifndef __SPHINX_FILTER_H__
#define __SPHINX_FILTER_H__

#include <sphinxclient/sphinxclientquery.h>
#include <sphinxclient/globals_public.h>
#include <sstream>
#include <string>
#include <vector>

namespace Sphinx
{

/** known filter types
 */

enum FilterType_t {
                    SPH_FILTER_VALUES = 0,
                    SPH_FILTER_RANGE = 1,
                    SPH_FILTER_FLOATRANGE = 2
                  };


/** @brief Attribute filter base class 
 *
 * Sucessors are RangeFilter and EnumFilter. exclude_flag is ignored
 * at versions older than 1.0.7. Filter classes replaces at 1.0.7
 * higher member structures SearchConfig_t.min, SearchConfig_t.max
 * and SearchConfig_t.filter. These structures remain at interface
 * to ensure backward compatibility.
 */
struct Filter_t
{
    Filter_t();

    virtual void dumpToBuff(Sphinx::Query_t &data,
                            const Sphinx::SearchCommandVersion_t &v) const = 0;

    /* @brief print current 
     *
     *
     */
    virtual std::ostream & print(std::ostream &o) const = 0;

    friend std::ostream & operator<< (std::ostream &o,
                                      const Filter_t &f);

    /** @brief make new object as a deep copy of current
      * @return pointer to the newly created object
      */
    virtual Filter_t * clone() const = 0;

       
    virtual ~Filter_t() {};

    std::string attrName;
    bool excludeFlag;
};

/** @brief Attribute range filter
  *
  * Range filter provides filtering search result based on attribute
  * range.
  */ 
struct RangeFilter_t : public Filter_t
{
    RangeFilter_t(std::string attrName, uint32_t minValue,
                  uint32_t maxValue, bool excludeFlag=false);
    void dumpToBuff(Sphinx::Query_t &data,
                    const Sphinx::SearchCommandVersion_t &v) const;
    virtual std::ostream & print(std::ostream &o) const;
    virtual Filter_t * clone() const;

    uint64_t minValue;
    uint64_t maxValue;
};

/** @brief Attribute enum filter
  *
  * Enum filter provides filtering search result based on enumerated
  * attribute values.
  */ 
struct EnumFilter_t : public Filter_t {


    EnumFilter_t(std::string attrName, const Int64Array_t &values,
                 bool excludeFlag = false);
    EnumFilter_t(std::string attrName, const IntArray_t &values,
                 bool excludeFlag = false);
    void dumpToBuff(Sphinx::Query_t &data,
                    const Sphinx::SearchCommandVersion_t &v) const;
    virtual std::ostream & print(std::ostream &o) const;
    virtual Filter_t * clone() const;

    Int64Array_t values;
};

/** @brief Attribute float range filter
  *
  * Range filter provides filtering search result based on floating point
  * attribute range.
  */ 
struct FloatRangeFilter_t : public Filter_t {

    FloatRangeFilter_t(std::string attrName, float minValue, float maxValue,
                 bool excludeFlag = false);
    void dumpToBuff(Sphinx::Query_t &data,
                    const Sphinx::SearchCommandVersion_t &v) const;
    virtual std::ostream & print(std::ostream &o) const;
    virtual Filter_t * clone() const;

    float minValue;
    float maxValue;
};

}//namespace

#endif

