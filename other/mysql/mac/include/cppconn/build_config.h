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

#ifndef _SQL_BUILD_CONFIG_H_
#define _SQL_BUILD_CONFIG_H_

#ifndef CPPCONN_PUBLIC_FUNC

#if defined(_WIN32)
 // mysqlcppconn_EXPORTS is added by cmake and defined for dynamic lib build only
  #ifdef mysqlcppconn_EXPORTS
    #define CPPCONN_PUBLIC_FUNC __declspec(dllexport)
  #else
    // this is for static build
    #ifdef CPPCONN_LIB_BUILD
      #define CPPCONN_PUBLIC_FUNC
    #else
      // this is for clients using dynamic lib
      #define CPPCONN_PUBLIC_FUNC __declspec(dllimport)
    #endif
  #endif
#else
  #define CPPCONN_PUBLIC_FUNC
#endif

#endif    //#ifndef CPPCONN_PUBLIC_FUNC

#endif    //#ifndef _SQL_BUILD_CONFIG_H_
