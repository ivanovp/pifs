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
#include "buffer.h"

#define PIFS_DEBUG_LEVEL    5
#include "pifs_debug.h"

#define ENABLE_SMALL_FILES_TEST       1
#define ENABLE_FULL_WRITE_TEST        0
#define ENABLE_BASIC_TEST             1
#define ENABLE_LARGE_TEST             1
#define ENABLE_WRITE_FRAGMENT_TEST    1
#define ENABLE_READ_FRAGMENT_TEST     1
#define ENABLE_SEEK_READ_TEST         1
#define ENABLE_SEEK_WRITE_TEST        0
#if ENABLE_SMALL_FILES_TEST
#define ENABLE_LIST_DIRECTORY_TEST    1
#endif

#define TEST_FULL_PAGE_NUM            (PIFS_FLASH_PAGE_NUM_FS / 2)
#define TEST_BUF_SIZE                 (PIFS_FLASH_PAGE_SIZE_BYTE * 2)
#define SEEK_TEST_POS                 100

#if TEST_BUF_SIZE < SEEK_TEST_POS
#error SEEK_TEST_POS shall be less than TEST_BUF_SIZE!
#endif
#if PIFS_FILENAME_LEN_MAX < 12
#error PIFS_FILENAME_LEN_MAX shall be at least 12!
#endif

#define PIFS_TEST_ERROR_MSG(...)    do { \
        printf("%s ERROR: ", __FUNCTION__); \
        printf(__VA_ARGS__); \
        exit(-1); \
    } while (0);

uint8_t test_buf_w[TEST_BUF_SIZE] __attribute__((aligned(4)));
uint8_t test_buf_r[TEST_BUF_SIZE] __attribute__((aligned(4)));

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

void print_fs_info(void)
{
    size_t               free_management_bytes;
    size_t               free_data_bytes;
    size_t               free_management_pages;
    size_t               free_data_pages;
    if (pifs_get_free_space(&free_management_bytes, &free_data_bytes,
                            &free_management_pages, &free_data_pages) != PIFS_SUCCESS)
    {
        PIFS_ERROR_MSG("Cannot get free space!\r\n");
    }
    PIFS_INFO_MSG("Free data area:                     %lu bytes, %lu pages\r\n",
                  free_data_bytes, free_data_pages);
    PIFS_INFO_MSG("Free management area:               %lu bytes, %lu pages\r\n",
                  free_management_bytes, free_management_pages);
}

pifs_status_t pifs_test(void)
{
    pifs_status_t ret = PIFS_ERROR_FLASH_INIT;
    P_FILE * file;
    size_t   written_size = 0;
    size_t   read_size = 0;
    size_t   i = 0;
    size_t   testfull = 0;
    size_t   written_pages = 0;
    char     filename[32];
    pifs_DIR * dir;
    struct pifs_dirent * dirent;

    ret = pifs_init();
    PIFS_ASSERT(ret == PIFS_SUCCESS);

#if ENABLE_SMALL_FILES_TEST
    printf("-------------------------------------------------\r\n");
    printf("Small file test: writing files\r\n");

    for (i = 0; i < PIFS_ENTRY_NUM_MAX / 2; i++)
    {
        snprintf(filename, sizeof(filename), "tsm%lu.dat", i);
        file = pifs_fopen(filename, "w");
        if (file)
        {
            printf("File opened for writing\r\n");
            generate_buffer(i);
            //print_buffer(test_buf_w, sizeof(test_buf_w), 0);
            written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
        }
        else
        {
            PIFS_ERROR_MSG("Cannot open file!\r\n");
        }
        pifs_fclose(file);
    }
#endif


#if ENABLE_FULL_WRITE_TEST
    printf("-------------------------------------------------\r\n");
    printf("Full write test: writing file\r\n");

    file = pifs_fopen("testfull.dat", "w");
    if (file)
    {
        printf("File opened for writing\r\n");
        written_pages = 0;
        for (i = 0; i < TEST_FULL_PAGE_NUM; i++)
        {
            generate_buffer(i + 55);
//            print_buffer(test_buf_w, sizeof(test_buf_w), 0);
            written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
            if (written_size == sizeof(test_buf_w))
            {
                written_pages++;
            }
            else
            {
                PIFS_DEBUG_MSG("Media full!!!\r\n");
                testfull = i;
                break;
            }
        }
    }
    else
    {
        PIFS_ERROR_MSG("Cannot open file!\r\n");
    }
    pifs_fclose(file);

    printf("-------------------------------------------------\r\n");
    printf("Full write test: reading file\r\n");

    file = pifs_fopen("testfull.dat", "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        for (i = 0; i < written_pages; i++)
        {
            printf("i: %i\r\n", i);
            generate_buffer(i + 55);
            read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
            if (read_size != sizeof(test_buf_r))
            {
                printf("Read size differs!\r\n");
            }
//            print_buffer(test_buf_r, sizeof(test_buf_r), 0);
            check_buffers();
        }
    }
    else
    {
        PIFS_ERROR_MSG("Cannot open file!\r\n");
    }
    pifs_fclose(file);
    if (pifs_remove("testfull.dat") == 0)
    {
        printf("File removed!\r\n");
    }
    else
    {
        printf("ERROR: Cannot remove file!\r\n");
    }
#endif

#if ENABLE_BASIC_TEST
    printf("-------------------------------------------------\r\n");
    printf("Basic test: writing file\r\n");

    file = pifs_fopen("testb.dat", "w");
    if (file)
    {
        printf("File opened for writing\r\n");
        generate_buffer(42);
        //print_buffer(test_buf_w, sizeof(test_buf_w), 0);
        written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
    }
    else
    {
        PIFS_ERROR_MSG("Cannot open file!\r\n");
    }
    pifs_fclose(file);
#endif


#if ENABLE_LARGE_TEST
    printf("-------------------------------------------------\r\n");
    printf("Large test: writing file\r\n");

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
    else
    {
        PIFS_ERROR_MSG("Cannot open file!\r\n");
    }
    pifs_fclose(file);
#endif

#if ENABLE_WRITE_FRAGMENT_TEST
    printf("-------------------------------------------------\r\n");
    printf("Fragment write test: writing file\r\n");

    file = pifs_fopen("test2.dat", "w");
    if (file)
    {
        const size_t size_delta = 5;
        printf("File opened for writing\r\n");
        generate_buffer(2);
        written_size = 0;
        for (i = 0; i < sizeof(test_buf_w); i += size_delta)
        {
            written_size += pifs_fwrite(&test_buf_w[i], 1, size_delta, file);
        }
    }
    else
    {
        PIFS_ERROR_MSG("Cannot open file!\r\n");
    }
    pifs_fclose(file);
#endif
#if ENABLE_READ_FRAGMENT_TEST
    printf("-------------------------------------------------\r\n");
    printf("Fragment read test: writing file\r\n");

    file = pifs_fopen("test3.dat", "w");
    if (file)
    {
        printf("File opened for writing\r\n");
        generate_buffer(3);
        written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
    }
    else
    {
        PIFS_ERROR_MSG("Cannot open file!\r\n");
    }
    pifs_fclose(file);
#endif

#if ENABLE_SEEK_READ_TEST
    printf("-------------------------------------------------\r\n");
    printf("Seek read test: writing file\r\n");

    file = pifs_fopen("tstseek1.dat", "w");
    if (file)
    {
        printf("File opened for writing\r\n");
        generate_buffer(7);
        written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
        written_size += pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
        if (written_size != 2 * sizeof(test_buf_w))
        {
            printf("Cannot write\r\n");
        }
    }
    else
    {
        PIFS_ERROR_MSG("Cannot open file!\r\n");
    }
    pifs_fclose(file);
#endif
#if ENABLE_SEEK_WRITE_TEST
    printf("-------------------------------------------------\r\n");
    printf("Seek write test: writing file\r\n");
    file = pifs_fopen("tstseek2.dat", "w");
    if (file)
    {
        printf("File opened for writing\r\n");
        generate_buffer(8);
        pifs_fseek(file, SEEK_TEST_POS, PIFS_SEEK_SET);
        written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
    }
    else
    {
        PIFS_ERROR_MSG("Cannot open file!\r\n");
    }
    pifs_fclose(file);
#endif

    /**************************************************************************/
    /**************************************************************************/
    /**************************************************************************/

#if ENABLE_SMALL_FILES_TEST
    printf("-------------------------------------------------\r\n");
    printf("Small files test: reading files\r\n");

    for (i = 0; i < PIFS_ENTRY_NUM_MAX / 2; i++)
    {
        snprintf(filename, sizeof(filename), "tsm%lu.dat", i);
        file = pifs_fopen(filename, "r");
        if (file)
        {
            printf("File opened for reading\r\n");
            generate_buffer(i);
            read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
            //print_buffer(test_buf_r, sizeof(test_buf_r), 0);
            check_buffers();
        }
        else
        {
            PIFS_ERROR_MSG("Cannot open file!\r\n");
        }
        pifs_fclose(file);
    }
#endif

#if ENABLE_LARGE_TEST
    printf("-------------------------------------------------\r\n");
    printf("Large test: reading file\r\n");

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
    else
    {
        PIFS_ERROR_MSG("Cannot open file!\r\n");
    }
    pifs_fclose(file);
#endif
#if ENABLE_BASIC_TEST
    printf("-------------------------------------------------\r\n");
    printf("Basic test: reading file\r\n");

    file = pifs_fopen("testb.dat", "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        generate_buffer(42);
        read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
        //print_buffer(test_buf_r, sizeof(test_buf_r), 0);
        check_buffers();
    }
    else
    {
        PIFS_ERROR_MSG("Cannot open file!\r\n");
    }
    pifs_fclose(file);
#endif
#if ENABLE_WRITE_FRAGMENT_TEST
    printf("-------------------------------------------------\r\n");
    printf("Write fragment test: reading file\r\n");

    memset(test_buf_r, 0, sizeof(test_buf_r));
    file = pifs_fopen("test2.dat", "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        generate_buffer(2);
        read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
        //print_buffer(test_buf_r, sizeof(test_buf_r), 0);
        check_buffers();
    }
    else
    {
        PIFS_ERROR_MSG("Cannot open file!\r\n");
    }
    pifs_fclose(file);
#endif
#if ENABLE_READ_FRAGMENT_TEST
    printf("-------------------------------------------------\r\n");
    printf("Read fragment test: reading file\r\n");

    memset(test_buf_r, 0, sizeof(test_buf_r));
    file = pifs_fopen("test3.dat", "r");
    if (file)
    {
        const size_t size_delta = 5;
        printf("File opened for reading\r\n");
        generate_buffer(3);
        read_size = 0;
        for (i = 0; i < sizeof(test_buf_r); i += size_delta)
        {
            read_size += pifs_fread(&test_buf_r[i], 1, size_delta, file);
        }
        check_buffers();
    }
    else
    {
        PIFS_ERROR_MSG("Cannot open file!\r\n");
    }
    pifs_fclose(file);
#endif
#if ENABLE_SEEK_READ_TEST
    printf("-------------------------------------------------\r\n");
    printf("Seek read test: reading file\r\n");

    file = pifs_fopen("tstseek1.dat", "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        generate_buffer(7);
        if (pifs_fseek(file, SEEK_TEST_POS, PIFS_SEEK_SET))
        {
            printf("Cannot seek!\r\n");
        }
        read_size = pifs_fread(&test_buf_r[SEEK_TEST_POS], 1, sizeof(test_buf_r) - SEEK_TEST_POS, file);
        if (pifs_fseek(file, -sizeof(test_buf_r), PIFS_SEEK_CUR))
        {
            printf("Cannot seek!\r\n");
        }
        read_size = pifs_fread(test_buf_r, 1, SEEK_TEST_POS, file);
        check_buffers();
        if (pifs_fseek(file, -sizeof(test_buf_r), PIFS_SEEK_END))
        {
            printf("Cannot seek!\r\n");
        }
        read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
        check_buffers();
    }
    else
    {
        PIFS_ERROR_MSG("Cannot open file!\r\n");
    }
    pifs_fclose(file);
#endif
#if ENABLE_SEEK_WRITE_TEST
    printf("-------------------------------------------------\r\n");
    printf("Seek write test: reading file\r\n");

    file = pifs_fopen("tstseek2.dat", "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        generate_buffer(8);
        /* First 100 byte shall be zero, due to fseek */
        fill_buffer(test_buf_w, sizeof(test_buf_w), FILL_TYPE_SEQUENCE_BYTE, 0);
        read_size = pifs_fread(test_buf_r, 1, 100, file);
        check_buffers();

//        pifs_fseek(file, 100, PIFS_SEEK_SET);
        read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
        check_buffers();
    }
    else
    {
        PIFS_ERROR_MSG("Cannot open file!\r\n");
    }
    pifs_fclose(file);
#endif
#if ENABLE_LIST_DIRECTORY_TEST
    printf("-------------------------------------------------\r\n");
    printf("List directory test\r\n");
    dir = pifs_opendir("/");
    if (dir != NULL)
    {
        while (dirent = pifs_readdir(dir))
        {
            printf("%s\t%i\r\n", dirent->d_name, pifs_filesize(dirent->d_name));
        }
        if (pifs_closedir (dir) != 0)
        {
            printf("Cannot close directory!\r\n");
        }
    }
    else
    {
        printf("Could not open the directory!\r\n");
    }
#endif

    printf("END OF TESTS\r\n");
    print_fs_info();
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

