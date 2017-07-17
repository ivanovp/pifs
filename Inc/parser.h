/**
 * @file        parser.h
 * @brief       Prototypes for parser
 * @author      Copyright (C) Peter Ivanov, 2008
 *
 * Created:     2007-05-19 11:31:29
 * Last modify: 2008-04-24 11:10:35 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef __INCLUDE_PARSER_H
#define __INCLUDE_PARSER_H

#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include "common.h"

#define PARSER_MAX_COMMAND_ITEMS    512
#define PARSER_MAX_COMMAND_LENGTH   32

typedef struct parserCommand_t parserCommand_t;

extern parserCommand_t *PARSER_parserCommand;
extern uint16_t PARSER_parserCommandSize; ///< Number of commands in PARSER_parserCommand

/**
 * Struct for storing commands and their handlers.
 *
 * @author Peter Ivanov
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
 * @author Peter Ivanov
 *
 * @param parserCommand Pointer to struct of commands.
 */
void PARSER_init (parserCommand_t* commands);

bool_t PARSER_process (const char* command, size_t size);

#endif // __INCLUDE_PARSER_H
