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
 * SphinxClient header file - error handling
 *
 * AUTHOR
 * Jan Kirschner <jan.kirschner@firma.seznam.cz>
 *
 * HISTORY
 * 2006-01-03 (jan.kirschner)
 *            First draft.
 */

//! @file error.h

#ifndef __SPHINXERROR_H__
#define __SPHINXERROR_H__

#include <string>
#include <exception>

#include <sstream>
#include <errno.h>
namespace Sphinx
{

// --------------------- exception handling ---------------------------

/** @brief Exception modes, thrown by SphinxClient_t::methods
  *
  * @see Error_t
  * @see ServerError_t
  * @see ConnectionError_t
  * @see MessageError_t
  * @see Client_t
  */

enum ErrorType_t {
    STATUS_OK = 0,        //!< @brief everything OK
    CONNECTION_ERROR = 1, //!< @brief unable to create connection to the searchd
    SERVER_ERROR = 2,   //!< @brief communication with searchd failed (version ?)
    MESSAGE_ERROR = 3,    //!< @brief received invalid message format (short ?)
    VALUE_TYPE_ERROR = 4,  //!< @brief expected another type of attribute value
    CLIENT_USAGE_ERROR = 5 //!< @brief invalid combination of function call
};

/** @brief Exception type thrown by Sphinx::Client_t::methods
  *
  * contains error code (see SCErrorType) and a brief error description
  *
  * @see ErrorType_t
  * @see Client_t
  */

struct Error_t : public std::exception
{
    std::string errMsg;
    ErrorType_t errCode;

    Error_t(ErrorType_t err, const std::string &msg)
                       :std::exception(),errMsg(msg),errCode(err)
    {}

    virtual const char* what() const throw() {
        return errMsg.c_str();
    }

    virtual ~Error_t() throw(){}
};//struct


/** @brief Exception is thrown when an search server error occured (invalid
  *        version).
  * @see Error_t
  */

struct ServerError_t : public Error_t
{
    ServerError_t(const std::string &msg)
                       :Error_t(SERVER_ERROR, msg) {}
};//struct

/** @brief Exception is thrown when unable to parse response from the
  *        search server.
  * @see Error_t
  */

struct MessageError_t : public Error_t
{
    MessageError_t(const std::string &msg)
                       :Error_t(MESSAGE_ERROR, msg) {}
};//struct

/** @brief Exception is thrown when an error occured during connecting to the
  *        search server.
  * @see Error_t
  */

struct ConnectionError_t : public Error_t
{
    ConnectionError_t(const std::string &msg)
                       :Error_t(CONNECTION_ERROR, msg) {}
};//struct


/** @brief Exception is thrown when a value is being read from Value_t
  *        and the expected type does not match value type
  * @see Error_t
  */

struct ValueTypeError_t : public Error_t
{
    ValueTypeError_t(const std::string &msg)
                       :Error_t(VALUE_TYPE_ERROR, msg) {}
};//struct


/** @brief Exception is thrown when a invalid combination of function calls
  *        occurs. For ex. addQuery call before initQuery call.
  * @see Error_t
  */

struct ClientUsageError_t : public Error_t
{
    ClientUsageError_t(const std::string &msg)
                       :Error_t(CLIENT_USAGE_ERROR, msg) {}
};//struct


inline std::string strError(const char *msg, int errNo=0) {
        std::ostringstream errMsg;
        errMsg << msg
               << ": "
               << (errNo == 0 ? strerror(errno) : strerror(errNo));
        return errMsg.str();
}

}

#endif

