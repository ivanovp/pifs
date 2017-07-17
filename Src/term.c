/**
 * @file        term.c
 * @brief       Terminal command handler
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-15 12:28:40 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "parser.h"
#include "buffer.h"

#define CMD_BUF_SIZE  128

bool_t promptIsEnabled = TRUE;

/**
 * Disable printing of prompt.
 */
void cmdNoPrompt (char* command, char* params)
{
    (void) command;
    (void) params;

    promptIsEnabled = FALSE;
    printf ("OK\r\n");
}

/**
 * Quit from program.
 */
void cmdQuit (char* command, char* params)
{
    (void) command;
    (void) params;

    printf("Quitting...\r\n");
    exit(0);
}

/**
 * Print available commands and brief help.
 * TODO Verbose help of commands.
 */
void cmdHelp (char *command, char *params)
{
    (void) command;
    (void) params;

    uint8_t i;
    const char help_str[] = 
        "\r\n"
        "Help\r\n"
        "----\r\n"
        "\r\n"
        "Available commands:\r\n\r\n"
        "Command  | Brief help\r\n"
        "---------+---------------------------------------------------------------------";
    puts (help_str);
    for (i = 0; i < PARSER_parserCommandSize; i++)
    {
        printf ("%-8s | %s\r\n", 
                PARSER_parserCommand[i].command, 
                PARSER_parserCommand[i].briefHelp);
    }
    printf ("\r\n");
}

static parserCommand_t parserCommands[] =
{
    //command       brief help                          callback function
    {"quit",        "Quit",                             cmdQuit},
    {"q",           "Quit",                             cmdQuit},
    {"noprompt",    "Prompt will not be displayed",     cmdNoPrompt},
    {"help",        "Print help",                       cmdHelp},
    {"?",           "Print help",                       cmdHelp},
    {NULL,          NULL,                               NULL}   // end of command list
};

/**
 * Print prompt to the serial interface.
 */
void printPrompt (void)
{
    if (promptIsEnabled)
    {
        printf ("> ");
    }
}

bool_t getLine(uint8_t * a_buf, size_t a_buf_size)
{
    size_t len;
    fgets((char*)a_buf, a_buf_size, stdin);
    len = strlen((char*)a_buf);
    if (a_buf[len - 1] == ASCII_LF)
    {
        a_buf[len - 1] = 0;
    }
//    print_buffer(a_buf, a_buf_size, 0);
    return TRUE;
}

void term_init (void)
{
    PARSER_init(parserCommands);
    printf ("Type 'help' to get assistance!\r\n\r\n");
    printPrompt();
}

/**
 * Check UART input buffer for new data. It will call parser if new command has got.
 */
void term_task (void)
{
    uint8_t buf[CMD_BUF_SIZE] = { 0 };            /* Current input from the serial line */
    static uint8_t prevBuf[CMD_BUF_SIZE] = { 0 }; /* Previous input from the serial line */
    
    if (getLine (buf, sizeof (buf)))           /* Check if enter was pressed */
    {
        //printf ("buf: [%s]\r\n", buf);
        /* New command was entered */
        if (strlen ((char*)buf))
        {
            if (!PARSER_process ((char*)buf, sizeof (buf)))
            {
                printf ("Unknown command: %s\r\n\r\n", buf);
            }
            else
            {
                /* Save the command if it was valid */
                strncpy ((char*)prevBuf, (char*)buf, sizeof (prevBuf));
            }
        }
        else
        {
            /* Execute previous command if nothing was entered */
            printPrompt ();
            printf ("%s\r\n", prevBuf);
            PARSER_process ((char*)prevBuf, sizeof (prevBuf));
        }
        printPrompt ();
    }
}


