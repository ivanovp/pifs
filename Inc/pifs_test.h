/**
 * @file        pifs_test.h
 * @brief       Function prototypes for test of Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-27 19:35:38 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_PIFS_TEST_H_
#define _INCLUDE_PIFS_TEST_H_

#include <stdint.h>

#include "common.h"

void pifs_info(void);
pifs_status_t pifs_test(void);

#endif /* _INCLUDE_PIFS_TEST_H_ */
