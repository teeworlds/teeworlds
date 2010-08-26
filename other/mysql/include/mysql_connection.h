/*
   Copyright 2007 - 2008 MySQL AB, 2008 - 2009 Sun Microsystems, Inc.  All rights reserved.

   The MySQL Connector/C++ is licensed under the terms of the GPL
   <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>, like most
   MySQL Connectors. There are special exceptions to the terms and
   conditions of the GPL as it is applied to this software, see the
   FLOSS License Exception
   <http://www.mysql.com/about/legal/licensing/foss-exception.html>.
*/

#ifndef _MYSQL_CONNECTION_H_
#define _MYSQL_CONNECTION_H_

#include <cppconn/connection.h>
struct st_mysql;


namespace sql
{
namespace mysql
{

class MySQL_Savepoint : public sql::Savepoint
{
	std::string name;

public:
	MySQL_Savepoint(const std::string &savepoint);
	virtual ~MySQL_Savepoint() {}

	int getSavepointId();

	std::string getSavepointName();

private:
	/* Prevent use of these */
	MySQL_Savepoint(const MySQL_Savepoint &);
	void operator=(MySQL_Savepoint &);
};


class MySQL_DebugLogger;
class MySQL_ConnectionData; /* PIMPL */

class CPPCONN_PUBLIC_FUNC MySQL_Connection : public sql::Connection
{
public:
	MySQL_Connection(const std::string& hostName, const std::string& userName, const std::string& password);

	MySQL_Connection(std::map< std::string, sql::ConnectPropertyVal > & options);

	virtual ~MySQL_Connection();

	struct ::st_mysql * getMySQLHandle();

	void clearWarnings();

	void close();

	void commit();

	sql::Statement * createStatement();

	bool getAutoCommit();

	std::string getCatalog();

	std::string getSchema();

	std::string getClientInfo();

	void getClientOption(const std::string & optionName, void * optionValue);

	sql::DatabaseMetaData * getMetaData();

	enum_transaction_isolation getTransactionIsolation();

	const SQLWarning * getWarnings();

	bool isClosed();

	std::string nativeSQL(const std::string& sql);

	sql::PreparedStatement * prepareStatement(const std::string& sql);

	void releaseSavepoint(Savepoint * savepoint) ;

	void rollback();

	void rollback(Savepoint * savepoint);

	void setAutoCommit(bool autoCommit);

	void setCatalog(const std::string& catalog);

	void setSchema(const std::string& catalog);

	sql::Connection * setClientOption(const std::string & optionName, const void * optionValue);

	sql::Savepoint * setSavepoint(const std::string& name);

	void setTransactionIsolation(enum_transaction_isolation level);

	std::string getSessionVariable(const std::string & varname);

	void setSessionVariable(const std::string & varname, const std::string & value);

protected:
	void checkClosed();
	void init(std::map<std::string, sql::ConnectPropertyVal> & properties);

	MySQL_ConnectionData * intern; /* pimpl */

private:
	/* Prevent use of these */
	MySQL_Connection(const MySQL_Connection &);
	void operator=(MySQL_Connection &);
};

} /* namespace mysql */
} /* namespace sql */

#endif // _MYSQL_CONNECTION_H_

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
