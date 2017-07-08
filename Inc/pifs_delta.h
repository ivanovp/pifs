/**
 * @file        pifs_delta.h
 * @brief       Internal header of Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-07-08 13:52:22 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_PIFS_DELTA_H_
#define _INCLUDE_PIFS_DELTA_H_

#include <stdint.h>

#include "common.h"
#include "flash.h"
#include "pifs_config.h"
#include "pifs.h"

pifs_status_t pifs_find_delta_page(pifs_block_address_t a_block_address,
                                   pifs_page_address_t a_page_address,
                                   pifs_block_address_t * a_delta_block_address,
                                   pifs_page_address_t * a_delta_page_address,
                                   bool_t * a_is_map_full);
pifs_status_t pifs_read_delta(pifs_block_address_t a_block_address,
                              pifs_page_address_t a_page_address,
                              pifs_page_offset_t a_page_offset,
                              void * const a_buf,
                              pifs_size_t a_buf_size);
pifs_status_t pifs_write_delta(pifs_block_address_t a_block_address,
                               pifs_page_address_t a_page_address,
                               pifs_page_offset_t a_page_offset,
                               const void * const a_buf,
                               pifs_size_t a_buf_size,
                               bool_t * a_is_delta);

#endif /* _INCLUDE_PIFS_DELTA_H_ */
