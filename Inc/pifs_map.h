/**
 * @file        pifs_map.h
 * @brief       Internal header of Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-08-04 14:26:52 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_PIFS_MAP_H_
#define _INCLUDE_PIFS_MAP_H_

#include <stdint.h>

#include "common.h"
#include "flash.h"
#include "pifs_config.h"
#include "pifs.h"

typedef pifs_status_t (*pifs_file_walker_func_t)(pifs_file_t * a_file, 
                                                 pifs_block_address_t a_block_address,
                                                 pifs_page_address_t a_page_address,
                                                 pifs_block_address_t a_delta_block_address,
                                                 pifs_page_address_t a_delta_page_address);

pifs_status_t pifs_read_first_map_entry(pifs_file_t * a_file);
pifs_status_t pifs_read_next_map_entry(pifs_file_t * a_file);
pifs_status_t pifs_is_free_map_entry(pifs_file_t * a_file,
                                     bool_t * a_is_free_map_entry);
pifs_status_t pifs_append_map_entry(pifs_file_t * a_file,
                                    pifs_block_address_t a_block_address,
                                    pifs_page_address_t a_page_address,
                                    pifs_page_count_t a_page_count);
pifs_status_t pifs_walk_file_pages(pifs_file_t * a_file, 
                                   pifs_file_walker_func_t a_file_walker_func);
pifs_status_t pifs_release_file_pages(pifs_file_t * a_file);

#endif /* _INCLUDE_PIFS_MAP_H_ */
