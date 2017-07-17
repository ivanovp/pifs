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
#include "term.h"

#define PIFS_DEBUG_LEVEL    5
#include "pifs_debug.h"

int main(void)
{
    srand(time(0));
    term_init();
    while (1)
    {
        term_task();
    }

    return 0;
}

