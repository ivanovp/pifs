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
#include "pifs_entry.h"
#include "pifs_test.h"
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

#define PIFS_TEST_ERROR_MSG(...)    do {    \
        printf("%s ERROR: ", __FUNCTION__); \
        printf(__VA_ARGS__);                \
        pifs_delete();                      \
        SOFTWARE_BREAKPOINT();              \
        exit(-1);                           \
    } while (0);

uint8_t test_buf_w[TEST_BUF_SIZE] __attribute__((aligned(4)));
uint8_t test_buf_r[TEST_BUF_SIZE] __attribute__((aligned(4)));
size_t  testfull_written_buffers = 0;

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

pifs_status_t pifs_test_small_w(void)
{
    pifs_status_t ret = PIFS_SUCCESS;
    P_FILE * file;
    size_t   i = 0;
    char     filename[32];
    size_t   written_size = 0;

    printf("-------------------------------------------------\r\n");
    printf("Small file test: writing files\r\n");

    for (i = 0; i < PIFS_ENTRY_NUM_MAX / 2; i++)
    {
        snprintf(filename, sizeof(filename), "small%lu.tst", i);
        file = pifs_fopen(filename, "w");
        if (file)
        {
            printf("File opened for writing %s\r\n", filename);
            generate_buffer(i);
            //print_buffer(test_buf_w, sizeof(test_buf_w), 0);
            written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
            if (written_size != sizeof(test_buf_w))
            {
                PIFS_TEST_ERROR_MSG("Cannot write file: %i!\r\n", pifs_errno);
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

    for (i = 0; i < PIFS_ENTRY_NUM_MAX / 2; i++)
    {
        snprintf(filename, sizeof(filename), "small%lu.tst", i);
        file = pifs_fopen(filename, "r");
        if (file)
        {
            printf("File opened for reading %s\r\n", filename);
            generate_buffer(i);
            read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
            if (read_size != sizeof(test_buf_r))
            {
                PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
            }
            //print_buffer(test_buf_r, sizeof(test_buf_r), 0);
            check_buffers();
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

pifs_status_t pifs_test_full_w(void)
{
    pifs_status_t ret = PIFS_SUCCESS;
    P_FILE * file;
    size_t   written_size = 0;
    size_t   i;

    printf("-------------------------------------------------\r\n");
    printf("Full write test: writing file\r\n");

    file = pifs_fopen("fullwrite.tst", "w");
    if (file)
    {
        printf("File opened for writing\r\n");
        for (i = 0; i < TEST_FULL_PAGE_NUM; i++)
        {
            printf("full_w: %i\r\n", i + 1);
            generate_buffer(i + 55);
//            print_buffer(test_buf_w, sizeof(test_buf_w), 0);
            written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
            if (written_size != sizeof(test_buf_w))
            {
                PIFS_DEBUG_MSG("Media full!!!\r\n");
                testfull_written_buffers = i;
                break;
            }
        }
        if (pifs_fclose(file))
        {
            PIFS_TEST_ERROR_MSG("Cannot close file!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
        PIFS_DEBUG_MSG("%i buffers written.\r\n", testfull_written_buffers);
    }
    else
    {
        PIFS_TEST_ERROR_MSG("Cannot open file!\r\n");
        ret = PIFS_ERROR_GENERAL;
    }

    return ret;
}

pifs_status_t pifs_test_full_r(void)
{
    pifs_status_t ret = PIFS_SUCCESS;
    P_FILE * file;
    size_t   read_size = 0;
    size_t   i;

    printf("-------------------------------------------------\r\n");
    printf("Full write test: reading file\r\n");

    file = pifs_fopen("fullwrite.tst", "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        for (i = 0; i < testfull_written_buffers; i++)
        {
            printf("full_r: %i/%i\r\n", i + 1, testfull_written_buffers);
            generate_buffer(i + 55);
            read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
            if (read_size != sizeof(test_buf_r))
            {
                printf("Read size differs!\r\n");
                ret = PIFS_ERROR_GENERAL;
            }
            //print_buffer(test_buf_r, sizeof(test_buf_r), 0);
            check_buffers();
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

pifs_status_t pifs_test_basic_w(const char * a_filename)
{
    pifs_status_t ret = PIFS_SUCCESS;
    const char  * filename = "basic.tst";
    P_FILE      * file;
    size_t        written_size = 0;

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
        generate_buffer(42);
        //print_buffer(test_buf_w, sizeof(test_buf_w), 0);
        written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
        if (written_size != sizeof(test_buf_w))
        {
            PIFS_TEST_ERROR_MSG("Cannot write file!\r\n");
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

pifs_status_t pifs_test_basic_r(const char * a_filename)
{
    pifs_status_t ret = PIFS_SUCCESS;
    const char  * filename = "basic.tst";
    P_FILE      * file;
    size_t        read_size = 0;

    if (a_filename)
    {
        filename = a_filename;
    }

    printf("-------------------------------------------------\r\n");
    printf("Basic test: reading file\r\n");

    file = pifs_fopen(filename, "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        generate_buffer(42);
        read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
        if (read_size != sizeof(test_buf_r))
        {
            PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
        }
        //print_buffer(test_buf_r, sizeof(test_buf_r), 0);
        check_buffers();
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

    printf("-------------------------------------------------\r\n");
    printf("Large test: writing file\r\n");

    file = pifs_fopen("large.tst", "w");
    if (file)
    {
        printf("File opened for writing\r\n");
        for (i = 0; i < 2 * PIFS_MAP_ENTRY_PER_PAGE + 2; i++)
        {
            generate_buffer(i);
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

    printf("-------------------------------------------------\r\n");
    printf("Large test: reading file\r\n");

    file = pifs_fopen("large.tst", "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        for (i = 0; i < 2 * PIFS_MAP_ENTRY_PER_PAGE + 2; i++)
        {
            generate_buffer(i);
            read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
            if (read_size != sizeof(test_buf_r))
            {
                PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
            }
//            print_buffer(test_buf_r, sizeof(test_buf_r), 0);
            check_buffers();
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

pifs_status_t pifs_test_wfragment_w(void)
{
    pifs_status_t ret = PIFS_SUCCESS;
    P_FILE * file;
    size_t   written_size = 0;
    size_t   i;

    printf("-------------------------------------------------\r\n");
    printf("Fragment write test: writing file\r\n");

    file = pifs_fopen("fragwr.tst", "w");
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

    printf("-------------------------------------------------\r\n");
    printf("Write fragment test: reading file\r\n");

    memset(test_buf_r, 0, sizeof(test_buf_r));
    file = pifs_fopen("fragwr.tst", "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        generate_buffer(2);
        read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
        if (read_size != sizeof(test_buf_r))
        {
            PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
        }
        //print_buffer(test_buf_r, sizeof(test_buf_r), 0);
        check_buffers();
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

const size_t size_delta = 5;

pifs_status_t pifs_test_rfragment_w(void)
{
    pifs_status_t ret = PIFS_SUCCESS;
    P_FILE * file;
    size_t   written_size = 0;

    printf("-------------------------------------------------\r\n");
    printf("Fragment read test: writing file\r\n");

    file = pifs_fopen("fragrd.tst", "w");
    if (file)
    {
        printf("File opened for writing\r\n");
        generate_buffer(3);
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

pifs_status_t pifs_test_rfragment_r(void)
{
    pifs_status_t ret = PIFS_SUCCESS;
    P_FILE * file;
    size_t   read_size = 0;
    size_t   i;

    printf("-------------------------------------------------\r\n");
    printf("Read fragment test: reading file\r\n");

    memset(test_buf_r, 0, sizeof(test_buf_r));
    file = pifs_fopen("fragrd.tst", "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        generate_buffer(3);
        read_size = 0;
        for (i = 0; i < sizeof(test_buf_r); i += size_delta)
        {
            read_size = pifs_fread(&test_buf_r[i], 1, size_delta, file);
            if (read_size != size_delta)
            {
                PIFS_TEST_ERROR_MSG("Cannot read file: %i, i: %i!\r\n", read_size, i);
            }
        }
        check_buffers();
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

    printf("-------------------------------------------------\r\n");
    printf("Seek read test: writing file\r\n");

    file = pifs_fopen("seekrd.tst", "w");
    if (file)
    {
        printf("File opened for writing\r\n");
        generate_buffer(7);
        written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
        written_size += pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
        if (written_size != 2 * sizeof(test_buf_w))
        {
            printf("Cannot write\r\n");
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

    printf("-------------------------------------------------\r\n");
    printf("Seek read test: reading file\r\n");

    file = pifs_fopen("seekrd.tst", "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        generate_buffer(7);
        if (pifs_fseek(file, SEEK_TEST_POS, PIFS_SEEK_SET))
        {
            printf("Cannot seek!\r\n");
        }
        read_size = pifs_fread(&test_buf_r[SEEK_TEST_POS], 1, sizeof(test_buf_r) - SEEK_TEST_POS, file);
        if (read_size != sizeof(test_buf_r) - SEEK_TEST_POS)
        {
            PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
        }
        if (pifs_fseek(file, -sizeof(test_buf_r), PIFS_SEEK_CUR))
        {
            printf("Cannot seek!\r\n");
        }
        read_size = pifs_fread(test_buf_r, 1, SEEK_TEST_POS, file);
        if (read_size != SEEK_TEST_POS)
        {
            PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
        }
        check_buffers();
        if (pifs_fseek(file, -sizeof(test_buf_r), PIFS_SEEK_END))
        {
            printf("Cannot seek!\r\n");
        }
        read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
        if (read_size != sizeof(test_buf_r))
        {
            PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
        }
        check_buffers();
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

    printf("-------------------------------------------------\r\n");
    printf("Seek write test: writing file\r\n");
    file = pifs_fopen("seekwr.tst", "w");
    if (file)
    {
        printf("File opened for writing\r\n");
        generate_buffer(8);
        pifs_fseek(file, SEEK_TEST_POS, PIFS_SEEK_SET);
        written_size = pifs_fwrite(test_buf_w, 1, sizeof(test_buf_w), file);
        if (written_size != sizeof(test_buf_w))
        {
            PIFS_TEST_ERROR_MSG("Cannot write file!\r\n");
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

    printf("-------------------------------------------------\r\n");
    printf("Seek write test: reading file\r\n");

    file = pifs_fopen("seekwr.tst", "r");
    if (file)
    {
        printf("File opened for reading\r\n");
        /* First 100 byte shall be zero, due to fseek */
        fill_buffer(test_buf_w, sizeof(test_buf_w), FILL_TYPE_SIMPLE_BYTE, 0);
        fill_buffer(test_buf_r, sizeof(test_buf_r), FILL_TYPE_SIMPLE_BYTE, 0);
        read_size = pifs_fread(test_buf_r, 1, SEEK_TEST_POS, file);
        if (read_size != SEEK_TEST_POS)
        {
            PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
        }
        check_buffers();

        generate_buffer(8);
        read_size = pifs_fread(test_buf_r, 1, sizeof(test_buf_r), file);
        if (read_size != sizeof(test_buf_r))
        {
            PIFS_TEST_ERROR_MSG("Cannot read file!\r\n");
        }
        check_buffers();
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
            printf("Cannot close directory!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }
    else
    {
        printf("Could not open the directory!\r\n");
    }

    return ret;
}

pifs_status_t pifs_test(void)
{
    pifs_status_t ret = PIFS_ERROR_FLASH_INIT;

//    ret = pifs_init();
//    PIFS_ASSERT(ret == PIFS_SUCCESS);
#if ENABLE_SMALL_FILES_TEST
    pifs_test_small_w();
#endif

#if ENABLE_FULL_WRITE_TEST
    pifs_test_full_w();
    pifs_test_full_r();
    if (pifs_remove("fullwrite.tst") == 0)
    {
        printf("File removed!\r\n");
    }
    else
    {
        printf("Cannot remove file!\r\n");
        ret = PIFS_ERROR_GENERAL;
    }
#endif

#if ENABLE_BASIC_TEST
    pifs_test_basic_w(NULL);
#endif

#if ENABLE_LARGE_TEST
    pifs_test_large_w();
#endif

#if ENABLE_WRITE_FRAGMENT_TEST
    pifs_test_wfragment_w();
#endif

#if ENABLE_READ_FRAGMENT_TEST
    pifs_test_rfragment_w();
#endif

#if ENABLE_SEEK_READ_TEST
    pifs_test_rseek_w();
#endif

#if ENABLE_SEEK_WRITE_TEST
    pifs_test_wseek_w();
#endif

    /**************************************************************************/
    /**************************************************************************/
    /**************************************************************************/

#if ENABLE_SMALL_FILES_TEST
    pifs_test_small_r();
#endif

#if ENABLE_LARGE_TEST
    pifs_test_large_r();
#endif

#if ENABLE_BASIC_TEST
    pifs_test_basic_r(NULL);
#endif

#if ENABLE_WRITE_FRAGMENT_TEST
    pifs_test_wfragment_r();
#endif

#if ENABLE_READ_FRAGMENT_TEST
    pifs_test_rfragment_r();
#endif

#if ENABLE_SEEK_READ_TEST
    pifs_test_rseek_r();
#endif

#if ENABLE_SEEK_WRITE_TEST
    pifs_test_wseek_r();
#endif

#if ENABLE_LIST_DIRECTORY_TEST
    pifs_test_list_dir();
#endif

    printf("END OF TESTS\r\n");
    pifs_print_free_space_info();
//    ret = pifs_delete();
//    PIFS_ASSERT(ret == PIFS_SUCCESS);

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
