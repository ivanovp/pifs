/**
 * @file        common.h
 * @brief       Common definitions
 * @author      Copyright (C) Peter Ivanov, 2011, 2012
 *
 * Created      2011-01-19 11:48:53
 * Last modify: 2017-06-15 12:19:50 ivanovp {Time-stamp}
 * Licence:     GPL
 */

#ifndef _INCLUDE_COMMON_H_
#define _INCLUDE_COMMON_H_

#ifndef bool_t
typedef unsigned char bool_t;
#endif

#ifndef FALSE
#define FALSE       (0)
#endif
#ifndef TRUE
#define TRUE        (1)
#endif

#define TOSTR(s)    XSTR(s)
#define XSTR(s)     #s

#define CR          (13)
#define LF          (10)

#endif /* _INCLUDE_COMMON_H_ */
