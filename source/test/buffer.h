/**
 * @file        flash.h
 * @brief       Header of flash routines
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-15 14:59:27 ivanovp {Time-stamp}
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
#ifndef _INCLUDE_BUFFER_H_
#define _INCLUDE_BUFFER_H_

#include <stdint.h>
#include "api_pifs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BUFFER_ERROR_MSG(...)    do { \
        printf("%s ERROR: ", __FUNCTION__); \
        printf(__VA_ARGS__); \
    } while (0);

typedef enum
{
    FILL_TYPE_SIMPLE_BYTE,
    FILL_TYPE_SIMPLE_WORD,
    FILL_TYPE_SIMPLE_DWORD,
    FILL_TYPE_SEQUENCE_BYTE,
    FILL_TYPE_SEQUENCE_WORD,
    FILL_TYPE_SEQUENCE_DWORD,
    FILL_TYPE_RANDOM
} fill_type_t;

void fill_buffer(void * a_buf, size_t a_buf_size, fill_type_t a_fill_type, uint32_t a_fill_arg);
pifs_status_t compare_buffer(void * a_buf1, size_t a_buf_size, void * a_buf2);
void print_buffer (const void *a_buf, size_t a_buf_size, uint32_t a_addr );

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDE_BUFFER_H_ */
