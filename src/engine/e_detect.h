/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef BASELIB_FILE_CONFIG_H
#define BASELIB_FILE_CONFIG_H

/*
  this file detected the family, platform and architecture
  to compile for.
*/

/* platforms */

/* windows Family */
#if defined(WIN64) || defined(_WIN64)
	/* Hmm, is this IA64 or x86-64? */
	#define CONF_FAMILY_WINDOWS 1
	#define CONF_FAMILY_STRING "windows"
	#define CONF_PLATFORM_WIN64 1
	#define CONF_PLATFORM_STRING "win64"
#elif defined(WIN32) || defined(_WIN32) || defined(__CYGWIN32__) || defined(__MINGW32__)
	#define CONF_FAMILY_WINDOWS 1
	#define CONF_FAMILY_STRING "windows"
	#define CONF_PLATFORM_WIN32 1
	#define CONF_PLATFORM_STRING "win32"
#endif

/* unix family */
#if defined(__FreeBSD__)
	#define CONF_FAMILY_UNIX 1
	#define CONF_FAMILY_STRING "unix"
	#define CONF_PLATFORM_FREEBSD 1
	#define CONF_PLATFORM_STRING "freebsd"
#endif

#if defined(__OpenBSD__)
	#define CONF_FAMILY_UNIX 1
	#define CONF_FAMILY_STRING "unix"
	#define CONF_PLATFORM_OPENBSD 1
	#define CONF_PLATFORM_STRING "openbsd"
#endif

#if defined(__LINUX__) || defined(__linux__)
	#define CONF_FAMILY_UNIX 1
	#define CONF_FAMILY_STRING "unix"
	#define CONF_PLATFORM_LINUX 1
	#define CONF_PLATFORM_STRING "linux"
#endif

#if defined(MACOSX) || defined(__APPLE__) || defined(__DARWIN__)
	#define CONF_FAMILY_UNIX 1
	#define CONF_FAMILY_STRING "unix"
	#define CONF_PLATFORM_MACOSX 1
	#define CONF_PLATFORM_STRING "macosx"
#endif

#if defined(__sun)
	#define CONF_FAMILY_UNIX 1
	#define CONF_FAMILY_STRING "unix"
	#define CONF_PLATFROM_SOLARIS 1
	#define CONF_PLATFORM_STRING "solaris"
#endif

/* beos family */
#if defined(__BeOS) || defined(__BEOS__)
	#define CONF_FAMILY_BEOS 1
	#define CONF_FAMILY_STRING "beos"
	#define CONF_PLATFORM_BEOS 1
	#define CONF_PLATFORM_STRING "beos"
#endif


/* architectures */
#if defined(i386) || defined(__i386__) || defined(__x86__) || defined(CONF_PLATFORM_WIN32)
	#define CONF_ARCH_IA32 1
	#define CONF_ARCH_STRING "ia32"
	#define CONF_ARCH_ENDIAN_LITTLE 1
#endif

#if defined(__ia64__)
	#define CONF_ARCH_IA64 1
	#define CONF_ARCH_STRING "ia64"
	#define CONF_ARCH_ENDIAN_LITTLE 1
#endif

#if defined(__amd64__) || defined(__x86_64__)
	#define CONF_ARCH_AMD64 1
	#define CONF_ARCH_STRING "amd64"
	#define CONF_ARCH_ENDIAN_LITTLE 1
#endif

#if defined(__powerpc__) || defined(__ppc__)
	#define CONF_ARCH_PPC 1
	#define CONF_ARCH_STRING "ppc"
	#define CONF_ARCH_ENDIAN_BIG 1
#endif

#if defined(__sparc__)
	#define CONF_ARCH_SPARC 1
	#define CONF_ARCH_STRING "sparc"
	#define CONF_ARCH_ENDIAN_BIG 1
#endif


#ifndef CONF_FAMILY_STRING
#define CONF_FAMILY_STRING "unknown"
#endif

#ifndef CONF_PLATFORM_STRING
#define CONF_PLATFORM_STRING "unknown"
#endif

#ifndef CONF_ARCH_STRING
#define CONF_ARCH_STRING "unknown"
#endif

#endif
