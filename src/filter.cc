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
 * Sphinx::Filter_t function definitions - implements conversion to
 * binary format of sphinx protocol and some constructors
 * 
 * AUTHOR
 * Jan Kirschner <jan.kirschner@firma.seznam.cz>
 *
 * HISTORY
 * 2006-12-11 (jan.kirschner)
 *            First draft.
 */


#include <sphinxclient/filter.h>


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

void Sphinx::RangeFilter_t::dumpToBuff(Sphinx::Query_t &data,
                                const Sphinx::SearchCommandVersion_t &v) const
{
    switch(v) {
        case VER_COMMAND_SEARCH_0_9_7_1:
            data << attrName;
            data << (uint32_t) 0
                 << (uint32_t) minValue
                 << (uint32_t) maxValue;
            data << (uint32_t) excludeFlag;
            break;

        case VER_COMMAND_SEARCH_0_9_8:
            data << attrName;
            data << (uint32_t) SPH_FILTER_RANGE
                 << (uint32_t) minValue
                 << (uint32_t) maxValue;
            data << (uint32_t) excludeFlag;
            break;

        default:
            //incompatible modes
            break;
    }//switch
}

Sphinx::EnumFilter_t::EnumFilter_t(std::string attrName,
                                   const IntArray_t &values,
                                   bool excludeFlag)
        : values(values)
{
        this->attrName = attrName;
        this->excludeFlag = excludeFlag;
}

void Sphinx::EnumFilter_t::dumpToBuff(Sphinx::Query_t &data,
                               const Sphinx::SearchCommandVersion_t &v) const
{
    switch(v) {
        case VER_COMMAND_SEARCH_0_9_7_1:
            data << attrName;
            data << (uint32_t) values.size();
            for (Sphinx::IntArray_t::const_iterator val = values.begin();
                 val != values.end(); val++)
            {
                data << (uint32_t)(*val);
            }
            data << (uint32_t) excludeFlag;
            break;

        case VER_COMMAND_SEARCH_0_9_8:
            data << attrName;
            data << (uint32_t) SPH_FILTER_VALUES;
            data << (uint32_t) values.size();
            for (Sphinx::IntArray_t::const_iterator val = values.begin();
                 val != values.end(); val++)
            {
                data << (uint32_t)(*val);
            }
            data << (uint32_t) excludeFlag;
            break;

        default:
            //incompatible modes
            break;
    }//switch
};


Sphinx::FloatRangeFilter_t::FloatRangeFilter_t(std::string attrName, float minValue,
                                     float maxValue, bool excludeFlag)
                : minValue(minValue), maxValue(maxValue) 
{
    this->attrName = attrName;
    this->excludeFlag = excludeFlag;
}

void Sphinx::FloatRangeFilter_t::dumpToBuff(Sphinx::Query_t &data,
                                const Sphinx::SearchCommandVersion_t &v) const
{
        //unsupported in previous versions, does not work 
        if(v >= VER_COMMAND_SEARCH_0_9_8) {
            data << attrName;
            data << (uint32_t) SPH_FILTER_FLOATRANGE
                 << minValue //as float values
                 << maxValue;
            data << (uint32_t) excludeFlag;
        }//else
}


