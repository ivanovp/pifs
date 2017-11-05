/**
 * @file        pifs_dir.h
 * @brief       Internal header of Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-27 19:35:38 ivanovp {Time-stamp}
 * Licence:     GPL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _INCLUDE_PIFS_DIR_H_
#define _INCLUDE_PIFS_DIR_H_

#include <stdint.h>

#include "common.h"
#include "flash.h"
#include "pifs_config.h"
#include "pifs.h"

#if PIFS_ENABLE_DIRECTORIES
#define PIFS_IS_ONE_DOT_DIR(name) (name[0] == PIFS_DOT_CHAR && name[1] == PIFS_EOS)
#define PIFS_IS_TWO_DOT_DIR(name) (name[0] == PIFS_DOT_CHAR && name[1] == PIFS_DOT_CHAR && name[2] == PIFS_EOS)
#define PIFS_IS_DOT_DIR(name) (PIFS_IS_ONE_DOT_DIR(name) || PIFS_IS_TWO_DOT_DIR(name))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef pifs_status_t (*pifs_dir_walker_func_t)(pifs_dirent_t * a_dirent, void * a_fund_data);
pifs_status_t pifs_resolve_path(const pifs_char_t * a_path,
                                pifs_address_t a_current_entry_list_address,
                                pifs_char_t * const a_filename,
                                pifs_address_t * const a_resolved_entry_list_address);
pifs_address_t * pifs_get_task_current_entry_list_address(void);
pifs_dir_t * pifs_internal_opendir(const pifs_char_t * a_name);
pifs_dirent_t *pifs_internal_readdir(pifs_dir_t * a_dirp);
int pifs_internal_closedir(pifs_dir_t * const a_dirp);
pifs_status_t pifs_walk_dir(const pifs_char_t * const a_path, bool_t a_recursive, bool_t a_stop_at_error,
                            pifs_dir_walker_func_t a_dir_walker_func, void * a_func_data);
pifs_status_t pifs_internal_mkdir(const pifs_char_t * const a_filename);
pifs_status_t pifs_internal_chdir(const pifs_char_t * const a_filename);

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDE_PIFS_DIR_H_ */
