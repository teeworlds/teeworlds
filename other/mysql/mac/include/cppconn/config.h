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

#define HAVE_FUNCTION_STRTOLD 1
#define HAVE_FUNCTION_STRTOLL 1
#define HAVE_FUNCTION_STRTOL 1
#define HAVE_FUNCTION_STRTOULL 1

#define HAVE_FUNCTION_STRTOUL 1

#define HAVE_FUNCTION_STRTOIMAX 1
#define HAVE_FUNCTION_STRTOUMAX 1

#define HAVE_STDINT_H 1
#define HAVE_INTTYPES_H 1

#define HAVE_INT8_T   1
#define HAVE_UINT8_T  1
#define HAVE_INT16_T  1
#define HAVE_UINT16_T 1
#define HAVE_INT32_T  1
#define HAVE_UINT32_T 1
#define HAVE_INT32_T  1
#define HAVE_UINT32_T 1
#define HAVE_INT64_T  1
#define HAVE_UINT64_T 1
/* #undef HAVE_MS_INT8 */
/* #undef HAVE_MS_UINT8 */
/* #undef HAVE_MS_INT16 */
/* #undef HAVE_MS_UINT16 */
/* #undef HAVE_MS_INT32 */
/* #undef HAVE_MS_UINT32 */
/* #undef HAVE_MS_INT64 */
/* #undef HAVE_MS_UINT64 */


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
