/*
   Copyright 2007 - 2008 MySQL AB, 2008 - 2009 Sun Microsystems, Inc.  All rights reserved.

   The MySQL Connector/C++ is licensed under the terms of the GPL
   <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>, like most
   MySQL Connectors. There are special exceptions to the terms and
   conditions of the GPL as it is applied to this software, see the
   FLOSS License Exception
   <http://www.mysql.com/about/legal/licensing/foss-exception.html>.
*/

#ifndef _SQL_RESULTSET_METADATA_H_
#define _SQL_RESULTSET_METADATA_H_

#include <string>
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

	virtual std::string getCatalogName(unsigned int column) = 0;

	virtual unsigned int getColumnCount() = 0;

	virtual unsigned int getColumnDisplaySize(unsigned int column) = 0;

	virtual std::string getColumnLabel(unsigned int column) = 0;

	virtual std::string getColumnName(unsigned int column) = 0;

	virtual int getColumnType(unsigned int column) = 0;

	virtual std::string getColumnTypeName(unsigned int column) = 0;

	virtual unsigned int getPrecision(unsigned int column) = 0;

	virtual unsigned int getScale(unsigned int column) = 0;

	virtual std::string getSchemaName(unsigned int column) = 0;

	virtual std::string getTableName(unsigned int column) = 0;

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
