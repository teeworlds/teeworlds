/*
   Copyright 2007 - 2008 MySQL AB, 2008 - 2009 Sun Microsystems, Inc.  All rights reserved.

   The MySQL Connector/C++ is licensed under the terms of the GPL
   <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>, like most
   MySQL Connectors. There are special exceptions to the terms and
   conditions of the GPL as it is applied to this software, see the
   FLOSS License Exception
   <http://www.mysql.com/about/legal/licensing/foss-exception.html>.
*/

#ifndef _SQL_WARNING_H_
#define _SQL_WARNING_H_


#include <stdexcept>
#include <string>
#include <memory>

namespace sql
{

#ifdef _WIN32
#pragma warning (disable : 4290)
//warning C4290: C++ exception specification ignored except to indicate a function is not __declspec(nothrow)
#endif

class SQLWarning
{
protected:

	const std::string	sql_state;
	const int			errNo;
	SQLWarning *		next;
	const std::string	descr;

public:

	SQLWarning(const std::string& reason, const std::string& SQLState, int vendorCode) :sql_state(SQLState), errNo(vendorCode),descr(reason)
	{
	}

	SQLWarning(const std::string& reason, const std::string& SQLState) :sql_state (SQLState), errNo(0), descr(reason)
	{
	}

	SQLWarning(const std::string& reason) : sql_state ("HY000"), errNo(0), descr(reason)
	{
	}

	SQLWarning() : sql_state ("HY000"), errNo(0) {}


	const std::string & getMessage() const
	{
		return descr;
	}


	const std::string & getSQLState() const
	{
		return sql_state;
	}

	int getErrorCode() const
	{
		return errNo;
	}

	const SQLWarning * getNextWarning() const
	{
		return next;
	}

	void setNextWarning(SQLWarning * _next)
	{
		next = _next;
	}

	virtual ~SQLWarning() throw () {};

protected:

	SQLWarning(const SQLWarning& e) : sql_state(e.sql_state), errNo(e.errNo), next(e.next), descr(e.descr) {}

	virtual SQLWarning * copy()
	{
		return new SQLWarning(*this);
	}

private:
	const SQLWarning & operator = (const SQLWarning & rhs);

};


} /* namespace sql */

#endif /* _SQL_WARNING_H_ */
