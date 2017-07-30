/**
 * @file        pifs_wear.h
 * @brief       Internal header of Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-27 19:35:38 ivanovp {Time-stamp}
 * Licence:     GPL
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
pifs_status_t pifs_find_least_weared_block(pifs_header_t * a_header,
                                          pifs_block_address_t * a_block_address,
                                          pifs_block_type_t a_block_type,
                                          pifs_wear_level_cntr_t * a_wear_level_cntr);
pifs_status_t pifs_generate_least_weared_blocks(pifs_header_t * a_header);

#endif /* _INCLUDE_PIFS_WEAR_H_ */
