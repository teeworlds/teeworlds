/*
   Copyright 2009 Sun Microsystems, Inc.  All rights reserved.

   The MySQL Connector/C++ is licensed under the terms of the GPL
   <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>, like most
   MySQL Connectors. There are special exceptions to the terms and
   conditions of the GPL as it is applied to this software, see the
   FLOSS License Exception
   <http://www.mysql.com/about/legal/licensing/foss-exception.html>.
*/

// libmysql defines HAVE_STRTOUL (on win), so we have to follow different pattern in definitions names
// to avoid annoying warnings.

/* #undef HAVE_FUNCTION_STRTOLD */
/* #undef HAVE_FUNCTION_STRTOLL */
#define HAVE_FUNCTION_STRTOL 1
/* #undef HAVE_FUNCTION_STRTOULL */

#define HAVE_FUNCTION_STRTOUL 1

/* #undef HAVE_FUNCTION_STRTOIMAX */
/* #undef HAVE_FUNCTION_STRTOUMAX */

/* #undef HAVE_STDINT_H */
/* #undef HAVE_INTTYPES_H */

/* #undef HAVE_INT8_T */
/* #undef HAVE_UINT8_T */
/* #undef HAVE_INT16_T */
/* #undef HAVE_UINT16_T */
/* #undef HAVE_INT32_T */
/* #undef HAVE_UINT32_T */
/* #undef HAVE_INT32_T */
/* #undef HAVE_UINT32_T */
/* #undef HAVE_INT64_T */
/* #undef HAVE_UINT64_T */
#define HAVE_MS_INT8     1
#define HAVE_MS_UINT8    1
#define HAVE_MS_INT16    1
#define HAVE_MS_UINT16   1
#define HAVE_MS_INT32    1
#define HAVE_MS_UINT32   1
#define HAVE_MS_INT64   1
#define HAVE_MS_UINT64   1


#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#ifndef CPPCONN_DONT_TYPEDEF_MS_TYPES_TO_C99_TYPES

#ifdef HAVE_MS_INT8
typedef __int8			int8_t;
#endif

#ifdef HAVE_MS_UINT8
typedef unsigned __int8	uint8_t;
#endif
#ifdef HAVE_MS_INT16
typedef __int16			int16_t;
#endif

#ifdef HAVE_MS_UINT16
typedef unsigned __int16	uint16_t;
#endif

#ifdef HAVE_MS_INT32
typedef __int32			int32_t;
#endif

#ifdef HAVE_MS_UINT32
typedef unsigned __int32	uint32_t;
#endif

#ifdef HAVE_MS_INT64
typedef __int64			int64_t;
#endif
#ifdef HAVE_MS_UINT64
typedef unsigned __int64	uint64_t;
#endif

#endif	// CPPCONN_DONT_TYPEDEF_MS_TYPES_TO_C99_TYPES
#endif	//	_WIN32
