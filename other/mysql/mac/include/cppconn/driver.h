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

#ifndef _SQL_DRIVER_H_
#define _SQL_DRIVER_H_

#include "connection.h"
#include "build_config.h"

namespace sql
{

class CPPCONN_PUBLIC_FUNC Driver
{
protected:
	virtual ~Driver() {}
public:
	// Attempts to make a database connection to the given URL.

	virtual Connection * connect(const sql::SQLString& hostName, const sql::SQLString& userName, const sql::SQLString& password) = 0;

	virtual Connection * connect(ConnectOptionsMap & options) = 0;

	virtual int getMajorVersion() = 0;

	virtual int getMinorVersion() = 0;

	virtual int getPatchVersion() = 0;

	virtual const sql::SQLString & getName() = 0;

	virtual void threadInit() = 0;

	virtual void threadEnd() = 0;
};

} /* namespace sql */

extern "C"
{
	CPPCONN_PUBLIC_FUNC sql::Driver * get_driver_instance();

  /* If dynamic loading is disabled in a driver then this function works just like get_driver_instance() */
	CPPCONN_PUBLIC_FUNC sql::Driver * get_driver_instance_by_name(const char * const clientlib);
}

#endif /* _SQL_DRIVER_H_ */
