/**
 * @file        pifs_wear.h
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
#ifndef _INCLUDE_PIFS_WEAR_H_
#define _INCLUDE_PIFS_WEAR_H_

#include <stdint.h>

#include "common.h"
#include "flash.h"
#include "pifs_config.h"
#include "pifs.h"

pifs_status_t pifs_wear_level_list_init(void);
pifs_status_t pifs_get_wear_level(pifs_block_address_t a_block_address,
                                  pifs_header_t * a_header,
                                  pifs_wear_level_entry_t * a_wear_level);
pifs_status_t pifs_inc_wear_level(pifs_block_address_t a_block_address,
                                  pifs_header_t * a_header);
pifs_status_t pifs_write_wear_level(pifs_block_address_t a_block_address,
                                    pifs_header_t * a_header,
                                    pifs_wear_level_entry_t * a_wear_level);
pifs_status_t pifs_copy_wear_level_list(pifs_header_t * a_old_header, pifs_header_t * a_new_header);
pifs_status_t pifs_get_block_wear_stats(pifs_block_type_t a_block_type,
                                        pifs_header_t * a_header,
                                        pifs_block_address_t * a_block_address,
                                        pifs_wear_level_cntr_t * a_wear_level_cntr,
                                        pifs_wear_level_cntr_t * a_wear_level_cntr_max);
pifs_status_t pifs_generate_least_weared_blocks(pifs_header_t * a_header);
pifs_status_t pifs_check_block(pifs_char_t * a_filename,
                               pifs_block_address_t a_block_address,
                               bool_t * a_is_block_used);
pifs_status_t pifs_empty_block(pifs_block_address_t a_block_address,
                               bool_t * a_is_emptied);
pifs_status_t pifs_static_wear_leveling(pifs_size_t a_max_block_num);

#endif /* _INCLUDE_PIFS_WEAR_H_ */
