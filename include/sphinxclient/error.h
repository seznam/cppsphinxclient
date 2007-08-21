/*
 * Copyright (c) 2006, Seznam.cz, a.s.
 * All Rights Reserved.
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
    MESSAGE_ERROR = 3     //!< @brief received invalid message format (short ?)
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


inline std::string strError(const char *msg, int errNo=0) {
        std::ostringstream errMsg;
        errMsg << msg
               << ": "
               << (errNo == 0 ? strerror(errno) : strerror(errNo));
        return errMsg.str();
}

}

#endif

