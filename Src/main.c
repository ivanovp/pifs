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

#include "flash.h"
#include "api_pifs.h"
#include "pifs.h"
#include "buffer.h"
#include "term.h"
#include "parser.h"

#define PIFS_DEBUG_LEVEL    5
#include "pifs_debug.h"

int main(int argc, char **argv)
{
    int  i;
    char args[256] = { 0 };

    srand(time(0));
    pifs_status = pifs_init();
    if (argc < 2)
    {
        /* No arguments */
        term_init();
        while (1)
        {
            term_task();
        }
    }
    else
    {
        /* Process arguments */
        PARSER_init(parserCommands);
        for (i = 1; i < argc; i++)
        {
            if (i != 1)
            {
                strncat(args, " ", sizeof(args));
            }
            strncat(args, argv[i], sizeof(args));
        }
//        printf("Arguments: [%s]\r\n", args);
        PARSER_process(args, strlen(args));
    }
    /* This return code will be deliberately avoided to keep PARSER_process()' */
    /* status */
    (void)pifs_delete();

    return pifs_status;
}

