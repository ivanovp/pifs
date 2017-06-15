/**
 * @file        flash_test.c
 * @brief       Flash test
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-15 14:59:50 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flash.h"
#include "api_pifs.h"
#include "pifs_debug.h"
#include "buffer.h"

#define FLASH_TEST_ERROR_MSG(...)    do { \
        fprintf(stderr, "%s ERROR: ", __FUNCTION__); \
        fprintf(stderr, __VA_ARGS__); \
    } while (0);

uint8_t test_buf_w[PIFS_FLASH_PAGE_SIZE_BYTE];
uint8_t test_buf_r[PIFS_FLASH_PAGE_SIZE_BYTE];

pifs_status_t flash_erase_all(void)
{
    block_address_t i;
    pifs_status_t ret = PIFS_SUCCESS;

    for (i = PIFS_FLASH_BLOCK_START; i < PIFS_FLASH_BLOCK_NUM; i++)
    {
        ret = flash_erase(i);
        /* TODO mark bad blocks */
    }

    return ret;
}

pifs_status_t flash_test_erase_program(void)
{
    pifs_status_t ret;
    block_address_t ba;
    page_address_t pa;

    printf("Testing erase and program... ");
    ba = PIFS_FLASH_BLOCK_START;
    ret = flash_erase(ba);
    PIFS_ASSERT(ret == PIFS_SUCCESS);

    for (pa = 0; pa < PIFS_FLASH_PAGE_PER_BLOCK; pa++)
    {
        /* Program bit 0 (clear bit 0) */
        fill_buffer(test_buf_w, sizeof(test_buf_w), FILL_TYPE_SIMPLE_BYTE, 0xFE);
        ret = flash_write(ba, pa, 0, test_buf_w, sizeof(test_buf_w));
        PIFS_ASSERT(ret == PIFS_SUCCESS);

        ret = flash_read(ba, pa, 0, test_buf_r, sizeof(test_buf_r));
        PIFS_ASSERT(ret == PIFS_SUCCESS);

        ret = compare_buffer(test_buf_w, sizeof(test_buf_w), test_buf_r);
        PIFS_ASSERT(ret == PIFS_SUCCESS);

        /* Program bit 3, 2, and 1 as well */
        fill_buffer(test_buf_w, sizeof(test_buf_w), FILL_TYPE_SIMPLE_BYTE, 0xF0);
        ret = flash_write(ba, pa, 0, test_buf_w, sizeof(test_buf_w));
        PIFS_ASSERT(ret == PIFS_SUCCESS);

        ret = flash_read(ba, pa, 0, test_buf_r, sizeof(test_buf_r));
        PIFS_ASSERT(ret == PIFS_SUCCESS);

        ret = compare_buffer(test_buf_w, sizeof(test_buf_w), test_buf_r);
        PIFS_ASSERT(ret == PIFS_SUCCESS);

        /* Testing program error detection */
        /* Try to Un-program bit 0, which should not be possible */
        test_buf_w[PIFS_FLASH_PAGE_SIZE_BYTE - 1] = 0xF1;
        ret = flash_write(ba, pa, 0, test_buf_w, sizeof(test_buf_w));
        PIFS_ASSERT(ret == PIFS_ERROR);
    }
    printf("Done.\r\n");

    return ret;
}

pifs_status_t flash_test_random_write(void)
{
    pifs_status_t ret;
    block_address_t ba;
    page_address_t pa;

    printf("Erasing all blocks... ");
    ret = flash_erase_all();
    PIFS_ASSERT(ret == PIFS_SUCCESS);
    printf("Done.\r\n");
    
    printf("Testing random data write... ");
    for (ba = PIFS_FLASH_BLOCK_START; ba < PIFS_FLASH_BLOCK_NUM; ba++)
    {
        for (pa = 0; pa < PIFS_FLASH_PAGE_PER_BLOCK; pa++)
        {
            fill_buffer(test_buf_w, sizeof(test_buf_w), FILL_TYPE_RANDOM, 0);
            ret = flash_write(ba, pa, 0, test_buf_w, sizeof(test_buf_w));
            PIFS_ASSERT(ret == PIFS_SUCCESS);

            ret = flash_read(ba, pa, 0, test_buf_r, sizeof(test_buf_r));
            PIFS_ASSERT(ret == PIFS_SUCCESS);

            ret = compare_buffer(test_buf_w, sizeof(test_buf_w), test_buf_r);
            PIFS_ASSERT(ret == PIFS_SUCCESS);
        }
    }
    printf("Done.\r\n");

    return ret;
}    

pifs_status_t flash_test_pattern(void)
{
    pifs_status_t ret;
    block_address_t ba;
    page_address_t pa;
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

    printf("Erasing all blocks... ");
    ret = flash_erase_all();
    PIFS_ASSERT(ret == PIFS_SUCCESS);
    printf("Done.\r\n");
    
    printf("Testing pattern write... ");
    for (ba = PIFS_FLASH_BLOCK_START; ba < PIFS_FLASH_BLOCK_NUM; ba++)
    {
        pattern = pattern_arr[ba & ((sizeof(pattern_arr) / sizeof(pattern_arr[0])) - 1)];
        printf("Pattern: 0x%08X\r\n", pattern);
        for (pa = 0; pa < PIFS_FLASH_PAGE_PER_BLOCK; pa++)
        {
            fill_buffer(test_buf_w, sizeof(test_buf_w), FILL_TYPE_SIMPLE_DWORD, pattern);
            ret = flash_write(ba, pa, 0, test_buf_w, sizeof(test_buf_w));
            PIFS_ASSERT(ret == PIFS_SUCCESS);

            ret = flash_read(ba, pa, 0, test_buf_r, sizeof(test_buf_r));
            PIFS_ASSERT(ret == PIFS_SUCCESS);

            ret = compare_buffer(test_buf_w, sizeof(test_buf_w), test_buf_r);
            PIFS_ASSERT(ret == PIFS_SUCCESS);
        }
    }
    printf("Done.\r\n");

    return ret;
}    
    
pifs_status_t flash_test_addressable(void)
{
    pifs_status_t ret;
    block_address_t ba;
    page_address_t pa;

    printf("Erasing all blocks... ");
    ret = flash_erase_all();
    PIFS_ASSERT(ret == PIFS_SUCCESS);
    printf("Done.\r\n");

    printf("Testing whole memory area is addressable... ");
    for (ba = PIFS_FLASH_BLOCK_START; ba < PIFS_FLASH_BLOCK_NUM; ba++)
    {
        for (pa = 0; pa < PIFS_FLASH_PAGE_PER_BLOCK; pa++)
        {
            fill_buffer(test_buf_w, sizeof(test_buf_w), FILL_TYPE_SEQUENCE_DWORD, ((uint32_t)ba << 16) | pa);
            ret = flash_write(ba, pa, 0, test_buf_w, sizeof(test_buf_w));
            PIFS_ASSERT(ret == PIFS_SUCCESS);
        }
    }
    
    for (ba = PIFS_FLASH_BLOCK_START; ba < PIFS_FLASH_BLOCK_NUM; ba++)
    {
        for (pa = 0; pa < PIFS_FLASH_PAGE_PER_BLOCK; pa++)
        {
            fill_buffer(test_buf_w, sizeof(test_buf_w), FILL_TYPE_SEQUENCE_DWORD, ((uint32_t)ba << 16) | pa);
            
            ret = flash_read(ba, pa, 0, test_buf_r, sizeof(test_buf_r));
            PIFS_ASSERT(ret == PIFS_SUCCESS);

            ret = compare_buffer(test_buf_w, sizeof(test_buf_w), test_buf_r);
            PIFS_ASSERT(ret == PIFS_SUCCESS);
        }
    }
    printf("Done.\r\n");

    return ret;
}

pifs_status_t flash_test(void)
{
    pifs_status_t ret = PIFS_FLASH_INIT_ERROR;

    printf("Size of flash memory:   %i bytes\r\n", PIFS_FLASH_SIZE_BYTE);
    printf("Number of blocks:       %i\r\n", PIFS_FLASH_BLOCK_NUM);
    printf("Number of pages/block:  %i\r\n", PIFS_FLASH_PAGE_PER_BLOCK);
    printf("Size of page:           %i byte\r\n", PIFS_FLASH_PAGE_SIZE_BYTE);

    ret = flash_init();
    PIFS_ASSERT(ret == PIFS_SUCCESS);

    ret = flash_test_erase_program();
    ret = flash_test_random_write();
    ret = flash_test_addressable();
    ret = flash_test_pattern();

    ret = flash_delete();
    PIFS_ASSERT(ret == PIFS_SUCCESS);

    return ret;
}

int main(void)
{
    flash_test();

    return 0;
}

