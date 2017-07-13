/**
 * @file        pifs_debug.h
 * @brief       Common definitions
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-16 13:12:44 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_PIFS_DEBUG_H_
#define _INCLUDE_PIFS_DEBUG_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "common.h"

#define PIFS_ASSERT(expression) do {                                                    \
        if (!((expression)))                                                            \
        {                                                                               \
            fprintf(stderr, "ERROR: Assertion '%s' failed. File: %s, line: %i\r\n",     \
                    TOSTR(expression),                                                  \
                    __FILE__, __LINE__                                                  \
                   );                                                                  \
            exit(-1);                                                                   \
        }                                                                               \
    } while (0);

#if (PIFS_DEBUG_LEVEL >= 1)
#define PIFS_ERROR_MSG(...)    do { \
        printf("%s ERROR: ", __FUNCTION__); \
        printf(__VA_ARGS__); \
        fflush(stdout); \
    } while (0);
#else
#define PIFS_ERROR_MSG(...)
#endif

#if (PIFS_DEBUG_LEVEL >= 1)
#define PIFS_FATAL_ERROR_MSG(...)    do { \
        printf("%s FATAL_ERROR: ", __FUNCTION__); \
        printf( __VA_ARGS__); \
        exit(-1); \
    } while (0);
#else
#define PIFS_FATAL_ERROR_MSG(...)
#endif

#if (PIFS_DEBUG_LEVEL >= 2)
#define PIFS_WARNING_MSG(...)    do { \
        printf("%s WARNING: ", __FUNCTION__); \
        printf(__VA_ARGS__); \
        fflush(stdout); \
    } while (0);
#else
#define PIFS_WARNING_MSG(...)
#endif

#if (PIFS_DEBUG_LEVEL >= 3)
#define PIFS_NOTICE_MSG(...)    do { \
        printf("%s ", __FUNCTION__); \
        printf(__VA_ARGS__); \
        fflush(stdout); \
    } while (0);
#else
#define PIFS_NOTICE_MSG(...)
#endif

#if (PIFS_DEBUG_LEVEL >= 4)
#define PIFS_INFO_MSG(...)    do { \
        printf("%s ", __FUNCTION__); \
        printf(__VA_ARGS__); \
        fflush(stdout); \
    } while (0);
#else
#define PIFS_INFO_MSG(...)
#endif

#if (PIFS_DEBUG_LEVEL >= 5)
#define PIFS_DEBUG_MSG(...)    do { \
        printf("%s ", __FUNCTION__); \
        printf(__VA_ARGS__); \
        fflush(stdout); \
    } while (0);
#else
#define PIFS_DEBUG_MSG(...)
#endif

#endif /* _INCLUDE_PIFS_DEBUG_H_ */
