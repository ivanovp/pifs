/**
 * @file        parser.c
 * @brief       Command parser
 * @author      Copyright (C) Peter Ivanov, 2008
 *
 * Created:     2007-05-19 11:31:29
 * Last modify: 2010-02-07 16:45:08 ivanovp {Time-stamp}
 * Licence:     GPL
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
            (PARSER_parserCommand[i].commandHandlerCallback) (pureCmd, params);
        }
    }
    return found;
}

