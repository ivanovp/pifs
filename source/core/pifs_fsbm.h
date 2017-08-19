/**
 * @file        pifs_fsbm.h
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
#ifndef _INCLUDE_PIFS_FSBM_H_
#define _INCLUDE_PIFS_FSBM_H_

#include <stdint.h>

#include "common.h"
#include "flash.h"
#include "pifs_config.h"
#include "pifs.h"

typedef struct
{
    pifs_page_count_t    page_count_minimum;  /**< Number of pages needed at least. */
    pifs_page_count_t    page_count_desired;  /**< Number of pages needed. */
    pifs_block_type_t    block_type;          /**< Block type to find. */
    bool_t               is_free;             /**< TRUE: find free page,
                                                   FALSE: find to be released page. */
    bool_t               is_same_block;       /**< TRUE: find pages in same block,
                                                   FALSE: pages can be in different block. */
    pifs_block_address_t start_block_address; /**< Start address of search. */
    pifs_block_address_t end_block_address;   /**< End address of search. */
    pifs_header_t      * header;
} pifs_find_t;

pifs_status_t pifs_calc_free_space_pos(const pifs_address_t * a_free_space_bitmap_address,
                                       pifs_block_address_t a_block_address,
                                       pifs_page_address_t a_page_address,
                                       pifs_block_address_t * a_free_space_block_address,
                                       pifs_page_address_t * a_free_space_page_address,
                                       pifs_bit_pos_t * a_free_space_bit_pos);
void pifs_calc_address(pifs_bit_pos_t a_bit_pos,
                       pifs_block_address_t * a_block_address,
                       pifs_page_address_t * a_page_address);
bool_t pifs_is_page_free(pifs_block_address_t a_block_address,
                         pifs_page_address_t a_page_address);
bool_t pifs_is_page_to_be_released(pifs_block_address_t a_block_address,
                                   pifs_page_address_t a_page_address);
pifs_status_t pifs_mark_page(pifs_block_address_t a_block_address,
                             pifs_page_address_t a_page_address,
                             pifs_page_count_t a_page_count, bool_t a_mark_used);
pifs_status_t pifs_find_free_page_wl(pifs_page_count_t a_page_count_minimum,
                                     pifs_page_count_t a_page_count_desired,
                                     pifs_block_type_t a_block_type,
                                     pifs_block_address_t * a_block_address,
                                     pifs_page_address_t * a_page_address,
                                     pifs_page_count_t * a_page_count_found);
pifs_status_t pifs_find_page(pifs_page_count_t a_page_count_minimum,
                             pifs_page_count_t a_page_count_desired,
                             pifs_block_type_t a_block_type,
                             bool_t a_is_free,
                             bool_t a_is_same_block,
                             pifs_block_address_t a_start_block_address,
                             pifs_block_address_t * a_block_address,
                             pifs_page_address_t * a_page_address,
                             pifs_page_count_t * a_page_count_found);
pifs_status_t pifs_find_page_adv(pifs_find_t * a_find,
                                 pifs_block_address_t * a_block_address,
                                 pifs_page_address_t * a_page_address,
                                 pifs_page_count_t * a_page_count_found);
pifs_status_t pifs_find_block_wl(pifs_size_t a_block_count,
                                   pifs_block_type_t a_block_type, bool_t a_is_free,
                                   pifs_header_t *a_header,
                                   pifs_block_address_t * a_block_address);
pifs_status_t pifs_find_to_be_released_block(pifs_size_t a_block_count,
                                             pifs_block_type_t a_block_type,
                                             pifs_block_address_t a_start_block_address,
                                             pifs_header_t * a_header,
                                             pifs_block_address_t * a_block_address);
pifs_status_t pifs_get_pages(bool_t a_is_free,
                             pifs_block_address_t a_start_block_address,
                             pifs_size_t a_block_count,
                             pifs_size_t * a_management_page_count,
                             pifs_size_t * a_data_page_count);
pifs_status_t pifs_get_to_be_released_pages(pifs_size_t * a_free_management_page_count,
                                            pifs_size_t * a_free_data_page_count);
pifs_status_t pifs_get_free_pages(pifs_size_t * a_free_management_page_count,
                                  pifs_size_t * a_free_data_page_count);

#endif /* _INCLUDE_PIFS_FSBM_H_ */
