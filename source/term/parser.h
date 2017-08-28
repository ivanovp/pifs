/**
 * @file        parser.h
 * @brief       Prototypes for parser
 * @author      Copyright (C) Peter Ivanov, 2008
 *
 * Created:     2007-05-19 11:31:29
 * Last modify: 2017-08-21 21:21:06 ivanovp {Time-stamp}
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
#ifndef __INCLUDE_PARSER_H
#define __INCLUDE_PARSER_H

#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include "common.h"

#define PARSER_MAX_COMMAND_ITEMS    512
#define PARSER_MAX_COMMAND_LENGTH   32

#ifdef __cplusplus
extern "C" {
#endif

typedef struct parserCommand_t parserCommand_t;

extern parserCommand_t *PARSER_parserCommand;
extern uint16_t PARSER_parserCommandSize; ///< Number of commands in PARSER_parserCommand

/**
 * Struct for storing commands and their handlers.
 */
struct parserCommand_t
{
    /**
     * Text of command.
     */
    char* command;
    
    /**
     * Brief help of command.
     */
    char* briefHelp;

    /**
     * Function to call if command started.
     */
    void (*commandHandlerCallback) (char *command, char *params);
};

/**
 * Count items of commands array.
 *
 * Input:
 *   @param commands Pointer of array to count.
 *
 * Output:
 *   @return The calculated size.
 */
uint16_t PARSER_getSize (const parserCommand_t* commands);

/**
 * Initialize parser system. After this you can call PARSER_process().
 * Example:
<pre>
parserCommand_t commands[] =
{
    //command   brief help      callback function
    {"help",    "Print help",   cmdHelp},
    {"?",       "Print help",   cmdHelp},
    {NULL,      NULL,           NULL}   // end of command list
};

int main ()
{
    // ...
    PARSER_init (commands);
}
</pre>
 *
 * @param parserCommand Pointer to struct of commands.
 */
void PARSER_init (parserCommand_t* commands);

bool_t PARSER_process (const char* command, size_t size);

char * PARSER_getNextParam(void);

#ifdef __cplusplus
}
#endif

#endif // __INCLUDE_PARSER_H
