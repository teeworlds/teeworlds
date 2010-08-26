/*
   Copyright 2007 - 2008 MySQL AB, 2008 - 2009 Sun Microsystems, Inc.  All rights reserved.

   The MySQL Connector/C++ is licensed under the terms of the GPL
   <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>, like most
   MySQL Connectors. There are special exceptions to the terms and
   conditions of the GPL as it is applied to this software, see the
   FLOSS License Exception
   <http://www.mysql.com/about/legal/licensing/foss-exception.html>.
*/


#ifndef _SQL_PREPARED_STATEMENT_H_
#define _SQL_PREPARED_STATEMENT_H_

#include <iostream>
#include "statement.h"


namespace sql
{

class Connection;
class ResultSet;
class ResultSetMetaData;
class ParameterMetaData;

class PreparedStatement : public Statement
{
public:
	virtual ~PreparedStatement() {}

	virtual void clearParameters() = 0;

	virtual bool execute(const std::string& sql) = 0;
	virtual bool execute() = 0;

	virtual ResultSet *executeQuery(const std::string& sql) = 0;
	virtual ResultSet *executeQuery() = 0;

	virtual int executeUpdate(const std::string& sql) = 0;
	virtual int executeUpdate() = 0;

	virtual ResultSetMetaData * getMetaData() = 0;

	virtual ParameterMetaData * getParameterMetaData() = 0;

	virtual void setBigInt(unsigned int parameterIndex, const std::string& value) = 0;

	virtual void setBlob(unsigned int parameterIndex, std::istream * blob) = 0;

	virtual void setBoolean(unsigned int parameterIndex, bool value) = 0;

	virtual void setDateTime(unsigned int parameterIndex, const std::string& value) = 0;

	virtual void setDouble(unsigned int parameterIndex, double value) = 0;

	virtual void setInt(unsigned int parameterIndex, int32_t value) = 0;

	virtual void setUInt(unsigned int parameterIndex, uint32_t value) = 0;

	virtual void setInt64(unsigned int parameterIndex, int64_t value) = 0;

	virtual void setUInt64(unsigned int parameterIndex, uint64_t value) = 0;

	virtual void setNull(unsigned int parameterIndex, int sqlType) = 0;

	virtual void setString(unsigned int parameterIndex, const std::string& value) = 0;

	virtual PreparedStatement * setResultSetType(sql::ResultSet::enum_type type) = 0;
};


} /* namespace sql */

#endif /* _SQL_PREPARED_STATEMENT_H_ */
