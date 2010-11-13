/*
  Copyright (c) 2008, 2010, Oracle and/or its affiliates. All rights reserved.

  The MySQL Connector/C++ is licensed under the terms of the GPLv2
  <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>, like most
  MySQL Connectors. There are special exceptions to the terms and
  conditions of the GPLv2 as it is applied to this software, see the
  FLOSS License Exception
  <http://www.mysql.com/about/legal/licensing/foss-exception.html>.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef _SQL_RESULTSET_H_
#define _SQL_RESULTSET_H_

#include "config.h"

#include <list>
#include <map>
#include <iostream>
#include "sqlstring.h"
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

	virtual void cancelRowUpdates() = 0;

	virtual void clearWarnings() = 0;

	virtual void close() = 0;

	virtual uint32_t findColumn(const sql::SQLString& columnLabel) const = 0;

	virtual bool first() = 0;

	virtual std::istream * getBlob(uint32_t columnIndex)  const = 0;
	virtual std::istream * getBlob(const sql::SQLString& columnLabel) const = 0;

	virtual bool getBoolean(uint32_t columnIndex) const = 0;
	virtual bool getBoolean(const sql::SQLString& columnLabel) const = 0;

	virtual int getConcurrency() = 0;
	virtual SQLString getCursorName() = 0;

	virtual long double getDouble(uint32_t columnIndex) const = 0;
	virtual long double getDouble(const sql::SQLString& columnLabel) const = 0;

	virtual int getFetchDirection() = 0;
	virtual size_t getFetchSize() = 0;
	virtual int getHoldability() = 0;

	virtual int32_t getInt(uint32_t columnIndex) const = 0;
	virtual int32_t getInt(const sql::SQLString& columnLabel) const = 0;

	virtual uint32_t getUInt(uint32_t columnIndex) const = 0;
	virtual uint32_t getUInt(const sql::SQLString& columnLabel) const = 0;

	virtual int64_t getInt64(uint32_t columnIndex) const = 0;
	virtual int64_t getInt64(const sql::SQLString& columnLabel) const = 0;

	virtual uint64_t getUInt64(uint32_t columnIndex) const = 0;
	virtual uint64_t getUInt64(const sql::SQLString& columnLabel) const = 0;

	virtual ResultSetMetaData * getMetaData() const = 0;

	virtual size_t getRow() const = 0;

	virtual RowID * getRowId(uint32_t columnIndex) = 0;
	virtual RowID * getRowId(const sql::SQLString & columnLabel) = 0;

	virtual const Statement * getStatement() const = 0;

	virtual SQLString getString(uint32_t columnIndex)  const = 0;
	virtual SQLString getString(const sql::SQLString& columnLabel) const = 0;

	virtual enum_type getType() const = 0;

	virtual void getWarnings() = 0;

	virtual void insertRow() = 0;

	virtual bool isAfterLast() const = 0;

	virtual bool isBeforeFirst() const = 0;

	virtual bool isClosed() const = 0;

	virtual bool isFirst() const = 0;

	virtual bool isLast() const = 0;

	virtual bool isNull(uint32_t columnIndex) const = 0;
	virtual bool isNull(const sql::SQLString& columnLabel) const = 0;

	virtual bool last() = 0;

	virtual bool next() = 0;

	virtual void moveToCurrentRow() = 0;

	virtual void moveToInsertRow() = 0;

	virtual bool previous() = 0;

	virtual void refreshRow() = 0;

	virtual bool relative(int rows) = 0;

	virtual bool rowDeleted() = 0;

	virtual bool rowInserted() = 0;

	virtual bool rowUpdated() = 0;

	virtual void setFetchSize(size_t rows) = 0;

	virtual size_t rowsCount() const = 0;

	virtual bool wasNull() const = 0;
};

} /* namespace sql */

#endif /* _SQL_RESULTSET_H_ */
