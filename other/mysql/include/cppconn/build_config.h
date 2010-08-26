/*
   Copyright 2007 - 2008 MySQL AB, 2008 - 2009 Sun Microsystems, Inc.  All rights reserved.

   The MySQL Connector/C++ is licensed under the terms of the GPL
   <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>, like most
   MySQL Connectors. There are special exceptions to the terms and
   conditions of the GPL as it is applied to this software, see the
   FLOSS License Exception
   <http://www.mysql.com/about/legal/licensing/foss-exception.html>.
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
