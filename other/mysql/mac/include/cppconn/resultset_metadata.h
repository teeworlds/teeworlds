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

#ifndef _SQL_RESULTSET_METADATA_H_
#define _SQL_RESULTSET_METADATA_H_

#include "sqlstring.h"
#include "datatype.h"

namespace sql
{

class ResultSetMetaData
{
public:
	enum
	{
		columnNoNulls,
		columnNullable,
		columnNullableUnknown
	};

	virtual SQLString getCatalogName(unsigned int column) = 0;

	virtual unsigned int getColumnCount() = 0;

	virtual unsigned int getColumnDisplaySize(unsigned int column) = 0;

	virtual SQLString getColumnLabel(unsigned int column) = 0;

	virtual SQLString getColumnName(unsigned int column) = 0;

	virtual int getColumnType(unsigned int column) = 0;

	virtual SQLString getColumnTypeName(unsigned int column) = 0;

	virtual unsigned int getPrecision(unsigned int column) = 0;

	virtual unsigned int getScale(unsigned int column) = 0;

	virtual SQLString getSchemaName(unsigned int column) = 0;

	virtual SQLString getTableName(unsigned int column) = 0;

	virtual bool isAutoIncrement(unsigned int column) = 0;

	virtual bool isCaseSensitive(unsigned int column) = 0;

	virtual bool isCurrency(unsigned int column) = 0;

	virtual bool isDefinitelyWritable(unsigned int column) = 0;

	virtual int isNullable(unsigned int column) = 0;

	virtual bool isReadOnly(unsigned int column) = 0;

	virtual bool isSearchable(unsigned int column) = 0;

	virtual bool isSigned(unsigned int column) = 0;

	virtual bool isWritable(unsigned int column) = 0;

	virtual bool isZerofill(unsigned int column) = 0;

protected:
	virtual ~ResultSetMetaData() {}
};


} /* namespace sql */

#endif /* _SQL_RESULTSET_METADATA_H_ */
