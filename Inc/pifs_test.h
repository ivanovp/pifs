/**
 * @file        pifs_test.h
 * @brief       Function prototypes for test of Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-27 19:35:38 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_PIFS_TEST_H_
#define _INCLUDE_PIFS_TEST_H_

#include <stdint.h>

#include "common.h"

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
pifs_status_t pifs_test(void);

#endif /* _INCLUDE_PIFS_TEST_H_ */
