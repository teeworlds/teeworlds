/*
   Copyright 2007 - 2008 MySQL AB, 2008 - 2009 Sun Microsystems, Inc.  All rights reserved.

   The MySQL Connector/C++ is licensed under the terms of the GPL
   <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>, like most
   MySQL Connectors. There are special exceptions to the terms and
   conditions of the GPL as it is applied to this software, see the
   FLOSS License Exception
   <http://www.mysql.com/about/legal/licensing/foss-exception.html>.
*/

#ifndef _SQL_RESULTSET_H_
#define _SQL_RESULTSET_H_

#include "config.h"

#include <list>
#include <string>
#include <map>
#include <iostream>
#include "resultset_metadata.h"


namespace sql
{

class Statement;

class RowID
{
public:
	virtual ~RowID() {}
};

class ResultSet
{
public:
	enum
	{
		CLOSE_CURSORS_AT_COMMIT,
		HOLD_CURSORS_OVER_COMMIT
	};
	enum
	{
		CONCUR_READ_ONLY,
		CONCUR_UPDATABLE
	};
	enum
	{
		FETCH_FORWARD,
		FETCH_REVERSE,
		FETCH_UNKNOWN
	};
	typedef enum
	{
		TYPE_FORWARD_ONLY,
		TYPE_SCROLL_INSENSITIVE,
		TYPE_SCROLL_SENSITIVE
	} enum_type;

	virtual ~ResultSet() {}

	virtual bool absolute(int row) = 0;

	virtual void afterLast() = 0;

	virtual void beforeFirst() = 0;

	virtual void close() = 0;

	virtual uint32_t findColumn(const std::string& columnLabel) const = 0;

	virtual bool first() = 0;

	virtual std::istream * getBlob(uint32_t columnIndex)  const = 0;
	virtual std::istream * getBlob(const std::string& columnLabel) const = 0;

	virtual bool getBoolean(uint32_t columnIndex) const = 0;
	virtual bool getBoolean(const std::string& columnLabel) const = 0;

	virtual long double getDouble(uint32_t columnIndex) const = 0;
	virtual long double getDouble(const std::string& columnLabel) const = 0;

	virtual int32_t getInt(uint32_t columnIndex) const = 0;
	virtual int32_t getInt(const std::string& columnLabel) const = 0;

	virtual uint32_t getUInt(uint32_t columnIndex) const = 0;
	virtual uint32_t getUInt(const std::string& columnLabel) const = 0;

	virtual int64_t getInt64(uint32_t columnIndex) const = 0;
	virtual int64_t getInt64(const std::string& columnLabel) const = 0;

	virtual uint64_t getUInt64(uint32_t columnIndex) const = 0;
	virtual uint64_t getUInt64(const std::string& columnLabel) const = 0;

	virtual ResultSetMetaData * getMetaData() const = 0;

	virtual size_t getRow() const = 0;

	virtual const Statement * getStatement() const = 0;

	virtual std::string getString(uint32_t columnIndex)  const = 0;
	virtual std::string getString(const std::string& columnLabel) const = 0;

	virtual enum_type getType() const = 0;

	virtual bool isAfterLast() const = 0;

	virtual bool isBeforeFirst() const = 0;

	virtual bool isClosed() const = 0;

	virtual bool isFirst() const = 0;

	virtual bool isLast() const = 0;

	virtual bool isNull(uint32_t columnIndex) const = 0;
	virtual bool isNull(const std::string& columnLabel) const = 0;

	virtual bool last() = 0;

	virtual bool next() = 0;

	virtual bool previous() = 0;

	virtual bool relative(int rows) = 0;

	virtual size_t rowsCount() const = 0;

	virtual bool wasNull() const = 0;
};

} /* namespace sql */

#endif /* _SQL_RESULTSET_H_ */
