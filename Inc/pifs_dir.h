/**
 * @file        pifs_dir.h
 * @brief       Internal header of Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-27 19:35:38 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_PIFS_DIR_H_
#define _INCLUDE_PIFS_DIR_H_

#include <stdint.h>

#include "common.h"
#include "flash.h"
#include "pifs_config.h"
#include "pifs.h"

#define PIFS_IS_ONE_DOT_DIR(name) (name[0] == PIFS_DOT_CHAR && name[1] == PIFS_EOS)
#define PIFS_IS_TWO_DOT_DIR(name) (name[0] == PIFS_DOT_CHAR && name[1] == PIFS_DOT_CHAR && name[2] == PIFS_EOS)
#define PIFS_IS_DOT_DIR(name) (PIFS_IS_ONE_DOT_DIR(name) || PIFS_IS_TWO_DOT_DIR(name))

typedef pifs_status_t (*pifs_dir_walker_func_t)(pifs_dirent_t * a_dirent, void * a_fund_data);
pifs_status_t pifs_walk_dir(const pifs_char_t * const a_path, bool_t a_recursive, bool_t a_stop_at_error,
                            pifs_dir_walker_func_t a_dir_walker_func, void * a_func_data);

#endif /* _INCLUDE_PIFS_DIR_H_ */
