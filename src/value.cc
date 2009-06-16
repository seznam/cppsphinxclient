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
 * Implementation of abstract class ValueBase and its subclasses - represents
 * unspecified type of attributes
 * 
 * AUTHOR
 * Jan Kirschner <jan.kirschner@firma.seznam.cz>
 *
 * HISTORY
 * 2006-12-11 (jan.kirschner)
 *            First draft.
 */


#include <sphinxclient/value.h>


namespace Sphinx {


/** @brief Abstract class, representing value data
 */
class ValueBase_t
{
protected:
    ValueType_t type;

public:
    ValueBase_t(ValueType_t t) : type(t) {}
    ValueBase_t(const ValueBase_t &v) : type(v.type) {}

    virtual ~ValueBase_t() {}

    inline ValueType_t getType() { return type; }

    virtual operator uint32_t () const = 0;
    virtual operator const std::vector<Value_t>& () const = 0;
    virtual operator float () const = 0;
    virtual operator uint64_t () const = 0;
};//class


class ValueUInt32_t : public ValueBase_t
{
protected:
    uint32_t value;

public:
    ValueUInt32_t(uint32_t val=0)
        : ValueBase_t(VALUETYPE_UINT32), value(val) {}
    ValueUInt32_t(const ValueUInt32_t &val)
        : ValueBase_t(val), value(val.value) {}

    virtual operator uint32_t () const { return value; }
    virtual operator const std::vector<Value_t>& () const {
        throw ValueTypeError_t("Value is of type uint32_t, "
                               "but requested is std::vector");
    }//konec fce
    virtual operator float () const {
        throw ValueTypeError_t("Value is of type uint32_t, "
                               "but requested is float");
    }//konec fce
    virtual operator uint64_t () const {
        throw ValueTypeError_t("Value is of type uint32_t, "
                               "but requested is uint64_t");
    }//konec fce
};//class


class ValueFloat_t : public ValueBase_t
{
protected:
    float value;

public:
    ValueFloat_t(float val=0.0f) : ValueBase_t(VALUETYPE_FLOAT), value(val) {}
    ValueFloat_t(const ValueFloat_t &val)
        : ValueBase_t(val), value(val.value) {}

    virtual operator uint32_t () const {
        throw ValueTypeError_t("Value is of type float, "
                               "but requested is uint32_t");
    }//konec fce

    virtual operator const std::vector<Value_t>& () const {
        throw ValueTypeError_t("Value is of type float, "
                               "but requested is std::vector");
    }//konec fce

    virtual operator uint64_t () const {
        throw ValueTypeError_t("Value is of type float, "
                               "but requested is uint64_t");
    }//konec fce

    virtual operator float () const { return value; }
};//class


class ValueVector_t : public ValueBase_t
{
protected:
    std::vector<Value_t> value;

public:
    ValueVector_t(const std::vector<Value_t> &v=std::vector<Value_t>())
        : ValueBase_t(VALUETYPE_VECTOR), value(v) {}
    ValueVector_t(const ValueVector_t &val)
        : ValueBase_t(val), value(val.value) {}

    virtual operator uint32_t () const {
        throw ValueTypeError_t("Value is of type std::vector, "
                               "but requested is uint32_t");
    }//konec fce

    virtual operator const std::vector<Value_t>& () const { return value; }

    virtual operator float () const {
        throw ValueTypeError_t("Value is of type std::vector, "
                               "but requested type is float");
    }//konec fce

    virtual operator uint64_t () const {
        throw ValueTypeError_t("Value is of type std::vector, "
                               "but requested is uint64_t");
    }//konec fce
};//class

class ValueUInt64_t : public ValueBase_t
{
protected:
    uint64_t value;

public:
    ValueUInt64_t(uint64_t val=0)
        : ValueBase_t(VALUETYPE_UINT64), value(val) {}
    ValueUInt64_t(const ValueUInt64_t &val)
        : ValueBase_t(val), value(val.value) {}

    virtual operator uint64_t () const { return value; }
    virtual operator const std::vector<Value_t>& () const {
        throw ValueTypeError_t("Value is of type uint64_t, "
                               "but requested is std::vector");
    }//konec fce
    virtual operator float () const {
        throw ValueTypeError_t("Value is of type uint64_t, "
                               "but requested is float");
    }//konec fce
    virtual operator uint32_t () const {
        throw ValueTypeError_t("Value is of type uint64_t, "
                               "but requested is uint32_t");
    }//konec fce
};//class

}//namespace

//----------------------------------------------------------------------
// class Value_t
//----------------------------------------------------------------------

using namespace Sphinx;

Value_t::Value_t(uint32_t v) : value(new ValueUInt32_t(v)) {}
Value_t::Value_t(float v) : value(new ValueFloat_t(v)) {}
Value_t::Value_t(const std::vector<Value_t> &v)
    : value(new ValueVector_t(v)) {}
Value_t::Value_t(uint64_t v) : value(new ValueUInt64_t(v)) {}

void Value_t::makeCopy(const Value_t &v)
{
    if (v.value) {
        switch(v.value->getType()) {
            case VALUETYPE_UINT32:
                value = new ValueUInt32_t(*(ValueUInt32_t*)v.value);
                break;
            case VALUETYPE_FLOAT:
                value = new ValueFloat_t(*(ValueFloat_t*)v.value);
                break;
            case VALUETYPE_VECTOR:
                value = new ValueVector_t(*(ValueVector_t*)v.value);
                break;
            case VALUETYPE_UINT64:
                value = new ValueUInt64_t(*(ValueUInt64_t*)v.value);
                break;
        }//switch
    }//if
    else
        value = 0;
}//konec fce

void Value_t::clear()
{
    delete value;
    value = 0;
}//konec fce

Value_t & Value_t::operator = (const Value_t &v)
{
    //preserve assigning the same object
    if (&v != this) {
        clear();
        makeCopy(v);
    }//if

    return *this;
}//operator

ValueType_t Value_t::getValueType() const { return value->getType(); }

Value_t::operator uint32_t () const throw (ValueTypeError_t)
{
    return (uint32_t)(*value);
}

Value_t::operator const std::vector<Value_t>& () const throw (ValueTypeError_t)
{
    return (const std::vector<Value_t>&)(*value);
}

Value_t::operator float () const throw (ValueTypeError_t)
{
    return (float)(*value);
}

Value_t::operator uint64_t () const throw (ValueTypeError_t)
{
    return (uint64_t)(*value);
}

