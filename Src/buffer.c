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

#define PRINT_BUF_COLUMNS               (16)
/* 1: print first mismatch, then print buffer content */
/* 0: print all mismatches, then print buffer content */
#define COMPARE_PRINT_FIRST_MISMATCH    1

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
            for (i = 0; i < buf_size16; i++)
            {
                buf16[i] = (uint16_t) a_fill_arg;
            }
            break;
        case FILL_TYPE_SIMPLE_DWORD:
            for (i = 0; i < buf_size32; i++)
            {
                buf32[i] = a_fill_arg;
            }
            break;
        case FILL_TYPE_SEQUENCE_BYTE:
            for (i = 0; i < buf_size8; i++)
            {
                buf8[i] = (uint8_t) (a_fill_arg + i);
            }
            break;
        case FILL_TYPE_SEQUENCE_WORD:
            for (i = 0; i < buf_size16; i++)
            {
                buf16[i] = (uint16_t) (a_fill_arg + i);
            }
            break;
        case FILL_TYPE_SEQUENCE_DWORD:
            for (i = 0; i < buf_size32; i++)
            {
                buf32[i] = a_fill_arg + i;
            }
            break;
        case FILL_TYPE_RANDOM:
            for (i = 0; i < buf_size32; i++)
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
    size_t          i;
    uint8_t       * buf1 = a_buf1;
    uint8_t       * buf2 = a_buf2;
    pifs_status_t   ret = PIFS_SUCCESS;

    for (i = 0; i < a_buf_size; i++)
    {
        if (buf1[i] != buf2[i])
        {
            BUFFER_ERROR_MSG("Mismatch at index %lu. Expected: 0x%02X, Read: 0x%02X\r\n",
                    i, buf1[i], buf2[i]);
            ret = PIFS_ERROR_GENERAL;
#if COMPARE_PRINT_FIRST_MISMATCH
            break;
#endif
        }
    }

    if (ret == PIFS_ERROR_GENERAL)
    {
        BUFFER_ERROR_MSG("a_buf1:\r\n");
        print_buffer(a_buf1, a_buf_size, 0);
        BUFFER_ERROR_MSG("a_buf2:\r\n");
        print_buffer(a_buf2, a_buf_size, 0);
        exit(-1);
    }

    return ret;
}

void print_hex_byte(uint8_t byte)
{
    printf("%02X", byte);
}

/*
 * Print buffer in hexadecimal and ASCII format to the UART interface.
 *
 * @param[in] buf       Buffer to print.
 * @param[in] buf_size  Length of buffer.
 * @param[in] address   Address of buffer
 */
void print_buffer (const void * a_buf, size_t a_buf_size, uint32_t a_addr)
{
    uint8_t * buf = (uint8_t*) a_buf;
    size_t  i, j, s;

    /* Extend buffer size to be dividable with PRINT_BUF_COLUMNS */
    if (a_buf_size % PRINT_BUF_COLUMNS == 0)
    {
        s = a_buf_size;
    }
    else
    {
        s = a_buf_size + PRINT_BUF_COLUMNS - a_buf_size % PRINT_BUF_COLUMNS;
    }

    print_hex_byte(a_addr >> 24);
    print_hex_byte(a_addr >> 16);
    print_hex_byte(a_addr >> 8);
    print_hex_byte(a_addr);
    putchar(' ');
    for (i = 0 ; i < s ; i++, a_addr++)
    {
        /* Print buffer in hexadecimal format */
        if (i < a_buf_size)
        {
            print_hex_byte(buf[i]);
        }
        else
        {
            putchar(' ');
            putchar(' ');
        }

        putchar(' ');
        if ((i + 1) % PRINT_BUF_COLUMNS == 0)
        {
            /* Print buffer in ASCII format */
            putchar(' ');
            putchar(' ');
            for (j = i - (PRINT_BUF_COLUMNS - 1) ; j <= i ; j++)
            {
                if (j < a_buf_size)
                {
                    uint8_t  data = buf[j];
                    if ((data >= 0x20) && (data <= 0x7F))
                    {
                        putchar(data);
                    }
                    else
                    {
                        putchar('.');
                    }
                }
                else
                {
                    putchar(' ');
                }
            }

            putchar(ASCII_CR);
            putchar(ASCII_LF);
            if (j < a_buf_size)
            {
                print_hex_byte((a_addr + 1) >> 24);
                print_hex_byte((a_addr + 1) >> 16);
                print_hex_byte((a_addr + 1) >> 8);
                print_hex_byte((a_addr + 1));
                putchar(' ');
            }
        }
    }

    putchar(ASCII_CR);
    putchar(ASCII_LF);
} /* print_buf */

