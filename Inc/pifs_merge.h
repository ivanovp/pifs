/**
 * @file        pifs_merge.h
 * @brief       Internal header of Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-27 19:35:38 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_PIFS_MERGE_H_
#define _INCLUDE_PIFS_MERGE_H_

#include <stdint.h>

#include "common.h"
#include "flash.h"
#include "pifs_config.h"
#include "pifs.h"

pifs_status_t pifs_merge(void);
pifs_status_t pifs_merge_check(pifs_file_t * a_file, pifs_size_t a_data_page_count_minimum);

#endif /* _INCLUDE_PIFS_MERGE_H_ */
