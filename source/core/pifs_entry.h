/**
 * @file        pifs_entry.h
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
#ifndef _INCLUDE_PIFS_ENTRY_H_
#define _INCLUDE_PIFS_ENTRY_H_

#include <stdint.h>

#include "common.h"
#include "flash.h"
#include "pifs_config.h"
#include "pifs.h"

typedef enum
{
    PIFS_FIND_ENTRY,
    PIFS_DELETE_ENTRY,  /**< Set deleted attribute or zero first character of name */
    PIFS_CLEAR_ENTRY    /**< Zero all bytes of entry */
} pifs_entry_cmd_t;

pifs_status_t pifs_read_entry(pifs_block_address_t a_entry_list_block_address,
                              pifs_page_address_t a_entry_list_page_address,
                              pifs_size_t a_entry_idx,
                              pifs_entry_t * const a_entry,
                              bool_t * const a_is_erased);
pifs_status_t pifs_write_entry(pifs_block_address_t a_entry_list_block_address,
                              pifs_page_address_t a_entry_list_page_address,
                              pifs_size_t a_entry_idx,
                              const pifs_entry_t * const a_entry);
pifs_status_t pifs_append_entry(const pifs_entry_t * const a_entry,
                                pifs_block_address_t a_entry_list_block_address,
                                pifs_page_address_t a_entry_list_page_address);
pifs_status_t pifs_update_entry(const pifs_char_t * a_name, pifs_entry_t * const a_entry,
                                pifs_block_address_t a_entry_list_block_address,
                                pifs_page_address_t a_entry_list_page_address);
void pifs_mark_entry_deleted(pifs_entry_t * a_entry);
bool_t pifs_is_entry_deleted(pifs_entry_t * a_entry);
pifs_status_t pifs_find_entry(pifs_entry_cmd_t entry_cmd,
                              const pifs_char_t * a_name, pifs_entry_t * const a_entry,
                              pifs_block_address_t a_entry_list_block_address,
                              pifs_page_address_t a_entry_list_page_address);
pifs_status_t pifs_delete_entry(const pifs_char_t * a_name,
                                pifs_block_address_t a_entry_list_block_address,
                                pifs_page_address_t a_entry_list_page_address);
pifs_status_t pifs_clear_entry(const pifs_char_t * a_name,
                               pifs_block_address_t a_entry_list_block_address,
                               pifs_page_address_t a_entry_list_page_address);
pifs_status_t pifs_count_entries(pifs_size_t * a_free_entry_count, pifs_size_t * a_to_be_released_entry_count,
                              pifs_block_address_t a_entry_list_block_address,
                              pifs_page_address_t a_entry_list_page_address);

#endif /* _INCLUDE_PIFS_ENTRY_H_ */
