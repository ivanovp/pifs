/**
 * @file        pifs_test.c
 * @brief       Test of Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-08-01 17:34:06 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "api_pifs.h"
#include "pifs.h"
#include "pifs_entry.h"
#include "pifs_test.h"
#include "pifs_helper.h"
#include "buffer.h"

#define PIFS_DEBUG_LEVEL    5
#include "pifs_debug.h"

#define ENABLE_SMALL_FILES_TEST       1
#define ENABLE_FULL_WRITE_TEST        1
#define ENABLE_BASIC_TEST             1
#define ENABLE_LARGE_TEST             0
#define ENABLE_WRITE_FRAGMENT_TEST    1
#define ENABLE_READ_FRAGMENT_TEST     1
#define ENABLE_SEEK_READ_TEST         1
#define ENABLE_SEEK_WRITE_TEST        0
#define ENABLE_DELTA_TEST             1
#if ENABLE_BASIC_TEST
#define ENABLE_RENAME_TEST            1
#endif
#if ENABLE_SMALL_FILES_TEST
#define ENABLE_LIST_DIRECTORY_TEST    1
#endif

#define TEST_FULL_PAGE_NUM            (PIFS_LOGICAL_PAGE_NUM_FS / 2)
#define TEST_BUF_SIZE                 (PIFS_LOGICAL_PAGE_SIZE_BYTE * 2)
#define SEEK_TEST_POS                 100

#if TEST_BUF_SIZE < SEEK_TEST_POS
#error SEEK_TEST_POS shall be less than TEST_BUF_SIZE!
#endif
#if PIFS_FILENAME_LEN_MAX < 12
#error PIFS_FILENAME_LEN_MAX shall be at least 12!
#endif

#define LARGE_FILE_SIZE  (2 * PIFS_MAP_ENTRY_PER_PAGE + 2)

#define PIFS_TEST_ERROR_MSG(...)    do {    \
        printf("%s:%i ERROR: ", __FUNCTION__, __LINE__); \
        printf(__VA_ARGS__);                \
    } while (0);

uint8_t test_buf_w[TEST_BUF_SIZE] __attribute__((aligned(4)));
uint8_t test_buf_r[TEST_BUF_SIZE] __attribute__((aligned(4)));
const size_t fragment_size = 5;

void generate_buffer(uint32_t a_sequence_start, const char * a_filename)
{
    fill_buffer(test_buf_w, sizeof(test_buf_w), FILL_TYPE_SEQUENCE_WORD, a_sequence_start);
    snprintf((char*)test_buf_w, sizeof(test_buf_w),
             "File: %s, sequence: %i#", a_filename, a_sequence_start);
}

pifs_status_t check_buffers()
{
    pifs_status_t  ret;

    ret = compare_buffer(test_buf_w, sizeof(test_buf_w), test_buf_r);
    if (ret == PIFS_SUCCESS)
    {
        PIFS_NOTICE_MSG("Buffers match!\r\n");
    }

    return ret;
}

pifs_status_t pifs_test_small_w(void)
{
    pifs_status_t ret = PIFS_SUCCESS;
    P_FILE * file;
    size_t   i = 0;
    char     filename[32];
    size_t   written_size = 0;

    printf("-------------------------------------------------\r\n");
    printf("Small file test: writing files\r\n");

    for (i = 0; i < PIFS_ENTRY_NUM_MAX / 2 && ret == PIFS_SUCCESS; i++)
    {
        snprintf(filename, sizeof(filename), "small%lu.tst", i);
        file = pifs_fopen(filename, "w");
        if (file)
        {
            printf("File opened for writing %s\r\n", filename);
            generate_buffer(i, filename);
            //print_buffer(test_buf_w, sizeof(test_buf_w), 0);
            written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
            if (written_size != sizeof(test_buf_w))
            {
                PIFS_TEST_ERROR_MSG("Cannot write file: %i!\r\n", pifs_errno);
                ret = PIFS_ERROR_GENERAL;
            }
            if (pifs_fclose(file))
            {
                PIFS_TEST_ERROR_MSG("Cannot close file!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
        }
        else
        {
            PIFS_TEST_ERROR_MSG("Cannot open file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }

    return ret;
}

pifs_status_t pifs_test_small_r(void)
{
    pifs_status_t ret = PIFS_SUCCESS;
    P_FILE * file;
    char     filename[32];
    size_t   read_size = 0;
    size_t   i;

    printf("-------------------------------------------------\r\n");
    printf("Small files test: reading files\r\n");

    for (i = 0; i < PIFS_ENTRY_NUM_MAX / 2 && ret == PIFS_SUCCESS; i++)
    {
        snprintf(filename, sizeof(filename), "small%lu.tst", i);
        file = pifs_fopen(filename, "r");
        if (file)
        {
            printf("File opened for reading %s\r\n", filename);
            generate_buffer(i, filename);
            read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
            if (read_size != sizeof(test_buf_r))
            {
                PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
            //print_buffer(test_buf_r, sizeof(test_buf_r), 0);
            if (ret == PIFS_SUCCESS)
            {
                ret = check_buffers();
            }
            if (pifs_fclose(file))
            {
                PIFS_TEST_ERROR_MSG("Cannot close file!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
        }
        else
        {
            PIFS_TEST_ERROR_MSG("Cannot open file %s!\r\n", filename);
            ret = PIFS_ERROR_GENERAL;
        }
    }

    return ret;
}

pifs_status_t pifs_test_full_w(const char * a_filename)
{
    pifs_status_t ret = PIFS_SUCCESS;
    P_FILE      * file;
    pifs_file_t * f;
    size_t        written_size = 0;
    size_t        i;
    const char  * filename = "fullwrite.tst";
    if (a_filename != NULL)
    {
        filename = a_filename;
    }

    printf("-------------------------------------------------\r\n");
    printf("Full write test: writing file\r\n");

    file = pifs_fopen(filename, "w");
    if (file)
    {
        printf("File opened for writing\r\n");
        for (i = 0; i < TEST_FULL_PAGE_NUM; i++)
        {
            printf("full_w: %i\r\n", i);
            generate_buffer(i + 55, filename);
//            print_buffer(test_buf_w, sizeof(test_buf_w), 0);
            written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
            if (written_size != sizeof(test_buf_w))
            {
                PIFS_DEBUG_MSG("Media full!!!\r\n");
                break;
            }
        }
        PIFS_DEBUG_MSG("%i buffers written.\r\n", i);
        f = (pifs_file_t*) file;
        (void)pifs_print_map_page(f->entry.first_map_address.block_address,
                                  f->entry.first_map_address.page_address,
                                  UINT32_MAX);
        if (pifs_fclose(file))
        {
            PIFS_TEST_ERROR_MSG("Cannot close file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }
    else
    {
        PIFS_TEST_ERROR_MSG("Cannot open file!\r\n");
        ret = PIFS_ERROR_GENERAL;
    }

    return ret;
}

pifs_status_t pifs_test_full_r(const char * a_filename)
{
    pifs_status_t ret = PIFS_SUCCESS;
    P_FILE      * file;
    pifs_file_t * f;
    size_t        testfull_written_buffers = 0;
    size_t        file_size;
    size_t        read_size = 0;
    size_t        i;
    const char  * filename = "fullwrite.tst";
    if (a_filename != NULL)
    {
        filename = a_filename;
    }

    printf("-------------------------------------------------\r\n");
    printf("Full write test: reading file\r\n");

    file = pifs_fopen(filename, "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        file_size = pifs_filesize(filename);
        testfull_written_buffers = file_size / sizeof(test_buf_r);
        printf("File size: %i bytes, buffers: %i\r\n",
               file_size, testfull_written_buffers);
        for (i = 0; i < testfull_written_buffers && ret == PIFS_SUCCESS; i++)
        {
            printf("full_r: %i/%i\r\n", i + 1, testfull_written_buffers);
            generate_buffer(i + 55, filename);
            read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
            if (read_size != sizeof(test_buf_r))
            {
                PIFS_TEST_ERROR_MSG("Read size differs!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
            //print_buffer(test_buf_r, sizeof(test_buf_r), 0);
            if (ret == PIFS_SUCCESS)
            {
                ret = check_buffers();
            }
        }
        f = (pifs_file_t*) file;
        (void)pifs_print_map_page(f->entry.first_map_address.block_address,
                                  f->entry.first_map_address.page_address,
                                  UINT32_MAX);
        if (pifs_fclose(file))
        {
            PIFS_TEST_ERROR_MSG("Cannot close file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }
    else
    {
        PIFS_TEST_ERROR_MSG("Cannot open file!\r\n");
        ret = PIFS_ERROR_GENERAL;
    }

    return ret;
}

pifs_status_t pifs_test_basic_w(const char * a_filename)
{
    pifs_status_t ret = PIFS_SUCCESS;
    const char  * filename = "basic.tst";
#if ENABLE_RENAME_TEST
    const char  * filename2 = "basic2.tst";
#endif
    P_FILE      * file;
    size_t        written_size = 0;
#if PIFS_ENABLE_USER_DATA
    pifs_user_data_t user_data;
#endif

    if (a_filename)
    {
        filename = a_filename;
    }

    printf("-------------------------------------------------\r\n");
    printf("Basic test: writing file\r\n");

    file = pifs_fopen(filename, "w");
    if (file)
    {
        printf("File opened for writing\r\n");
#if PIFS_ENABLE_USER_DATA
        fill_buffer(&user_data, sizeof(user_data), FILL_TYPE_SEQUENCE_BYTE, 42);
        ret = pifs_fsetuserdata(file, &user_data);
        if (ret == PIFS_SUCCESS)
        {
            printf("User data set\r\n");
        }
        else
        {
            PIFS_TEST_ERROR_MSG("Cannot set user data!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
#endif
        generate_buffer(42, filename);
        //print_buffer(test_buf_w, sizeof(test_buf_w), 0);
        written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
        if (written_size != sizeof(test_buf_w))
        {
            PIFS_TEST_ERROR_MSG("Cannot write file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
        if (pifs_fclose(file))
        {
            PIFS_TEST_ERROR_MSG("Cannot close file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
#if ENABLE_RENAME_TEST
        if (ret == PIFS_SUCCESS && !a_filename)
        {
            printf("Renaming '%s' to '%s'...\r\n", filename, filename2);
            ret = pifs_rename(filename, filename2);
        }
#endif
    }
    else
    {
        PIFS_TEST_ERROR_MSG("Cannot open file!\r\n");
    }

    return ret;
}

pifs_status_t pifs_test_basic_r(const char * a_filename)
{
    pifs_status_t ret = PIFS_SUCCESS;
    const char  * filename = "basic.tst";
#if ENABLE_RENAME_TEST
    const char  * filename2 = "basic2.tst";
#endif
    P_FILE      * file;
    size_t        read_size = 0;
#if PIFS_ENABLE_USER_DATA
    pifs_user_data_t user_data_w;
    pifs_user_data_t user_data_r;
#endif

    if (a_filename)
    {
        filename = a_filename;
        generate_buffer(42, filename);
    }
    else
    {
        generate_buffer(42, filename);
#if ENABLE_RENAME_TEST
        filename = filename2;
#endif
    }

    printf("-------------------------------------------------\r\n");
    printf("Basic test: reading file\r\n");

    file = pifs_fopen(filename, "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
        if (read_size != sizeof(test_buf_r))
        {
            PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
        //print_buffer(test_buf_r, sizeof(test_buf_r), 0);
        if (ret == PIFS_SUCCESS)
        {
            ret = check_buffers();
        }
        if (pifs_feof(file))
        {
            printf("EOF indicator OK\r\n");
        }
        else
        {
            PIFS_TEST_ERROR_MSG("EOF indicator is wrong!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
#if PIFS_ENABLE_USER_DATA
        fill_buffer(&user_data_w, sizeof(user_data_w), FILL_TYPE_SEQUENCE_BYTE, 42);
        ret = pifs_fgetuserdata(file, &user_data_r);
        if (compare_buffer(&user_data_w, sizeof(user_data_w), &user_data_r) == PIFS_SUCCESS)
        {
            printf("User data OK\r\n");
        }
        else
        {
            PIFS_TEST_ERROR_MSG("User data mismatch!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
#endif
        if (pifs_fclose(file))
        {
            PIFS_TEST_ERROR_MSG("Cannot close file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }
    else
    {
        PIFS_TEST_ERROR_MSG("Cannot open file!\r\n");
    }

    return ret;
}

pifs_status_t pifs_test_large_w(void)
{
    pifs_status_t ret = PIFS_SUCCESS;
    P_FILE * file;
    size_t   written_size = 0;
    size_t   i;
    const char * filename = "large.tst";

    printf("-------------------------------------------------\r\n");
    printf("Large test: writing file\r\n");

    file = pifs_fopen(filename, "w");
    if (file)
    {
        printf("File opened for writing\r\n");
        for (i = 0; i < LARGE_FILE_SIZE && ret == PIFS_SUCCESS; i++)
        {
            generate_buffer(i, filename);
//            print_buffer(test_buf_w, sizeof(test_buf_w), 0);
            written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
            if (written_size != sizeof(test_buf_w))
            {
                PIFS_TEST_ERROR_MSG("Cannot write file!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
        }
        if (pifs_fclose(file))
        {
            PIFS_TEST_ERROR_MSG("Cannot close file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }
    else
    {
        PIFS_TEST_ERROR_MSG("Cannot open file!\r\n");
        ret = PIFS_ERROR_GENERAL;
    }

    return ret;
}

pifs_status_t pifs_test_large_r(void)
{
    pifs_status_t ret = PIFS_SUCCESS;
    P_FILE * file;
    size_t   read_size = 0;
    size_t   i;
    size_t   filesize;
    long int pos;
    const char * filename = "large.tst";

    printf("-------------------------------------------------\r\n");
    printf("Large test: reading file\r\n");

    file = pifs_fopen(filename, "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        filesize = pifs_filesize(filename);
        printf("File size: %i bytes\r\n", filesize);
        for (i = 0; i < LARGE_FILE_SIZE && ret == PIFS_SUCCESS; i++)
        {
            generate_buffer(i, filename);
            read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
            if (read_size != sizeof(test_buf_r))
            {
                PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
            if (i < LARGE_FILE_SIZE - 1)
            {
                if (pifs_feof(file))
                {
                    PIFS_TEST_ERROR_MSG("EOF indicator is wrong!\r\n");
                    ret = PIFS_ERROR_GENERAL;
                }
            }
//            print_buffer(test_buf_r, sizeof(test_buf_r), 0);
            if (ret == PIFS_ERROR_GENERAL)
            {
                ret = check_buffers();
            }
        }
        if (ret == PIFS_SUCCESS)
        {
            ret = pifs_fseek(file, 0, PIFS_SEEK_SET);
            if (ret != PIFS_SUCCESS)
            {
                PIFS_TEST_ERROR_MSG("Cannot seek to beginning of file!\r\n");
            }
        }
        if (ret == PIFS_SUCCESS)
        {
            ret = pifs_fseek(file, filesize, PIFS_SEEK_SET);
            if (ret != PIFS_SUCCESS)
            {
                PIFS_TEST_ERROR_MSG("Cannot seek to end of file!\r\n");
            }
        }
        if (ret == PIFS_SUCCESS)
        {
            pos = pifs_ftell(file);
            printf("Position: %i\r\n", pos);
            if (pos != (long int)filesize)
            {
                PIFS_TEST_ERROR_MSG("Wrong position (%i) while seeked to end of file!\r\n",
                                    pos);
                ret = PIFS_ERROR_GENERAL;
            }
        }
        if (pifs_feof(file))
        {
            printf("EOF indicator OK\r\n");
        }
        else
        {
            PIFS_TEST_ERROR_MSG("EOF indicator is wrong!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
        if (pifs_fclose(file))
        {
            PIFS_TEST_ERROR_MSG("Cannot close file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }
    else
    {
        PIFS_TEST_ERROR_MSG("Cannot open file!\r\n");
        ret = PIFS_ERROR_GENERAL;
    }

    return ret;
}

pifs_status_t pifs_test_wfragment_w(size_t a_fragment_size)
{
    pifs_status_t ret = PIFS_SUCCESS;
    P_FILE * file;
    size_t   written_size = 0;
    size_t   i;
    const char * filename = "fragwr.tst";

    printf("-------------------------------------------------\r\n");
    printf("Fragment write test: writing file\r\n");

    file = pifs_fopen(filename, "w");
    if (file)
    {
        printf("File opened for writing\r\n");
        generate_buffer(2, filename);
        written_size = 0;
        for (i = 0; i < sizeof(test_buf_w) && ret == PIFS_SUCCESS; i += a_fragment_size)
        {
            written_size = pifs_fwrite(&test_buf_w[i], 1, a_fragment_size, file);
            if (written_size != a_fragment_size)
            {
                PIFS_TEST_ERROR_MSG("Cannot write file!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
        }
        if (pifs_fclose(file))
        {
            PIFS_TEST_ERROR_MSG("Cannot close file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }
    else
    {
        PIFS_TEST_ERROR_MSG("Cannot open file!\r\n");
        ret = PIFS_ERROR_GENERAL;
    }

    return ret;
}

pifs_status_t pifs_test_wfragment_r(void)
{
    pifs_status_t ret = PIFS_SUCCESS;
    P_FILE * file;
    size_t   read_size = 0;
    const char * filename = "fragwr.tst";

    printf("-------------------------------------------------\r\n");
    printf("Write fragment test: reading file\r\n");

    memset(test_buf_r, 0, sizeof(test_buf_r));
    file = pifs_fopen(filename, "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        generate_buffer(2, filename);
        read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
        if (read_size != sizeof(test_buf_r))
        {
            PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
        //print_buffer(test_buf_r, sizeof(test_buf_r), 0);
        if (ret == PIFS_SUCCESS)
        {
            ret = check_buffers();
        }
        if (pifs_fclose(file))
        {
            PIFS_TEST_ERROR_MSG("Cannot close file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }
    else
    {
        PIFS_TEST_ERROR_MSG("Cannot open file!\r\n");
        ret = PIFS_ERROR_GENERAL;
    }

    return ret;
}

pifs_status_t pifs_test_rfragment_w(void)
{
    pifs_status_t ret = PIFS_SUCCESS;
    P_FILE * file;
    size_t   written_size = 0;
    const char * filename = "fragrd.tst";

    printf("-------------------------------------------------\r\n");
    printf("Fragment read test: writing file\r\n");

    file = pifs_fopen(filename, "w");
    if (file)
    {
        printf("File opened for writing\r\n");
        generate_buffer(3, filename);
        written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
        written_size += pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
        if (written_size != sizeof(test_buf_w) * 2)
        {
            PIFS_TEST_ERROR_MSG("Cannot write file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
        if (pifs_fclose(file))
        {
            PIFS_TEST_ERROR_MSG("Cannot close file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }
    else
    {
        PIFS_TEST_ERROR_MSG("Cannot open file!\r\n");
        ret = PIFS_ERROR_GENERAL;
    }

    return ret;
}

pifs_status_t pifs_test_rfragment_r(size_t a_fragment_size)
{
    pifs_status_t ret = PIFS_SUCCESS;
    P_FILE * file;
    size_t   read_size = 0;
    size_t   i;
    const char * filename = "fragrd.tst";

    printf("-------------------------------------------------\r\n");
    printf("Read fragment test: reading file\r\n");

    memset(test_buf_r, 0, sizeof(test_buf_r));
    file = pifs_fopen(filename, "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        generate_buffer(3, filename);
        read_size = 0;
        for (i = 0; i < sizeof(test_buf_r) && ret == PIFS_SUCCESS; i += a_fragment_size)
        {
            if (i + a_fragment_size >= sizeof(test_buf_r))
            {
                a_fragment_size = sizeof(test_buf_r) - i;
            }
            read_size = pifs_fread(&test_buf_r[i], 1, a_fragment_size, file);
            if (read_size != a_fragment_size)
            {
                PIFS_TEST_ERROR_MSG("Cannot read file: %i, i: %i!\r\n", read_size, i);
                ret = PIFS_ERROR_GENERAL;
            }
        }
        if (ret == PIFS_SUCCESS)
        {
            ret = check_buffers();
        }
        if (pifs_fclose(file))
        {
            PIFS_TEST_ERROR_MSG("Cannot close file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }
    else
    {
        PIFS_TEST_ERROR_MSG("Cannot open file!\r\n");
    }

    return ret;
}

pifs_status_t pifs_test_rseek_w(void)
{
    pifs_status_t ret = PIFS_SUCCESS;
    P_FILE * file;
    size_t   written_size = 0;
    const char * filename = "seekrd.tst";

    printf("-------------------------------------------------\r\n");
    printf("Seek read test: writing file\r\n");

    file = pifs_fopen(filename, "w");
    if (file)
    {
        printf("File opened for writing\r\n");
        generate_buffer(7, filename);
        written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
        written_size += pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
        if (written_size != 2 * sizeof(test_buf_w))
        {
            PIFS_TEST_ERROR_MSG("Cannot write\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
        if (pifs_fclose(file))
        {
            PIFS_TEST_ERROR_MSG("Cannot close file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }
    else
    {
        PIFS_TEST_ERROR_MSG("Cannot open file!\r\n");
        ret = PIFS_ERROR_GENERAL;
    }

    return ret;
}

pifs_status_t pifs_test_rseek_r(void)
{
    pifs_status_t ret = PIFS_SUCCESS;
    P_FILE * file;
    size_t   read_size = 0;
    const char * filename = "seekrd.tst";

    printf("-------------------------------------------------\r\n");
    printf("Seek read test: reading file\r\n");

    file = pifs_fopen(filename, "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        generate_buffer(7, filename);
        if (pifs_fseek(file, SEEK_TEST_POS, PIFS_SEEK_SET))
        {
            PIFS_TEST_ERROR_MSG("Cannot seek!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
        if (ret == PIFS_SUCCESS)
        {
            read_size = pifs_fread(&test_buf_r[SEEK_TEST_POS], 1, sizeof(test_buf_r) - SEEK_TEST_POS, file);
            if (read_size != sizeof(test_buf_r) - SEEK_TEST_POS)
            {
                PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
        }
        if (ret == PIFS_SUCCESS)
        {
            if (pifs_fseek(file, -sizeof(test_buf_r), PIFS_SEEK_CUR))
            {
                PIFS_TEST_ERROR_MSG("Cannot seek!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
        }
        if (ret == PIFS_SUCCESS)
        {
            read_size = pifs_fread(test_buf_r, 1, SEEK_TEST_POS, file);
            if (read_size != SEEK_TEST_POS)
            {
                PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
        }
        if (ret == PIFS_SUCCESS)
        {
            ret = check_buffers();
        }
        if (ret == PIFS_SUCCESS)
        {
            if (pifs_fseek(file, -sizeof(test_buf_r), PIFS_SEEK_END))
            {
                PIFS_TEST_ERROR_MSG("Cannot seek!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
        }
        if (ret == PIFS_SUCCESS)
        {
            read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
            if (read_size != sizeof(test_buf_r))
            {
                PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
        }
        if (ret == PIFS_SUCCESS)
        {
            ret = check_buffers();
        }
        if (pifs_fclose(file))
        {
            PIFS_TEST_ERROR_MSG("Cannot close file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }
    else
    {
        PIFS_TEST_ERROR_MSG("Cannot open file!\r\n");
        ret = PIFS_ERROR_GENERAL;
    }

    return ret;
}

pifs_status_t pifs_test_wseek_w(void)
{
    pifs_status_t ret = PIFS_SUCCESS;
#if PIFS_ENABLE_FSEEK_BEYOND_FILE
    P_FILE * file;
    size_t   written_size = 0;
    const char * filename = "seekwr.tst";

    printf("-------------------------------------------------\r\n");
    printf("Seek write test: writing file\r\n");
    file = pifs_fopen(filename, "w");
    if (file)
    {
        printf("File opened for writing\r\n");
        generate_buffer(8, filename);
        pifs_fseek(file, SEEK_TEST_POS, PIFS_SEEK_SET);
        written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
        if (written_size != sizeof(test_buf_w))
        {
            PIFS_TEST_ERROR_MSG("Cannot write file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
        if (pifs_fclose(file))
        {
            PIFS_TEST_ERROR_MSG("Cannot close file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }
    else
    {
        PIFS_TEST_ERROR_MSG("Cannot open file!\r\n");
        ret = PIFS_ERROR_GENERAL;
    }
#endif

    return ret;
}

pifs_status_t pifs_test_wseek_r(void)
{
    pifs_status_t ret = PIFS_SUCCESS;
#if PIFS_ENABLE_FSEEK_BEYOND_FILE
    P_FILE * file;
    size_t   read_size = 0;
    const char * filename = "seekwr.tst";

    printf("-------------------------------------------------\r\n");
    printf("Seek write test: reading file\r\n");

    file = pifs_fopen(filename, "r");
    if (file)
    {
        printf("File opened for reading\r\n");
#if PIFS_ENABLE_FSEEK_ERASED_VALUE
        /* First 100 byte shall be 0xFF, due to fseek */
        fill_buffer(test_buf_w, sizeof(test_buf_w), FILL_TYPE_SIMPLE_BYTE, PIFS_FLASH_ERASED_BYTE_VALUE);
        fill_buffer(test_buf_r, sizeof(test_buf_r), FILL_TYPE_SIMPLE_BYTE, PIFS_FLASH_ERASED_BYTE_VALUE);
#else
        /* First 100 byte shall be zero, due to fseek */
        fill_buffer(test_buf_w, sizeof(test_buf_w), FILL_TYPE_SIMPLE_BYTE, PIFS_FLASH_PROGRAMMED_BYTE_VALUE);
        fill_buffer(test_buf_r, sizeof(test_buf_r), FILL_TYPE_SIMPLE_BYTE, PIFS_FLASH_PROGRAMMED_BYTE_VALUE);
#endif
        read_size = pifs_fread(test_buf_r, 1, SEEK_TEST_POS, file);
        if (read_size != SEEK_TEST_POS)
        {
            PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
        if (ret == PIFS_SUCCESS)
        {
            ret = check_buffers();
        }
        if (ret == PIFS_SUCCESS)
        {
            generate_buffer(8, filename);
            read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
            if (read_size != sizeof(test_buf_r))
            {
                PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
        }
        if (ret == PIFS_SUCCESS)
        {
            ret = check_buffers();
        }
        if (pifs_fclose(file))
        {
            PIFS_TEST_ERROR_MSG("Cannot close file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }
    else
    {
        PIFS_TEST_ERROR_MSG("Cannot open file!\r\n");
        ret = PIFS_ERROR_GENERAL;
    }
#endif

    return ret;
}

pifs_status_t pifs_test_list_dir(void)
{
    pifs_status_t ret = PIFS_SUCCESS;
    pifs_DIR * dir;
    struct pifs_dirent * dirent;

    printf("-------------------------------------------------\r\n");
    printf("List directory test\r\n");
    dir = pifs_opendir("/");
    if (dir != NULL)
    {
        while ((dirent = pifs_readdir(dir)))
        {
            printf("%-32s  %i\r\n", dirent->d_name, pifs_filesize(dirent->d_name));
        }
        if (pifs_closedir (dir) != 0)
        {
            PIFS_TEST_ERROR_MSG("Cannot close directory!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }
    else
    {
        PIFS_TEST_ERROR_MSG("Could not open the directory!\r\n");
    }

    return ret;
}

pifs_status_t pifs_test_delta_w(const char * a_filename)
{
    pifs_status_t ret = PIFS_SUCCESS;
    const char  * filename = "delta.tst";
    P_FILE      * file;
    size_t        written_size = 0;
    int           r;

    if (a_filename)
    {
        filename = a_filename;
    }

    printf("-------------------------------------------------\r\n");
    printf("Delta test: writing file\r\n");

    file = pifs_fopen(filename, "w");
    if (file)
    {
        printf("File opened for writing\r\n");
        generate_buffer(33, filename);
        //print_buffer(test_buf_w, sizeof(test_buf_w), 0);
        written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
        if (written_size != sizeof(test_buf_w))
        {
            PIFS_TEST_ERROR_MSG("Cannot write file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
        if (ret == PIFS_SUCCESS)
        {
            r = pifs_fseek(file, 0, PIFS_SEEK_SET);
            if (r != 0)
            {
                PIFS_TEST_ERROR_MSG("Cannot seek!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
        }
        if (ret == PIFS_SUCCESS)
        {
            r = pifs_ftell(file);
            if (r != 0)
            {
                PIFS_TEST_ERROR_MSG("Seek to 0, but position is not 0! Pos: %i\r\n", r);
                ret = PIFS_ERROR_GENERAL;
            }
        }
        if (ret == PIFS_SUCCESS)
        {
            generate_buffer(133, filename);
            written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
            if (written_size != sizeof(test_buf_w))
            {
                PIFS_TEST_ERROR_MSG("Cannot write file!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
        }
        if (ret == PIFS_SUCCESS)
        {
            r = pifs_ftell(file);
            printf("File pos: %i\r\n", r);
            if (r != sizeof(test_buf_w))
            {
                PIFS_TEST_ERROR_MSG("Wrong file position!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
        }
        if (ret == PIFS_SUCCESS)
        {
            generate_buffer(134, filename);
            written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
            if (written_size != sizeof(test_buf_w))
            {
                PIFS_TEST_ERROR_MSG("Cannot write file!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
        }
        if (pifs_fclose(file))
        {
            PIFS_TEST_ERROR_MSG("Cannot close file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }
    else
    {
        PIFS_TEST_ERROR_MSG("Cannot open file!\r\n");
    }

    return ret;
}

pifs_status_t pifs_test_delta_r(const char * a_filename)
{
    pifs_status_t ret = PIFS_SUCCESS;
    const char  * filename = "delta.tst";
    P_FILE      * file;
    size_t        read_size = 0;

    if (a_filename)
    {
        filename = a_filename;
    }

    printf("-------------------------------------------------\r\n");
    printf("Delta test: reading file\r\n");

    file = pifs_fopen(filename, "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        generate_buffer(133, filename);
        read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
        if (read_size != sizeof(test_buf_r))
        {
            PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
        //print_buffer(test_buf_r, sizeof(test_buf_r), 0);
        if (ret == PIFS_SUCCESS)
        {
            ret = check_buffers();
        }
        if (ret == PIFS_SUCCESS)
        {
            generate_buffer(134, filename);
            read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
            if (read_size != sizeof(test_buf_r))
            {
                PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
        }
        //print_buffer(test_buf_r, sizeof(test_buf_r), 0);
        if (ret == PIFS_SUCCESS)
        {
            ret = check_buffers();
        }
        if (ret == PIFS_SUCCESS)
        {
            if (pifs_feof(file))
            {
                printf("EOF indicator OK\r\n");
            }
            else
            {
                PIFS_TEST_ERROR_MSG("EOF indicator is wrong!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
        }
        if (pifs_fclose(file))
        {
            PIFS_TEST_ERROR_MSG("Cannot close file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }
    else
    {
        PIFS_TEST_ERROR_MSG("Cannot open file!\r\n");
    }

    return ret;
}


pifs_status_t pifs_test(void)
{
    pifs_status_t ret = PIFS_SUCCESS;

#if ENABLE_SMALL_FILES_TEST
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_test_small_w();
    }
#endif

#if ENABLE_FULL_WRITE_TEST
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_test_full_w(NULL);
    }
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_test_full_r(NULL);
    }
    if (ret == PIFS_SUCCESS)
    {
        if (pifs_remove("fullwrite.tst") == 0)
        {
            printf("File removed!\r\n");
        }
        else
        {
            PIFS_TEST_ERROR_MSG("Cannot remove file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }
#endif

#if ENABLE_BASIC_TEST
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_test_basic_w(NULL);
    }
#endif

#if ENABLE_LARGE_TEST
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_test_large_w();
    }
#endif

#if ENABLE_WRITE_FRAGMENT_TEST
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_test_wfragment_w(fragment_size);
    }
#endif

#if ENABLE_READ_FRAGMENT_TEST
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_test_rfragment_w();
    }
#endif

#if ENABLE_SEEK_READ_TEST
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_test_rseek_w();
    }
#endif

#if ENABLE_SEEK_WRITE_TEST
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_test_wseek_w();
    }
#endif

#if ENABLE_DELTA_TEST
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_test_delta_w(NULL);
    }
#endif

    /**************************************************************************/
    /**************************************************************************/
    /**************************************************************************/

#if ENABLE_SMALL_FILES_TEST
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_test_small_r();
    }
#endif

#if ENABLE_LARGE_TEST
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_test_large_r();
    }
#endif

#if ENABLE_BASIC_TEST
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_test_basic_r(NULL);
    }
#endif

#if ENABLE_WRITE_FRAGMENT_TEST
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_test_wfragment_r();
    }
#endif

#if ENABLE_READ_FRAGMENT_TEST
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_test_rfragment_r(fragment_size);
    }
#endif

#if ENABLE_SEEK_READ_TEST
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_test_rseek_r();
    }
#endif

#if ENABLE_SEEK_WRITE_TEST
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_test_wseek_r();
    }
#endif

#if ENABLE_DELTA_TEST
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_test_delta_r(NULL);
    }
#endif

#if ENABLE_LIST_DIRECTORY_TEST
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_test_list_dir();
    }
#endif

    if (ret == PIFS_SUCCESS)
    {
        printf("All test passed!\r\n");
    }
    else
    {
        printf("TEST FAILED\r\n");
    }
    printf("END OF TESTS\r\n");
    pifs_print_free_space_info();

    return ret;
}

#if 0
int main(void)
{
    srand(time(0));
    pifs_test();

    return 0;
}
#endif
