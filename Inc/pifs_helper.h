/**
 * @file        pifs_helper.h
 * @brief       Internal header of Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-27 19:35:38 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_PIFS_HELPER_H_
#define _INCLUDE_PIFS_HELPER_H_

#include <stdint.h>

#include "common.h"
#include "flash.h"
#include "pifs_config.h"
#include "pifs.h"

char * pifs_address2str(pifs_address_t * a_address);
char * pifs_ba_pa2str(pifs_block_address_t a_block_address, pifs_page_address_t a_page_address);
char * pifs_flash_address2str(pifs_address_t * a_address);
char * pifs_flash_ba_pa2str(pifs_block_address_t a_block_address, pifs_page_address_t a_page_address);
char * pifs_byte2bin_str(uint8_t byte);
void pifs_print_cache(void);
pifs_status_t pifs_print_map_page(pifs_block_address_t a_block_address,
                                  pifs_page_address_t a_page_address,
                                  pifs_size_t a_count);
char * yesNo(bool_t expression);
void pifs_print_page_info (size_t addr, pifs_size_t cntr);
bool_t pifs_is_address_valid(pifs_address_t * a_address);
bool_t pifs_is_block_type(pifs_block_address_t a_block_address,
                          pifs_block_type_t a_block_type,
                          pifs_header_t *a_header);
bool_t pifs_is_buffer_erased(const void * a_buf, pifs_size_t a_buf_size);
bool_t pifs_is_page_erased(pifs_block_address_t a_block_address,
                           pifs_page_address_t a_page_address);
bool_t pifs_is_buffer_programmable(const void * a_orig_buf, const void * a_new_buf, pifs_size_t a_buf_size);
bool_t pifs_is_buffer_programmed(const void * a_buf, pifs_size_t a_buf_size);
void pifs_parse_open_mode(pifs_file_t * a_file, const pifs_char_t *a_modes);
pifs_status_t pifs_inc_address(pifs_address_t * a_address);
pifs_status_t pifs_add_address(pifs_address_t * a_address, pifs_size_t a_page_count);
pifs_status_t pifs_inc_ba_pa(pifs_block_address_t * a_block_address,
                             pifs_page_address_t * a_page_address);
pifs_status_t pifs_add_ba_pa(pifs_block_address_t * a_block_address,
                             pifs_page_address_t * a_page_address,
                             pifs_size_t a_page_count);
pifs_status_t pifs_check_filename(const pifs_char_t *a_filename);
pifs_status_t pifs_get_file(pifs_file_t **a_file);

#endif /* _INCLUDE_PIFS_HELPER_H_ */
