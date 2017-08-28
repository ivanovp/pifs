/**
 * @file        pifs_map.h
 * @brief       Internal header of Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-08-04 14:26:52 ivanovp {Time-stamp}
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
#ifndef _INCLUDE_PIFS_MAP_H_
#define _INCLUDE_PIFS_MAP_H_

#include <stdint.h>

#include "common.h"
#include "flash.h"
#include "pifs_config.h"
#include "pifs.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief pifs_file_walker_func_t
 * Callback function which is called for file's every data and map pages.
 * If delta block and page are equal to original block address, no delta
 * page is used.
 * If a_map_page is TRUE, delta address is not applicabe (invalid address
 * given).
 *
 * @param[in] a_file                 Pointer to file.
 * @param[in] a_block_address        Original block address.
 * @param[in] a_page_address         Original page address.
 * @param[in] a_delta_block_address  Delta block address.
 * @param[in] a_delta_page_address   Delta page address.
 * @param[in] a_map_page             TRUE: a_block_address/a_page_address points
 *                                   to a map page.
 *                                   FALSE: a_block_address/a_page_address points
 *                                   to a data page.
 * @param[in] a_func_data            User data.
 *
 * @return PIFS_SUCCESS if page processed successfully.
 */
typedef pifs_status_t (*pifs_file_walker_func_t)(pifs_file_t * a_file,
                                                 pifs_block_address_t a_block_address,
                                                 pifs_page_address_t a_page_address,
                                                 pifs_block_address_t a_delta_block_address,
                                                 pifs_page_address_t a_delta_page_address,
                                                 bool_t a_map_page,
                                                 void * a_func_data);

pifs_status_t pifs_read_first_map_entry(pifs_file_t * a_file);
pifs_status_t pifs_read_next_map_entry(pifs_file_t * a_file);
pifs_status_t pifs_is_free_map_entry(pifs_file_t * a_file,
                                     bool_t * a_is_free_map_entry);
pifs_status_t pifs_append_map_entry(pifs_file_t * a_file,
                                    pifs_block_address_t a_block_address,
                                    pifs_page_address_t a_page_address,
                                    pifs_page_count_t a_page_count);
pifs_status_t pifs_walk_file_pages(pifs_file_t * a_file,
                                   pifs_file_walker_func_t a_file_walker_func,
                                   void * a_func_data);
pifs_status_t pifs_release_file_pages(pifs_file_t * a_file);

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDE_PIFS_MAP_H_ */
