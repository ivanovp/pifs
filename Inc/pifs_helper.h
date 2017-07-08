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
char * pifs_byte2bin_str(uint8_t byte);
void pifs_print_cache(void);
bool_t pifs_is_address_valid(pifs_address_t * a_address);

#endif /* _INCLUDE_PIFS_HELPER_H_ */
