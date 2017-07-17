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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "parser.h"
#include "buffer.h"
#include "flash.h"
#include "flash_test.h"
#include "api_pifs.h"
#include "pifs_test.h"
#include "pifs_helper.h"

#define ENABLE_DOS_ALIAS    0
#define CMD_BUF_SIZE        128

bool_t promptIsEnabled = TRUE;
static uint8_t buf[CMD_BUF_SIZE] = { 0 };     /* Current input from the serial line */
static uint8_t prevBuf[CMD_BUF_SIZE] = { 0 }; /* Previous input from the serial line */
static char buf_r[PIFS_FLASH_PAGE_SIZE_BYTE];
static char buf_w[PIFS_FLASH_PAGE_SIZE_BYTE];

void cmdErase (char* command, char* params)
{
    (void) command;
    (void) params;

    flash_erase_all();
}

void cmdTestFlash (char* command, char* params)
{
    (void) command;
    (void) params;

    flash_test();
}

void cmdTestPifs (char* command, char* params)
{
    (void) command;
    (void) params;

    pifs_test();
}

void cmdListDir (char* command, char* params)
{
    char               * path = "/";
    pifs_DIR           * dir;
    struct pifs_dirent * dirent;

    (void) command;

    if (params)
    {
        path = params;
    }
    printf("List directory '%s'\r\n", path);
    dir = pifs_opendir(path);
    if (dir != NULL)
    {
        while ((dirent = pifs_readdir(dir)))
        {
            printf("%-32s  %i\r\n", dirent->d_name, pifs_filesize(dirent->d_name));
        }
        if (pifs_closedir (dir) != 0)
        {
            printf("Cannot close directory!\r\n");
        }
    }
    else
    {
        printf("Could not open the directory!\r\n");
    }
}

void cmdRemove (char* command, char* params)
{
    (void) command;

    if (params)
    {
        printf("Remove file '%s'\r\n", params);
        if (pifs_remove(params) == PIFS_SUCCESS)
        {
            printf("File removed\r\n");
        }
        else
        {
            printf("ERROR: Cannot remove file!\r\n");
        }
    }
    else
    {
        printf("ERROR: Missing parameter!\r\n");
    }
}

void cmdDumpFile (char* command, char* params)
{
    P_FILE * file;
    size_t   read;
    uint32_t addr = 0;

    (void) command;

    if (params)
    {
        printf("Dump file '%s'\r\n", params);
        file = pifs_fopen(params, "r");
        if (file)
        {
            do
            {
                read = pifs_fread(buf_r, 1, sizeof(buf_r), file);
                if (read > 0)
                {
                    print_buffer(buf_r, read, addr);
                    addr += read;
                }
            } while (read > 0);
            pifs_fclose(file);
        }
        else
        {
            printf("ERROR: Cannot open file!\r\n");
        }
    }
    else
    {
        printf("ERROR: Missing parameter!\r\n");
    }
}

void cmdReadFile (char* command, char* params)
{
    P_FILE * file;
    size_t   read;

    (void) command;

    if (params)
    {
        printf("Read file '%s'\r\n", params);
        file = pifs_fopen(params, "r");
        if (file)
        {
            do
            {
                read = pifs_fread(buf_r, 1, sizeof(buf_r), file);
                if (read > 0)
                {
                    fwrite(buf_r, 1, read, stdout);
                }
            } while (read > 0);
            pifs_fclose(file);
        }
        else
        {
            printf("ERROR: Cannot open file!\r\n");
        }
    }
    else
    {
        printf("ERROR: Missing parameter!\r\n");
    }
}

void cmdCreateFile (char* command, char* params)
{
    P_FILE * file;
    size_t   read;
    size_t   written;

    (void) command;

    if (params)
    {
        printf("Create file '%s'\r\n", params);
        file = pifs_fopen(params, "w");
        if (file)
        {
            do
            {
                read = fread(buf_w, 1, sizeof(buf_w), stdin);
                if (read)
                {
                    written = pifs_fwrite(buf_w, 1, read, file);
                }
            } while (read && written == read && buf_w[0] != 'q');
            pifs_fclose(file);
        }
        else
        {
            printf("ERROR: Cannot open file!\r\n");
        }
    }
    else
    {
        printf("ERROR: Missing parameter!\r\n");
    }
}

void cmdDumpPage (char* command, char* params)
{
    unsigned long int    addr = 0;
    pifs_block_address_t ba;
    pifs_page_address_t  pa;
    pifs_page_offset_t   po;
    pifs_status_t        ret;

    (void) command;

    if (params)
    {
        //printf("Params: [%s]\r\n", params);
        addr = strtoul(params, NULL, 0);
        //printf("Addr: 0x%X\r\n", addr);
        po = addr % PIFS_FLASH_PAGE_SIZE_BYTE;
        pa = (addr / PIFS_FLASH_PAGE_SIZE_BYTE) % PIFS_FLASH_PAGE_PER_BLOCK;
        ba = (addr / PIFS_FLASH_PAGE_SIZE_BYTE) / PIFS_FLASH_PAGE_PER_BLOCK;
        printf("Dump page %s\r\n", pifs_ba_pa2str(ba, pa));
        if (ba < PIFS_FLASH_BLOCK_NUM_ALL)
        {
            ret = pifs_flash_read(ba, pa, po, buf_r, sizeof(buf_r));
            if (ret == PIFS_SUCCESS)
            {
                print_buffer(buf_r, sizeof(buf_r), addr);
            }
            else
            {
                printf("ERROR: Cannot read from flash memory!\r\n");
            }
        }
        else
        {
            printf("ERROR: Invalid address!\r\n");
        }
    }
    else
    {
        printf("ERROR: Missing parameter!\r\n");
    }
}

void cmdPifsInfo (char* command, char* params)
{
    (void) command;
    (void) params;

    pifs_print_fs_info();
    pifs_print_header_info();
}

void cmdFreeSpaceInfo (char* command, char* params)
{
    (void) command;
    (void) params;

    pifs_print_free_space_info();
}

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
 * Test parameter handling.
 */
void cmdParamTest (char* command, char* params)
{
    char * param;
    (void) command;


    printf("Params: %s\r\n", params);
    while ((param = PARSER_getNextParam()))
    {
        printf("Param: [%s]\r\n", param);
    }
    printf("Params: %s\r\n", params);
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

parserCommand_t parserCommands[] =
{
    //command       brief help                          callback function
    {"erase",       "Erase flash",                      cmdErase},
    {"e",           "Erase flash",                      cmdErase},
    {"tf",          "Test flash",                       cmdTestFlash},
    {"tstflash",    "Test flash",                       cmdTestFlash},
    {"tp",          "Test Pi file system",              cmdTestPifs},
    {"tstpifs",     "Test Pi file system",              cmdTestPifs},
    {"ls",          "List directory",                   cmdListDir},
#if ENABLE_DOS_ALIAS
    {"dir",         "List directory",                   cmdListDir},
#endif
    {"rm",          "Remove file",                      cmdRemove},
#if ENABLE_DOS_ALIAS
    {"del",         "Remove file",                      cmdRemove},
#endif
    {"dump",        "Dump file in hexadecimal format",  cmdDumpFile},
    {"d",           "Dump file in hexadecimal format",  cmdDumpFile},
    {"cat",         "Read file",                        cmdReadFile},
#if ENABLE_DOS_ALIAS
    {"type",        "Read file",                        cmdReadFile},
#endif
    {"create",      "Create file, write until 'q'",     cmdCreateFile},
    {"dumpp",       "Dump page in hexadecimal format",  cmdDumpPage},
    {"dp",          "Dump page in hexadecimal format",  cmdDumpPage},
    {"info",        "Print info of Pi file system",     cmdPifsInfo},
    {"i",           "Print info of Pi file system",     cmdPifsInfo},
    {"free",        "Print info of free space",         cmdFreeSpaceInfo},
    {"f",           "Print info of free space",         cmdFreeSpaceInfo},
    {"quit",        "Quit",                             cmdQuit},
    {"q",           "Quit",                             cmdQuit},
    {"noprompt",    "Prompt will not be displayed",     cmdNoPrompt},
    {"p",           "Parameter test",                   cmdParamTest},
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
    bool_t is_removed = FALSE;
    fgets((char*)a_buf, a_buf_size, stdin);
    do
    {
        is_removed = FALSE;
        len = strlen((char*)a_buf);
        if (a_buf[len - 1] == ASCII_LF)
        {
            a_buf[len - 1] = 0;
            is_removed = TRUE;
        }
        len = strlen((char*)a_buf);
        if (a_buf[len - 1] == ASCII_CR)
        {
            a_buf[len - 1] = 0;
            is_removed = TRUE;
        }
    } while (is_removed);
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


