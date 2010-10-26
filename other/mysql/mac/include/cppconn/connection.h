/*
   Copyright 2007 - 2008 MySQL AB, 2008 - 2009 Sun Microsystems, Inc.  All rights reserved.

   The MySQL Connector/C++ is licensed under the terms of the GPL
   <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>, like most
   MySQL Connectors. There are special exceptions to the terms and
   conditions of the GPL as it is applied to this software, see the
   FLOSS License Exception
   <http://www.mysql.com/about/legal/licensing/foss-exception.html>.
*/

#ifndef _SQL_CONNECTION_H_
#define _SQL_CONNECTION_H_

#include <string>
#include <map>

#include "build_config.h"
#include "warning.h"

namespace sql
{

typedef union _ConnectPropertyVal
{
		struct
		{
			const char * val;
			size_t len;
		} str;
		double dval;
		long long lval;
		bool bval;
		void * pval;
} ConnectPropertyVal;


class DatabaseMetaData;
class PreparedStatement;
class Statement;

typedef enum transaction_isolation
{
	TRANSACTION_NONE= 0,
	TRANSACTION_READ_COMMITTED,
	TRANSACTION_READ_UNCOMMITTED,
	TRANSACTION_REPEATABLE_READ,
	TRANSACTION_SERIALIZABLE
} enum_transaction_isolation;

class Savepoint
{
	/* Prevent use of these */
	Savepoint(const Savepoint &);
	void operator=(Savepoint &);
public:
	Savepoint() {};
	virtual ~Savepoint() {};
	virtual int getSavepointId() = 0;

	virtual std::string getSavepointName() = 0;
};


class CPPCONN_PUBLIC_FUNC Connection
{
	/* Prevent use of these */
	Connection(const Connection &);
	void operator=(Connection &);
public:

	Connection() {};

	virtual ~Connection() {};

	virtual void clearWarnings() = 0;

	virtual Statement *createStatement() = 0;

	virtual void close()  = 0;

	virtual void commit() = 0;

	virtual bool getAutoCommit() = 0;

	virtual std::string getCatalog() = 0;

	virtual std::string getSchema() = 0;

	virtual std::string getClientInfo() = 0;

	virtual void getClientOption(const std::string & optionName, void * optionValue) = 0;

	/* virtual int getHoldability() = 0; */

	/* virtual std::map getTypeMap() = 0; */

	virtual DatabaseMetaData * getMetaData() = 0;

	virtual enum_transaction_isolation getTransactionIsolation() = 0;

	virtual const SQLWarning * getWarnings() = 0;

	virtual bool isClosed() = 0;

	virtual std::string nativeSQL(const std::string& sql) = 0;

	virtual PreparedStatement * prepareStatement(const std::string& sql) = 0;

	virtual void releaseSavepoint(Savepoint * savepoint) = 0;

	virtual void rollback() = 0;

	virtual void rollback(Savepoint * savepoint) = 0;

	virtual void setAutoCommit(bool autoCommit) = 0;

	virtual void setCatalog(const std::string& catalog) = 0;

	virtual void setSchema(const std::string& catalog) = 0;

	virtual sql::Connection * setClientOption(const std::string & optionName, const void * optionValue) = 0;

	virtual Savepoint * setSavepoint(const std::string& name) = 0;

	virtual void setTransactionIsolation(enum_transaction_isolation level) = 0;

	/* virtual void setTypeMap(Map map) = 0; */
};

} /* namespace sql */

#endif // _SQL_CONNECTION_H_
