/**
 * @file        pifs_file.h
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
#ifndef _INCLUDE_PIFS_FILE_H_
#define _INCLUDE_PIFS_FILE_H_

#include <stdint.h>

#include "common.h"
#include "flash.h"
#include "pifs_config.h"
#include "pifs.h"

#ifdef __cplusplus
extern "C" {
#endif

pifs_status_t pifs_internal_open(pifs_file_t * a_file,
                                 const pifs_char_t * a_filename,
                                 const pifs_char_t * a_modes, bool_t a_is_merge_allowed);
pifs_status_t pifs_inc_write_address(pifs_file_t * a_file);
pifs_status_t pifs_inc_read_address(pifs_file_t * a_file);
int pifs_internal_fflush(P_FILE * a_file);
int pifs_internal_fclose(P_FILE * a_file);
int pifs_internal_fseek(P_FILE * a_file, long int a_offset, int a_origin);
bool_t pifs_internal_is_file_exist(const pifs_char_t * a_filename);
void pifs_internal_rewind(P_FILE * a_file);
int pifs_internal_fsetuserdata(P_FILE * a_file, const pifs_user_data_t * a_user_data);
int pifs_internal_remove(const pifs_char_t * a_filename);

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDE_PIFS_FILE_H_ */
