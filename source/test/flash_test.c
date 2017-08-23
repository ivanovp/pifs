/**
 * @file        flash_test.c
 * @brief       Flash test
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-07-17 11:34:37 ivanovp {Time-stamp}
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flash.h"
#include "api_pifs.h"
#include "pifs.h"
#include "pifs_debug.h"
#include "buffer.h"

#define FLASH_TEST_ERROR_MSG(...)    do { \
        printf("%s ERROR: ", __FUNCTION__); \
        printf(__VA_ARGS__); \
    } while (0);

uint8_t test_buf_w[PIFS_FLASH_PAGE_SIZE_BYTE];
uint8_t test_buf_r[PIFS_FLASH_PAGE_SIZE_BYTE];

pifs_status_t flash_erase_all(void)
{
    pifs_block_address_t i;
    pifs_status_t ret = PIFS_SUCCESS;

    printf("Erasing all blocks ");
    for (i = PIFS_FLASH_BLOCK_RESERVED_NUM; i < PIFS_FLASH_BLOCK_NUM_ALL; i++)
    {
        ret = pifs_flash_erase(i);
        putchar('.');
        /* TODO mark bad blocks */
    }
    printf("\r\nDone.\r\n");

    return ret;
}

pifs_status_t flash_test_erase_program(void)
{
    pifs_status_t ret;
    pifs_block_address_t ba;
    pifs_page_address_t pa;

    printf("Testing erase and program...\r\n");
    ba = PIFS_FLASH_BLOCK_RESERVED_NUM;
    ret = pifs_flash_erase(ba);
    PIFS_ASSERT(ret == PIFS_SUCCESS);

    printf("Block %i ", ba + 1);
    for (pa = 0; pa < PIFS_FLASH_PAGE_PER_BLOCK; pa++)
    {
        putchar('.');
        //printf("Page %i/%i\r\n", pa, PIFS_FLASH_PAGE_PER_BLOCK);
        /* Program bit 0 (clear bit 0) */
        fill_buffer(test_buf_w, sizeof(test_buf_w), FILL_TYPE_SIMPLE_BYTE, 0xFE);
        ret = pifs_flash_write(ba, pa, 0, test_buf_w, sizeof(test_buf_w));
        PIFS_ASSERT(ret == PIFS_SUCCESS);

        ret = pifs_flash_read(ba, pa, 0, test_buf_r, sizeof(test_buf_r));
        PIFS_ASSERT(ret == PIFS_SUCCESS);

        ret = compare_buffer(test_buf_w, sizeof(test_buf_w), test_buf_r);
        PIFS_ASSERT(ret == PIFS_SUCCESS);

        /* Program bit 3, 2, and 1 as well */
        fill_buffer(test_buf_w, sizeof(test_buf_w), FILL_TYPE_SIMPLE_BYTE, 0xF0);
        ret = pifs_flash_write(ba, pa, 0, test_buf_w, sizeof(test_buf_w));
        PIFS_ASSERT(ret == PIFS_SUCCESS);

        ret = pifs_flash_read(ba, pa, 0, test_buf_r, sizeof(test_buf_r));
        PIFS_ASSERT(ret == PIFS_SUCCESS);

        ret = compare_buffer(test_buf_w, sizeof(test_buf_w), test_buf_r);
        PIFS_ASSERT(ret == PIFS_SUCCESS);
#if 0
        /* Testing program error detection */
        /* Try to Un-program bit 0, which should not be possible */
        test_buf_w[PIFS_FLASH_PAGE_SIZE_BYTE - 1] = 0xF1;
        ret = pifs_flash_write(ba, pa, 0, test_buf_w, sizeof(test_buf_w));
        PIFS_ASSERT(ret == PIFS_ERROR_FLASH_WRITE);
#endif
    }
    printf("\r\nDone.\r\n");

    return ret;
}

pifs_status_t flash_test_random_write(void)
{
    pifs_status_t ret;
    pifs_block_address_t ba;
    pifs_block_address_t ba_max;
    pifs_page_address_t pa;

    ret = flash_erase_all();
    PIFS_ASSERT(ret == PIFS_SUCCESS);

    ba_max = PIFS_MIN(PIFS_FLASH_BLOCK_RESERVED_NUM + 2, PIFS_FLASH_BLOCK_NUM_ALL);

    printf("Testing random data write...\r\n");
    for (ba = PIFS_FLASH_BLOCK_RESERVED_NUM; ba < ba_max; ba++)
    {
        printf("Block %i/%i ", ba + 1, ba_max);
        for (pa = 0; pa < PIFS_FLASH_PAGE_PER_BLOCK; pa++)
        {
            //printf("Block %i/%i, page %i/%i\r\n", ba + 1, ba_max, pa, PIFS_FLASH_PAGE_PER_BLOCK);
            fill_buffer(test_buf_w, sizeof(test_buf_w), FILL_TYPE_RANDOM, 0);
            ret = pifs_flash_write(ba, pa, 0, test_buf_w, sizeof(test_buf_w));
            PIFS_ASSERT(ret == PIFS_SUCCESS);

            ret = pifs_flash_read(ba, pa, 0, test_buf_r, sizeof(test_buf_r));
            PIFS_ASSERT(ret == PIFS_SUCCESS);

            ret = compare_buffer(test_buf_w, sizeof(test_buf_w), test_buf_r);
            PIFS_ASSERT(ret == PIFS_SUCCESS);
            putchar('.');
        }
        putchar(ASCII_CR);
        putchar(ASCII_LF);
    }
    printf("Done.\r\n");

    return ret;
}    

pifs_status_t flash_test_pattern(void)
{
    pifs_status_t ret;
    pifs_block_address_t ba;
    pifs_block_address_t ba_max;
    pifs_page_address_t pa;
    uint32_t pattern;
    const uint32_t pattern_arr[8] =
    {
        0xFF00FF00,
        0x00FF00FF,
        0x0FF00FF0,
        0xAA00AA00,
        0x55005500,
        0xAA55AA55,
        0x5AA55AA5,
        0xA55AA55A
    };

    ret = flash_erase_all();
    PIFS_ASSERT(ret == PIFS_SUCCESS);

    ba_max = PIFS_MIN((sizeof(pattern_arr) / sizeof(pattern_arr[0])), PIFS_FLASH_BLOCK_NUM_ALL);

    printf("Testing pattern write...\r\n");
    for (ba = PIFS_FLASH_BLOCK_RESERVED_NUM; ba < ba_max; ba++)
    {
        pattern = pattern_arr[ba & ((sizeof(pattern_arr) / sizeof(pattern_arr[0])) - 1)];
        printf("Pattern: 0x%08X\r\n", pattern);
        printf("Block %i/%i ", ba + 1, ba_max);
        for (pa = 0; pa < PIFS_FLASH_PAGE_PER_BLOCK; pa++)
        {
            //printf("Block %i/%i, page %i/%i\r\n", ba, PIFS_FLASH_BLOCK_NUM_ALL, pa, PIFS_FLASH_PAGE_PER_BLOCK);
            fill_buffer(test_buf_w, sizeof(test_buf_w), FILL_TYPE_SIMPLE_DWORD, pattern);
            ret = pifs_flash_write(ba, pa, 0, test_buf_w, sizeof(test_buf_w));
            PIFS_ASSERT(ret == PIFS_SUCCESS);

            ret = pifs_flash_read(ba, pa, 0, test_buf_r, sizeof(test_buf_r));
            PIFS_ASSERT(ret == PIFS_SUCCESS);

            ret = compare_buffer(test_buf_w, sizeof(test_buf_w), test_buf_r);
            PIFS_ASSERT(ret == PIFS_SUCCESS);
            putchar('.');
        }
        putchar(ASCII_CR);
        putchar(ASCII_LF);
    }
    printf("Done.\r\n");

    return ret;
}    
    
pifs_status_t flash_test_addressable(void)
{
    pifs_status_t ret;
    pifs_block_address_t ba;
    pifs_page_address_t pa;

    ret = flash_erase_all();
    PIFS_ASSERT(ret == PIFS_SUCCESS);

    printf("Testing whole memory area is addressable...\r\n");
    printf("Writing...\r\n");
    for (ba = PIFS_FLASH_BLOCK_RESERVED_NUM; ba < PIFS_FLASH_BLOCK_NUM_ALL; ba++)
    {
        printf("Block %i/%i ", ba + 1, PIFS_FLASH_BLOCK_NUM_ALL);
        for (pa = 0; pa < PIFS_FLASH_PAGE_PER_BLOCK; pa++)
        {
            //printf("Block %i/%i, page %i/%i\r\n", ba, PIFS_FLASH_BLOCK_NUM_ALL, pa, PIFS_FLASH_PAGE_PER_BLOCK);
            fill_buffer(test_buf_w, sizeof(test_buf_w), FILL_TYPE_SEQUENCE_DWORD, ((uint32_t)ba << 16) | pa);
            ret = pifs_flash_write(ba, pa, 0, test_buf_w, sizeof(test_buf_w));
            PIFS_ASSERT(ret == PIFS_SUCCESS);
            putchar('.');
        }
        putchar(ASCII_CR);
        putchar(ASCII_LF);
    }
    
    printf("Reading...\r\n");
    for (ba = PIFS_FLASH_BLOCK_RESERVED_NUM; ba < PIFS_FLASH_BLOCK_NUM_ALL; ba++)
    {
        printf("Block %i/%i ", ba + 1, PIFS_FLASH_BLOCK_NUM_ALL);
        for (pa = 0; pa < PIFS_FLASH_PAGE_PER_BLOCK; pa++)
        {
            //printf("Block %i/%i, page %i/%i\r\n", ba, PIFS_FLASH_BLOCK_NUM_ALL, pa, PIFS_FLASH_PAGE_PER_BLOCK);
            fill_buffer(test_buf_w, sizeof(test_buf_w), FILL_TYPE_SEQUENCE_DWORD, ((uint32_t)ba << 16) | pa);
            
            ret = pifs_flash_read(ba, pa, 0, test_buf_r, sizeof(test_buf_r));
            PIFS_ASSERT(ret == PIFS_SUCCESS);

            ret = compare_buffer(test_buf_w, sizeof(test_buf_w), test_buf_r);
            PIFS_ASSERT(ret == PIFS_SUCCESS);
            putchar('.');
        }
        putchar(ASCII_CR);
        putchar(ASCII_LF);
    }
    printf("Done.\r\n");

    return ret;
}

pifs_status_t flash_test(void)
{
    pifs_status_t ret = PIFS_ERROR_FLASH_INIT;

    printf("Size of flash memory (full):        %i bytes\r\n", PIFS_FLASH_SIZE_BYTE_ALL);
    printf("Size of flash memory (used by FS):  %i bytes\r\n", PIFS_FLASH_SIZE_BYTE_ALL);
    printf("Number of blocks (full):            %i\r\n", PIFS_FLASH_BLOCK_NUM_ALL);
    printf("Number of blocks (used by FS)):     %i\r\n", PIFS_FLASH_BLOCK_NUM_ALL - PIFS_FLASH_BLOCK_RESERVED_NUM);
    printf("Number of pages/block:              %i\r\n", PIFS_FLASH_PAGE_PER_BLOCK);
    printf("Size of page:                       %i byte\r\n", PIFS_FLASH_PAGE_SIZE_BYTE);

    ret = flash_test_erase_program();
    if (ret == PIFS_SUCCESS)
    {
        ret = flash_test_random_write();
    }
    if (ret == PIFS_SUCCESS)
    {
        ret = flash_test_addressable();
    }
    if (ret == PIFS_SUCCESS)
    {
        ret = flash_test_pattern();
    }

    return ret;
}
