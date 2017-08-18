/**
 * @file        pifs_file.h
 * @brief       Internal header of Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-27 19:35:38 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_PIFS_FILE_H_
#define _INCLUDE_PIFS_FILE_H_

#include <stdint.h>

#include "common.h"
#include "flash.h"
#include "pifs_config.h"
#include "pifs.h"

pifs_status_t pifs_internal_open(pifs_file_t * a_file,
                                 const pifs_char_t * a_filename,
                                 const pifs_char_t * a_modes, bool_t a_is_merge_allowed);
pifs_status_t pifs_inc_write_address(pifs_file_t * a_file);
pifs_status_t pifs_inc_read_address(pifs_file_t * a_file);

#endif /* _INCLUDE_PIFS_FILE_H_ */
