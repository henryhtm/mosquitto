#ifndef CONFIG_H
#define CONFIG_H
/* ============================================================
 * Platform options
 * ============================================================ */

//#define FOR_CODE_READING  /* 仅供代码阅读和分析时使用，编译时必须关闭该宏 */
#ifdef FOR_CODE_READING
/* 没在项目代码文件中显式定义的宏没办法被代码编辑工具识别到，需要手工将宏添加到这里 */
#define WITH_TLS
#define WITH_TLS_PSK
#define WITH_THREADING
#define WITH_SOCKS
#define WITH_BROKER
#define WITH_MEMORY_TRACKING
#define WITH_PERSISTENCE
#define WITH_SYS_TREE
#define __GLIBC__
#include "Error:Please undefine FOR_CODE_READING."
#endif

#ifdef __APPLE__
#  define __DARWIN_C_SOURCE
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__SYMBIAN32__) || defined(__QNX__)
#  define _XOPEN_SOURCE 700
#  define __BSD_VISIBLE 1
#  define HAVE_NETINET_IN_H
#else
#  define _XOPEN_SOURCE 700
#  define _DEFAULT_SOURCE 1
#  define _POSIX_C_SOURCE 200809L
#endif


#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#define OPENSSL_LOAD_CONF

/* ============================================================
 * Compatibility defines
 * ============================================================ */
#if defined(_MSC_VER) && _MSC_VER < 1900
#  define snprintf sprintf_s
#  define EPROTO ECONNABORTED
#endif

#ifdef WIN32
#  ifndef strcasecmp
#    define strcasecmp strcmpi
#  endif
#  define strtok_r strtok_s
#  define strerror_r(e, b, l) strerror_s(b, l, e)
#endif


#define uthash_malloc(sz) mosquitto__malloc(sz)
#define uthash_free(ptr,sz) mosquitto__free(ptr)


#ifdef WITH_TLS
#  include <openssl/opensslconf.h>
#  if defined(WITH_TLS_PSK) && !defined(OPENSSL_NO_PSK)
#    define FINAL_WITH_TLS_PSK
#  endif
#endif


#ifdef __COVERITY__
#  include <stdint.h>
/* These are "wrong", but we don't use them so it doesn't matter */
#  define _Float32 uint32_t
#  define _Float32x uint32_t
#  define _Float64 uint64_t
#  define _Float64x uint64_t
#  define _Float128 uint64_t
#endif

#define UNUSED(A) (void)(A)

/* Android Bionic libpthread implementation doesn't have pthread_cancel */
#ifndef ANDROID
#  define HAVE_PTHREAD_CANCEL
#endif

#endif
