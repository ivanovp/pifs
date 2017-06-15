/**
 * @file        buffer.c
 * @brief       Buffer filling and comparing for tests
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-15 15:01:16 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api_pifs.h"
#include "buffer.h"
#include "pifs_debug.h"

void fill_buffer(void * a_buf, size_t a_buf_size, fill_type_t a_fill_type, uint32_t a_fill_arg)
{
    size_t      i;
    uint8_t   * buf8 = a_buf;
    size_t      buf_size8 = a_buf_size;
    uint16_t  * buf16 = a_buf;
    size_t      buf_size16 = a_buf_size >> 1;
    uint32_t  * buf32 = a_buf;
    size_t      buf_size32 = a_buf_size >> 2;

    switch (a_fill_type)
    {
        case FILL_TYPE_SIMPLE_BYTE:
            memset(a_buf, (uint8_t) a_fill_arg, a_buf_size);
            break;
        case FILL_TYPE_SIMPLE_WORD:
            for ( i = 0; i < buf_size16; i++ )
            {
                buf16[i] = (uint16_t) a_fill_arg;
            }
            break;
        case FILL_TYPE_SIMPLE_DWORD:
            for ( i = 0; i < buf_size32; i++ )
            {
                buf32[i] = a_fill_arg;
            }
            break;
        case FILL_TYPE_SEQUENCE_BYTE:
            for ( i = 0; i < buf_size8; i++ )
            {
                buf8[i] = (uint8_t) (a_fill_arg + i);
            }
            break;
        case FILL_TYPE_SEQUENCE_WORD:
            for ( i = 0; i < buf_size16; i++ )
            {
                buf16[i] = (uint16_t) (a_fill_arg + i);
            }
            break;
        case FILL_TYPE_SEQUENCE_DWORD:
            for ( i = 0; i < buf_size32; i++ )
            {
                buf32[i] = a_fill_arg + i;
            }
            break;
        case FILL_TYPE_RANDOM:
            for ( i = 0; i < buf_size32; i++ )
            {
                buf32[i] = rand();
            }
            break;
        default:
            PIFS_ASSERT(0);
            break;
    }
}

pifs_status_t compare_buffer(void * a_buf1, size_t a_buf_size, void * a_buf2)
{
    size_t    i;
    uint8_t * buf1 = a_buf1;
    uint8_t * buf2 = a_buf2;
    pifs_status_t ret = PIFS_SUCCESS;

    for (i = 0; i < a_buf_size; i++)
    {
        if (buf1[i] != buf2[i])
        {
            BUFFER_ERROR_MSG("Mismatch at index %lu. Expected: 0x%02X, Read: 0x%02X\r\n",
                    i, buf1[i], buf2[i]);
            ret = PIFS_ERROR;
        }
    }

    return ret;
}

