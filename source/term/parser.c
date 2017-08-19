/**
 * @file        parser.c
 * @brief       Command parser
 * @author      Copyright (C) Peter Ivanov, 2008
 *
 * Created:     2007-05-19 11:31:29
 * Last modify: 2010-02-07 16:45:08 ivanovp {Time-stamp}
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
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "common.h"

/**
 * Pointer to the table of commands. This variable is initialized by PARSER_init()!
 */
parserCommand_t *PARSER_parserCommand = NULL;
/**
 * Number of commands in PARSER_parserCommand. This variable is initialized by PARSER_init()!
 */
uint16_t PARSER_parserCommandSize = 0; 
static char * actualParam;
static char * params;
static uint32_t paramIdx;
static uint32_t paramSize;

/**
 * Count number of elements in the command array.
 *
 * Input:
 *   @param command Null-terminated string.
 *
 * Output:
 *   @return Number of elements.
 */
uint16_t PARSER_getSize (const parserCommand_t* commands)
{
    uint16_t i;
    for (i = 0; i < PARSER_MAX_COMMAND_ITEMS && commands[i].command; i++);
    return i;
}

static void PARSER_paramInit(char * a_params, uint32_t a_paramMaxSize)
{
    bool_t exit = FALSE;

    actualParam = a_params;
    params = a_params;
    paramIdx = 0;
    if (a_params != NULL)
    {
        paramSize = strnlen(a_params, a_paramMaxSize);
        /* Check if parameters start with whitespace */
        if (a_params[0] == ASCII_SPACE || a_params[0] == ASCII_TAB)
        {
            for (; paramIdx < paramSize && !exit; paramIdx++)
            {
                if (params[paramIdx] != ASCII_SPACE && params[paramIdx] != ASCII_TAB)
                {
                    actualParam = &params[paramIdx];
                    exit = TRUE;
                }
            }
        }
    }
    else
    {
        paramSize = 0;
    }
}

char * PARSER_getNextParam(void)
{
    char  * ret = actualParam;
    bool_t  endOfString = FALSE;
    bool_t  exit = FALSE;
    bool_t  isInString = FALSE;
    char    strChr; /* '"' or '\"' */

    actualParam = NULL;
    for (; paramIdx < paramSize && !exit; paramIdx++)
    {
        if (!isInString)
        {
            /* Normal, not inside string */
            if (!endOfString && (params[paramIdx] == ASCII_APOSTROPHE || params[paramIdx] == ASCII_QUOTE))
            {
                isInString = TRUE;
                strChr = params[paramIdx];
//                params[paramIdx] = 0;
            }
            else if (params[paramIdx] == ASCII_SPACE || params[paramIdx] == ASCII_TAB)
            {
                params[paramIdx] = 0;
                endOfString = TRUE;
            }
            else if (endOfString)
            {
                actualParam = &params[paramIdx];
                paramIdx--;
                exit = TRUE;
            }
        }
        else
        {
            /* Inside string */
            if (params[paramIdx] == strChr)
            {
//                params[paramIdx] = 0;
                endOfString = TRUE;
                isInString = FALSE;
            }
        }
    }

    return ret;
}

/**
 * Initialize parser: calculate number of commands and set up pointer to table.
 *
 * Input:
 *   @param commands Pointer to array of commands.
 */
void PARSER_init (parserCommand_t* commands)
{
    PARSER_parserCommandSize = PARSER_getSize (commands);
    PARSER_parserCommand = commands;
}

/**
 * Parses command and executes it.
 *
 * Input:
 *   @param command Null-terminated string.
 *   @param size Maximum length of command.
 *
 * Output:
 *   @return TRUE: if OK, FALSE: if command is unknown
 */
bool_t PARSER_process (const char* command, size_t size)
{
    uint8_t i;
    bool_t found = FALSE;
    char pureCmd[PARSER_MAX_COMMAND_LENGTH]; // command without parameters
    uint8_t pureCmdLen;
    char *params;

    //printf ("command: [%s]\r\n", command);
    // looking for parameters
    if ((params = strchr (command, ' ')))
    {
        // gotcha!
        pureCmdLen = params - command;
        if (pureCmdLen < sizeof (pureCmd))
        {
            memcpy (pureCmd, command, pureCmdLen);
            pureCmd[pureCmdLen] = 0;
        }
        else
        {
            // internal error
            printf ("Command is too long!\r\n");
            return FALSE;
        }
        params++;
    }
    else
    {
        if (strnlen (command, size) > PARSER_MAX_COMMAND_LENGTH)
        {
            printf ("Command is too long!\r\n");
            return FALSE;
        }
        strncpy (pureCmd, command, sizeof (pureCmd));
    }
    //printf ("pureCmd: [%s]\r\n", pureCmd);
    //printf ("pureCmd len: %i\r\n", strlen (pureCmd));
    //printf ("PARSER_parserCommandSize: %i\r\n", PARSER_parserCommandSize);
    for (i = 0; i < PARSER_parserCommandSize && !found; ++i)
    {
        if (!strncmp (pureCmd, PARSER_parserCommand[i].command, sizeof (pureCmd)))
        {
            //UART1_printf ("callback found: %s\r\n", PARSER_parserCommand[i].command);
            found = TRUE;
            PARSER_paramInit(params, size - pureCmdLen);
            (PARSER_parserCommand[i].commandHandlerCallback) (pureCmd, params);
        }
    }
    return found;
}

