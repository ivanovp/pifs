/**
 * @file        pifs_test.h
 * @brief       Function prototypes for test of Pi file system
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
#ifndef _INCLUDE_PIFS_TEST_H_
#define _INCLUDE_PIFS_TEST_H_

#include <stdint.h>

#include "common.h"
#include "pifs_config.h"

pifs_status_t pifs_test_small_w(void);
pifs_status_t pifs_test_small_r(void);
pifs_status_t pifs_test_full_w(const char * a_filename);
pifs_status_t pifs_test_full_r(const char * a_filename);
pifs_status_t pifs_test_basic_w(const char * a_filename);
pifs_status_t pifs_test_basic_r(const char * a_filename);
pifs_status_t pifs_test_large_w(void);
pifs_status_t pifs_test_large_r(void);
pifs_status_t pifs_test_wfragment_w(size_t fragment_size);
pifs_status_t pifs_test_wfragment_r(void);
pifs_status_t pifs_test_rfragment_w(void);
pifs_status_t pifs_test_rfragment_r(size_t a_fragment_size);
pifs_status_t pifs_test_rseek_w(void);
pifs_status_t pifs_test_rseek_r(void);
pifs_status_t pifs_test_wseek_w(void);
pifs_status_t pifs_test_wseek_r(void);
pifs_status_t pifs_test_delta_w(const char * a_filename);
pifs_status_t pifs_test_delta_r(const char * a_filename);
pifs_status_t pifs_test_list_dir(void);
#if PIFS_ENABLE_DIRECTORIES
pifs_status_t pifs_test_dir_w(void);
pifs_status_t pifs_test_dir_r(void);
#endif
pifs_status_t pifs_test(void);

#endif /* _INCLUDE_PIFS_TEST_H_ */
