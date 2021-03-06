/**
 * @file        main.c
 * @brief       Entry point of Pi file system's PC emulator
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-09-03 07:24:47 ivanovp {Time-stamp}
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
    if (pifs_status != PIFS_SUCCESS)
    {
        PIFS_ERROR_MSG("Cannot initialize file system: %i\r\n", pifs_status);
        exit(-2);
    }
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

