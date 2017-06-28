/**
 * @file        pifs_test.c
 * @brief       Test of Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-15 14:34:46 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "api_pifs.h"
#include "pifs.h"
#include "pifs_debug.h"
#include "buffer.h"

#define ENABLE_LARGE_TEST       0
#define ENABLE_FRAGMENT_TEST    1

#define PIFS_TEST_ERROR_MSG(...)    do { \
        fprintf(stderr, "%s ERROR: ", __FUNCTION__); \
        fprintf(stderr, __VA_ARGS__); \
    } while (0);

uint8_t test_buf_w[2 * 256] __attribute__((aligned(4)));
uint8_t test_buf_r[2 * 256] __attribute__((aligned(4)));

void generate_buffer(uint32_t sequence_start)
{
    fill_buffer(test_buf_w, sizeof(test_buf_w), FILL_TYPE_SEQUENCE_WORD, sequence_start);
}

void check_buffers()
{
    if (compare_buffer(test_buf_w, sizeof(test_buf_w), test_buf_r) == PIFS_SUCCESS)
    {
        PIFS_NOTICE_MSG("Buffers match!\r\n");
    }
}

pifs_status_t pifs_test(void)
{
    pifs_status_t ret = PIFS_ERROR_FLASH_INIT;
    P_FILE * file;
    size_t   written_size = 0;
    size_t   read_size = 0;
    size_t   i = 0;

    ret = pifs_init();
    PIFS_ASSERT(ret == PIFS_SUCCESS);

#if ENABLE_LARGE_TEST
    printf("-------------------------------------------------\r\n");

    file = pifs_fopen("test.dat", "w");
    if (file)
    {
        printf("File opened for writing\r\n");
        for (i = 0; i < PIFS_MAP_ENTRY_PER_PAGE + 2; i++)
        {
            generate_buffer(i);
//            print_buffer(test_buf_w, sizeof(test_buf_w), 0);
            written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
        }
    }
    pifs_fclose(file);
#endif

#if ENABLE_FRAGMENT_TEST
    printf("-------------------------------------------------\r\n");

    file = pifs_fopen("test2.dat", "w");
    if (file)
    {
        const size_t size_delta = 4;
        printf("File opened for writing\r\n");
        generate_buffer(2);
        written_size = 0;
        for (i = 0; i < sizeof(test_buf_w); i += size_delta)
        {
            written_size += pifs_fwrite(&test_buf_w[i], 1, size_delta, file);
        }
    }
    pifs_fclose(file);
#endif
#if 0
    printf("-------------------------------------------------\r\n");

    file = pifs_fopen("test3.dat", "w");
    if (file)
    {
        printf("File opened for writing\r\n");
        generate_buffer(3);
        written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
    }
    pifs_fclose(file);
#endif
#if ENABLE_LARGE_TEST
    printf("-------------------------------------------------\r\n");

    file = pifs_fopen("test.dat", "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        for (i = 0; i <= PIFS_MAP_ENTRY_PER_PAGE; i++)
        {
            generate_buffer(i);
            read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
//            print_buffer(test_buf_r, sizeof(test_buf_r), 0);
            check_buffers();
        }
    }
    pifs_fclose(file);
#endif
#if ENABLE_FRAGMENT_TEST
    printf("-------------------------------------------------\r\n");

    file = pifs_fopen("test2.dat", "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        generate_buffer(2);
        read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
        //print_buffer(test_buf_r, sizeof(test_buf_r), 0);
        check_buffers();
    }
    pifs_fclose(file);
#endif
    ret = pifs_delete();
    PIFS_ASSERT(ret == PIFS_SUCCESS);

    return ret;
}

int main(void)
{
    srand(time(0));
    pifs_test();

    return 0;
}

