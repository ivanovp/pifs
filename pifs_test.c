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
#include <string.h>

#include "api_pifs.h"
#include "pifs.h"
#include "pifs_debug.h"

#define PIFS_TEST_ERROR_MSG(...)    do { \
        fprintf(stderr, "%s ERROR: ", __FUNCTION__); \
        fprintf(stderr, __VA_ARGS__); \
    } while (0);

pifs_status_t pifs_test(void)
{
    pifs_status_t ret = PIFS_FLASH_INIT_ERROR;
    P_FILE * file;

    ret = pifs_init();
    PIFS_ASSERT(ret == PIFS_SUCCESS);

    file = pifs_fopen("test.dat", "w");
    if (file)
    {
        printf("File opened\r\n");
    }

    ret = pifs_delete();
    PIFS_ASSERT(ret == PIFS_SUCCESS);

    return ret;
}

int main(void)
{
    pifs_test();

    return 0;
}

