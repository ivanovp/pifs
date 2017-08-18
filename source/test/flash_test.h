/**
 * @file        flash_test.h
 * @brief       Function prototypes for flash test
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-27 19:35:38 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_FLASH_TEST_H_
#define _INCLUDE_FLASH_TEST_H_

#include <stdint.h>

#include "common.h"
#include "flash.h"

pifs_status_t flash_erase_all(void);
pifs_status_t flash_test(void);

#endif /* _INCLUDE_FLASH_TEST_H_ */
