/**
 * @file        pifs_entry.h
 * @brief       Internal header of Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-27 19:35:38 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_PIFS_ENTRY_H_
#define _INCLUDE_PIFS_ENTRY_H_

#include <stdint.h>

#include "common.h"
#include "flash.h"
#include "pifs_config.h"
#include "pifs.h"

pifs_status_t pifs_append_entry(pifs_entry_t * a_entry);
pifs_status_t pifs_update_entry(const pifs_char_t * a_name, pifs_entry_t * const a_entry);
pifs_status_t pifs_find_entry(const pifs_char_t * a_name, pifs_entry_t * const a_entry);
pifs_status_t pifs_clear_entry(const pifs_char_t * a_name);
pifs_status_t pifs_count_entries(pifs_size_t * a_free_entry_count, pifs_size_t * a_to_be_released_entry_count);

#endif /* _INCLUDE_PIFS_ENTRY_H_ */
