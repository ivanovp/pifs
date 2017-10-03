/**
 * @file        term.c
 * @brief       Terminal command handler
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-08-15 16:25:42 ivanovp {Time-stamp}
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
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "parser.h"
#include "buffer.h"
#include "flash.h"
#include "flash_test.h"
#include "api_pifs.h"
#include "pifs_test.h"
#include "pifs_helper.h"
#include "pifs_delta.h"
#include "pifs_fsbm.h"
#include "pifs_wear.h"
#include "pifs_merge.h"
#include "pifs_os.h"

#if ENABLE_DHT
#include "dht.h"
#include "fatfs.h"
#endif

#if ENABLE_DATETIME
#include "stm32f4xx_hal.h"
#endif

#if STM32F4xx || STM32F1xx
#include "uart.h"
#endif

#define ENABLE_DOS_ALIAS    0
#define CMD_BUF_SIZE        128

bool_t promptIsEnabled = TRUE;
static uint8_t buf[CMD_BUF_SIZE] = { 0 };     /* Current input from the serial line */
static uint8_t prevBuf[CMD_BUF_SIZE] = { 0 }; /* Previous input from the serial line */
static char buf_r[PIFS_FLASH_PAGE_SIZE_BYTE];
static char buf_w[PIFS_FLASH_PAGE_SIZE_BYTE];
pifs_status_t pifs_status;

#if ENABLE_DHT
static uint8_t copy_buf[PIFS_LOGICAL_PAGE_SIZE_BYTE];
#endif

bool_t getLine(uint8_t * a_buf, size_t a_buf_size);

void cmdErase (char* command, char* params)
{
    unsigned long int    addr = 0;
    pifs_block_address_t ba;
    char               * param;
    pifs_size_t          cntr = 1;

    (void) command;

    if (params)
    {
        //printf("Params: [%s]\r\n", params);
        param = PARSER_getNextParam();
        if (param[0] == '-')
        {
            if (param[1] == 'a')
            {
                addr = PIFS_FLASH_BLOCK_RESERVED_NUM * PIFS_FLASH_BLOCK_SIZE_BYTE;
                cntr = PIFS_FLASH_BLOCK_NUM_FS;
            }
            else if (param[1] == 'A')
            {
                addr = 0;
                cntr = PIFS_FLASH_BLOCK_NUM_ALL;
            }
        }
        else
        {
            addr = strtoul(param, NULL, 0);
            param = PARSER_getNextParam();
            if (param)
            {
                cntr = strtoul(param, NULL, 0);
            }
        }
        //printf("Addr: 0x%X\r\n", addr);
        //po = addr % PIFS_FLASH_PAGE_SIZE_BYTE;
        ba = (addr / PIFS_FLASH_PAGE_SIZE_BYTE) / PIFS_FLASH_PAGE_PER_BLOCK;

        pifs_delete();
        pifs_flash_init();
        while (cntr--)
        {
            printf("Erasing block %i... ", ba);
            if (pifs_flash_erase(ba) == PIFS_SUCCESS)
            {
                printf("Done.\r\n");
            }
            else
            {
                printf("Error!\r\n");
            }
            ba++;
        }
        pifs_flash_delete();
        pifs_init();
    }
    else
    {
        printf("ERROR: Missing parameter!\r\n");
    }
}

void cmdTestFlash (char* command, char* params)
{
    (void) command;
    (void) params;

    pifs_delete();
    pifs_flash_init();
    flash_test();
    flash_erase_all();
    pifs_flash_delete();
    pifs_init();
}

void cmdTestPifs (char* command, char* params)
{
    (void) command;
    (void) params;

    pifs_status = pifs_test();
}

void cmdTestPifsBasic (char* command, char* params)
{
    char buf[PIFS_FILENAME_LEN_MAX];
    char * param;
    char * filename;

    (void) command;
    (void) params;

    param = PARSER_getNextParam();
    if (param[0] == '-' && param[1] == 'r')
    {
        filename = pifs_tmpnam(buf);
    }
    else
    {
        filename = param;
    }

    pifs_test_basic_w(filename);
    pifs_test_basic_r(filename);
}

void cmdTestPifsSmall (char* command, char* params)
{
    (void) command;
    (void) params;

    pifs_test_small_w();
    pifs_test_small_r();
}

void cmdTestPifsLarge (char* command, char* params)
{
    (void) command;
    (void) params;

    pifs_test_large_w();
    pifs_test_large_r();
}

void cmdTestPifsFull (char* command, char* params)
{
    const char * filename;
    const char * mode;

    (void) command;
    (void) params;

    filename = PARSER_getNextParam();
    mode = PARSER_getNextParam();
    if (mode)
    {
        printf("Mode: %s\r\n", mode);
        if (strchr(mode, 'w'))
        {
            pifs_test_full_w(filename);
        }
        if (strchr(mode, 'r'))
        {
            pifs_test_full_r(filename);
        }
    }
    else
    {
        pifs_test_full_w(filename);
        pifs_test_full_r(filename);
    }
}

void cmdTestPifsFragment (char* command, char* params)
{
    size_t fragment_size = 5;
    char * param;

    (void) command;
    (void) params;

    if (params)
    {
        param = PARSER_getNextParam();
        if (param)
        {
            fragment_size = strtoul(param, NULL, 0);
        }
    }

    printf("Fragment size: %i bytes\r\n", fragment_size);

    pifs_test_wfragment_w(fragment_size);
    pifs_test_wfragment_r();
    pifs_test_rfragment_w();
    pifs_test_rfragment_r(fragment_size);
}

void cmdTestPifsSeek (char* command, char* params)
{
    (void) command;
    (void) params;

    pifs_test_rseek_w();
    pifs_test_rseek_r();
    pifs_test_wseek_w();
    pifs_test_wseek_r();
}

void cmdTestPifsDelta (char* command, char* params)
{
    char * filename;
    char * mode;

    (void) command;
    (void) params;

    filename = PARSER_getNextParam();
    mode = PARSER_getNextParam();
    if (mode)
    {
        printf("Mode: %s\r\n", mode);
        if (strchr(mode, 'w'))
        {
            pifs_test_delta_w(filename);
        }
        if (strchr(mode, 'r'))
        {
            pifs_test_delta_r(filename);
        }
    }
    else
    {
        pifs_test_delta_w(filename);
        pifs_test_delta_r(filename);
    }
}

#if PIFS_ENABLE_DIRECTORIES
void cmdTestPifsDir (char* command, char* params)
{
    (void) command;
    (void) params;

    pifs_test_dir_w();
    pifs_test_dir_r();
}
#endif

void cmdPageInfo (char* command, char* params)
{
    unsigned long int    addr = 0;
    //pifs_page_offset_t   po;
    char               * param;
    pifs_size_t          cntr = 1;

    (void) command;

    if (params)
    {
        //printf("Params: [%s]\r\n", params);
        param = PARSER_getNextParam();
        if (param[0] == '-' && param[1] == 'a')
        {
            addr = 0;
            cntr = PIFS_LOGICAL_PAGE_NUM_ALL;
        }
        else
        {
            addr = strtoul(param, NULL, 0);
            param = PARSER_getNextParam();
            if (param)
            {
                cntr = strtoul(param, NULL, 0);
            }
        }
        pifs_print_page_info(addr, cntr);
    }
    else
    {
        printf("ERROR: Missing parameter!\r\n");
    }
}

void cmdWearLevel (char* command, char* params)
{
    pifs_block_address_t    ba;
    pifs_wear_level_entry_t wear_level;
    pifs_status_t           ret = PIFS_SUCCESS;

    (void) command;
    (void) params;

    printf("Block | Erase count\r\n");
    printf("------+------------\r\n");
    for (ba = PIFS_FLASH_BLOCK_RESERVED_NUM;
         ba < PIFS_FLASH_BLOCK_NUM_ALL && ret == PIFS_SUCCESS;
         ba++)
    {
        ret = pifs_get_wear_level(ba, &pifs.header, &wear_level);
        printf("%5i | %i\r\n", ba, wear_level.wear_level_cntr);
    }
}

void cmdLeastWearedBlocks(char* command, char* params)
{
    pifs_status_t           ret = PIFS_SUCCESS;
    pifs_wear_level_entry_t wear_level;
    pifs_size_t             i;

    (void) command;
    (void) params;

    ret = pifs_generate_least_weared_blocks(&pifs.header);

    printf("Block | Erase count\r\n");
    printf("------+------------\r\n");
    for (i = 0;
         i < PIFS_LEAST_WEARED_BLOCK_NUM && ret == PIFS_SUCCESS;
         i++)
    {
        ret = pifs_get_wear_level(pifs.header.least_weared_blocks[i].block_address, &pifs.header, &wear_level);
        printf("%5i | %i\r\n", pifs.header.least_weared_blocks[i].block_address, wear_level.wear_level_cntr);
    }
}

void cmdMostWearedBlocks(char* command, char* params)
{
    pifs_status_t           ret = PIFS_SUCCESS;
    pifs_wear_level_entry_t wear_level;
    pifs_size_t             i;

    (void) command;
    (void) params;

    ret = pifs_generate_most_weared_blocks(&pifs.header);

    printf("Block | Erase count\r\n");
    printf("------+------------\r\n");
    for (i = 0;
         i < PIFS_MOST_WEARED_BLOCK_NUM && ret == PIFS_SUCCESS;
         i++)
    {
        ret = pifs_get_wear_level(pifs.header.most_weared_blocks[i].block_address, &pifs.header, &wear_level);
        printf("%5i | %i\r\n", pifs.header.most_weared_blocks[i].block_address, wear_level.wear_level_cntr);
    }
}

void cmdEmptyBlock(char* command, char* params)
{
    pifs_status_t        ret;
    pifs_block_address_t ba;
    char               * param;
    bool_t               is_emptied;

    (void) command;

    if (params)
    {
        param = PARSER_getNextParam();
        ba = strtoul(param, NULL, 0);
        printf("Empty block %i...\r\n", ba);
        ret = pifs_empty_block(ba, &is_emptied);
        printf("Emptied: %s, ret: %i\r\n", pifs_yes_no(is_emptied), ret);
    }
}

void cmdStaticWear(char* command, char* params)
{
    pifs_status_t   ret;
    pifs_size_t     max_block_num = 1;
    char          * param;

    (void) command;

    if (params)
    {
        param = PARSER_getNextParam();
        max_block_num = strtoul(param, NULL, 0);
    }
    printf("Static wear leveling on %i blocks...\r\n", max_block_num);
    ret = pifs_static_wear_leveling(max_block_num);
    printf("Ret: %i\r\n", ret);
}

#if ENABLE_DHT
extern void ftest(void);
#endif

void cmdDebug (char* command, char* params)
{
    pifs_status_t ret;
    pifs_block_address_t ba;

    (void) command;
    (void) params;

#if ENABLE_DHT
    ftest();
#else
    printf("Find to be released block...\r\n");
    ba = PIFS_BLOCK_ADDRESS_INVALID;
    ret = pifs_find_to_be_released_block(1, PIFS_BLOCK_TYPE_DATA, PIFS_FLASH_BLOCK_RESERVED_NUM,
                                         &pifs.header, &ba);
    printf("ret: %i, ba: %i\r\n", ret, ba);

    printf("Find to free block...\r\n");
    ret = pifs_find_block_wl(PIFS_MANAGEMENT_BLOCK_NUM,
                             PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT | PIFS_BLOCK_TYPE_DATA,
                             TRUE,
                             &pifs.header, &ba);
    printf("ret: %i, ba: %i\r\n", ret, ba);
#endif
}

#if ENABLE_DHT
extern void save_measurement(void);

void cmdDht (char* command, char* params)
{
    (void) command;
    (void) params;

    if (DHT_read())
    {
        save_measurement();
    }
}

extern FATFS sd_fat_fs;
extern char disk_path[4];

void cmdCpDat (char* command, char* params)
{
    char               * path = ".";
    pifs_DIR           * dir;
    struct pifs_dirent * dirent;
    pifs_status_t    ret = PIFS_ERROR_NO_MORE_RESOURCE;
    pifs_file_t    * file = NULL;
    FIL              file2;
    FRESULT          fres;
    pifs_size_t      read_bytes;
    pifs_size_t      written_bytes;

    (void) command;
    (void) params;

    printf("Mounting SD card... ");
    fres = f_mount(&sd_fat_fs, (TCHAR const*)disk_path, 1);
    if (fres == FR_OK)
    {
        printf("Done.\r\n");

        dir = pifs_opendir(path);
        if (dir != NULL)
        {
            while ((dirent = pifs_readdir(dir)))
            {
                printf("Copying %s... ", dirent->d_name);

                file = pifs_fopen(dirent->d_name, "r");
                if (file)
                {
                    fres = f_open(&file2, dirent->d_name, FA_WRITE | FA_CREATE_ALWAYS);
                    if (fres == FR_OK)
                    {
                        do
                        {
                            read_bytes = pifs_fread(copy_buf, 1,
                                                    PIFS_LOGICAL_PAGE_SIZE_BYTE, file);
                            if (read_bytes > 0)
                            {
                                written_bytes = 0;
                                fres = f_write(&file2, copy_buf, read_bytes, &written_bytes);
                                if (fres != FR_OK || read_bytes != written_bytes)
                                {
                                    printf("ERROR: Cannot write to card! Code: %i, read bytes: %i, written bytes: %i\r\n",
                                           fres, read_bytes, written_bytes);
                                }
                            }
                        } while (read_bytes > 0 && read_bytes == written_bytes);
                        fres = f_close(&file2);
                        if (fres != FR_OK)
                        {
                            printf("ERROR: Cannot close file on card! Code: %i\r\n", fres);
                        }
                    }
                    else
                    {
                        printf("ERROR: Cannot create on card! Code: %i\r\n", fres);
                    }
                    ret = pifs_fclose(file);
                    if (ret != PIFS_SUCCESS)
                    {
                        printf("ERROR: Cannot close file on flash! Code: %i\r\n\r\n", ret);
                    }
                    if (ret == PIFS_SUCCESS && fres == FR_OK)
                    {
                        printf("Done.\r\n");
                    }
                }
                else
                {
                    printf("ERROR: Cannot open file '%s'\r\n", dirent->d_name);
                }
            }
            if (pifs_closedir (dir) != 0)
            {
                printf("ERROR: Cannot close directory!\r\n");
            }
        }
        else
        {
            printf("ERROR: Could not open the directory!\r\n");
        }

        printf("Un-mounting SD card... ");
        fres = f_mount(NULL, (TCHAR const*)disk_path, 0);
        if (fres == FR_OK)
        {
            printf("Done.\r\n");
        }
        else
        {
            printf("ERROR: Cannot unmount SD card: %i\r\n", fres);
        }
    }
    else
    {
        printf("ERROR: Cannot mount SD card: %i\r\n", fres);
    }
}
#endif

#if ENABLE_DATETIME
extern RTC_HandleTypeDef hrtc;

void printDateTime(void)
{
    HAL_StatusTypeDef stat;
    RTC_DateTypeDef date;
    RTC_TimeTypeDef time;

    /* First time shall be read */
    stat = HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    if (stat == HAL_OK)
    {
        printf("Hour: %i, min: %i, sec: %i\r\n", time.Hours, time.Minutes, time.Seconds);
    }
    /* After date shall be read */
    stat = HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
    if (stat == HAL_OK)
    {
        printf("Year: %i, month: %i, day: %i, weekday: %i\r\n",
               date.Year, date.Month, date.Date, date.WeekDay);
    }
}

void cmdDate (char* command, char* params)
{
    HAL_StatusTypeDef stat;
    RTC_DateTypeDef date = { 0 };
    bool_t param_ok = FALSE;
    char * param;

    (void) command;
    (void) params;

    if (params)
    {
        param = PARSER_getNextParam();
        if (param[0] == '-' && param[1] == 'h')
        {
            printf("Usage:\r\n");
            printf("  date <year> <month> <date> <dayofweek>\r\n");
            printf("\r\n");
            printf("Parameters:\r\n");
            printf("year: year - 2000, 17 is 2017.\r\n");
            printf("dayofweek: 1 Monday, 2 Tuesday, 7 Sunday\r\n");
            printf("\r\n");
            printf("Example:\r\n");
            printf("date 17 8 28 1\r\n");
            return;
        }
        date.Year = strtoul(param, NULL, 0);
        param = PARSER_getNextParam();
        if (param)
        {
            date.Month = strtoul(param, NULL, 0);
            param = PARSER_getNextParam();
            if (param)
            {
                date.Date = strtoul(param, NULL, 0);
                param = PARSER_getNextParam();
                if (param)
                {
                    date.WeekDay = strtoul(param, NULL, 0);
                    param_ok = TRUE;
                }
            }
        }
        if (param_ok)
        {
            printf("Year: %i, month: %i, day: %i, weekday: %i\r\n",
                   date.Year, date.Month, date.Date, date.WeekDay);
            stat = HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);
            if (stat == HAL_OK)
            {
                printf("Date setting done.\r\n");
            }
            else
            {
                printf("ERROR: cannot set date! Code: %i\r\n", stat);
            }
        }
        else
        {
            printf("Missing parameter!\r\n");
        }
    }
    else
    {
        printDateTime();
    }
}

void cmdTime (char* command, char* params)
{
    HAL_StatusTypeDef stat;
    RTC_TimeTypeDef time = { 0 };
    bool_t param_ok = FALSE;
    char * param;
    signed long l;

    (void) command;
    (void) params;

    if (params)
    {
        param = PARSER_getNextParam();
        if (param[0] == '-' && param[1] == 'h')
        {
            printf("Usage:\r\n");
            printf("  time <hour> <min> <sec> <daylightsaving>\r\n");
            printf("\r\n");
            printf("Parameters:\r\n");
            printf("daylightsaving: Can be 1, -1 or 0.\r\n");
            printf("\r\n");
            printf("Example:\r\n");
            printf("time 14 5 0 0\r\n");
            return;
        }
        time.Hours = strtoul(param, NULL, 0);
        param = PARSER_getNextParam();
        if (param)
        {
            time.Minutes = strtoul(param, NULL, 0);
            param = PARSER_getNextParam();
            if (param)
            {
                time.Seconds = strtoul(param, NULL, 0);
                param = PARSER_getNextParam();
                if (param)
                {
                    l = strtol(param, NULL, 0);
                    if (l == 1)
                    {
                        time.DayLightSaving = RTC_DAYLIGHTSAVING_ADD1H;
                    }
                    else if (l == -1)
                    {
                        time.DayLightSaving = RTC_DAYLIGHTSAVING_SUB1H;
                    }
                    else
                    {
                        time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
                    }
                    param_ok = TRUE;
                }
            }
        }
        if (param_ok)
        {
            printf("Hour: %i, min: %i, sec: %i\r\n", time.Hours, time.Minutes, time.Seconds);
            stat = HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
            if (stat == HAL_OK)
            {
                printf("Time setting done.\r\n");
            }
            else
            {
                printf("ERROR: cannot set time! Code: %i\r\n", stat);
            }
        }
        else
        {
            printf("Missing parameter!\r\n");
        }
    }
    else
    {
        printDateTime();
    }
}
#endif

void cmdCheckFilesystem (char* command, char* params)
{
    (void) command;
    (void) params;

    pifs_check();
}

void cmdListDir (char* command, char* params)
{
    char               * path = ".";
    pifs_DIR           * dir;
    struct pifs_dirent * dirent;
    char               * param;
    bool_t               long_list = FALSE;
    bool_t               examine = FALSE;
    bool_t               find_deleted = FALSE;

    (void) params;

    if (strcmp(command, "l") == 0)
    {
        long_list = TRUE;
        examine = TRUE;
        //find_deleted = TRUE;
    }

    while ((param = PARSER_getNextParam()))
    {
        if (param[0] == '-')
        {
            switch (param[1])
            {
                case 'l':
                    long_list = TRUE;
                    break;
                case 'e':
                    examine = TRUE;
                    break;
                case 'd':
                    find_deleted = TRUE;
                    break;
                default:
                    printf("Unknown switch: %c\r\n", param[1]);
                    break;
            }
        }
        else
        {
            path = param;
        }
    }
    printf("List directory '%s'\r\n", path);
    dir = pifs_opendir(path);
    if (dir != NULL)
    {
        /* Changing internal variable of pifs_dir_t */
        ((pifs_dir_t*) dir)->find_deleted = find_deleted;
        while ((dirent = pifs_readdir(dir)))
        {
            printf("%-32s", dirent->d_name);
            if (long_list)
            {
#if PIFS_ENABLE_DIRECTORIES
                if (PIFS_IS_DIR(dirent->d_attrib))
                {
                    printf("     <DIR>");
                }
                else
#endif
                {
                    printf("  %8i", dirent->d_filesize);
                }
            }
            if (examine)
            {
                printf("  %-20s", pifs_ba_pa2str(dirent->d_first_map_block_address, dirent->d_first_map_page_address));
            }
            if (PIFS_IS_DELETED(dirent->d_attrib))
            {
                printf(" DELETED");
            }
            printf("\r\n");
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
    bool_t               all = FALSE;
    char               * param;
    char               * path = ".";
    pifs_DIR           * dir;
    struct pifs_dirent * dirent;

    (void) command;

    if (params)
    {
        while ((param = PARSER_getNextParam()))
        {
            if (param[0] == '-')
            {
                switch (param[1])
                {
                    case 'a':
                        all = TRUE;
                        break;
                    default:
                        printf("Unknown switch: %c\r\n", param[1]);
                        break;
                }
            }
            else
            {
                printf("Remove file '%s'... ", param);
                if (pifs_remove(param) == PIFS_SUCCESS)
                {
                    printf("Done.\r\n");
                }
                else
                {
                    printf("ERROR: Cannot remove file!\r\n");
                }
            }
        }
        if (all)
        {
            dir = pifs_opendir(path);
            if (dir != NULL)
            {
                while ((dirent = pifs_readdir(dir)))
                {
                    if (!PIFS_IS_DIR(dirent->d_attrib))
                    {
                        printf("Remove file '%s'... ", dirent->d_name);
                        if (pifs_remove(dirent->d_name) == PIFS_SUCCESS)
                        {
                            printf("Done.\r\n");
                        }
                        else
                        {
                            printf("ERROR: Cannot remove file!\r\n");
                        }
                    }
                }
                if (pifs_closedir (dir) != 0)
                {
                    printf("Cannot close directory!\r\n");
                }
            }
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
    size_t   file_size;

    (void) command;

    if (params)
    {
        file_size = pifs_filesize(params);
        printf("File size: %i bytes\r\n", file_size);
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
            printf("End position: %i bytes\r\n", pifs_ftell(file));
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

static void write_file(const char* a_filename, const char* a_mode)
{
    P_FILE * file;
    size_t   read;
    size_t   written;

    file = pifs_fopen(a_filename, a_mode);
    if (file)
    {
        do
        {
            /* Wait until text is entered */
            while (!getLine((uint8_t*)buf_w, sizeof(buf_w)))
            {
            }
            /* Append CR LF */
            strncat(buf_w, "\r\n", sizeof(buf_w));
            read = strlen(buf_w);
            //print_buffer(buf_w, read, 0);
            if (read)
            {
                written = pifs_fwrite(buf_w, 1, read, file);
                if (read != written)
                {
                    printf("ERROR: Cannot write file!\r\n");
                }
            }
        } while (read && written == read && buf_w[0] != 'q');
        pifs_fclose(file);
    }
    else
    {
        printf("ERROR: Cannot open file!\r\n");
    }

}

void cmdCreateFile (char* command, char* params)
{
    (void) command;

    if (params)
    {
        printf("Create file '%s'\r\n", params);
        write_file(params, "w");
    }
    else
    {
        printf("ERROR: Missing parameter!\r\n");
    }
}

void cmdAppendFile (char* command, char* params)
{
    (void) command;

    if (params)
    {
        printf("Append file '%s'\r\n", params);
        write_file(params, "a");
    }
    else
    {
        printf("ERROR: Missing parameter!\r\n");
    }
}

void cmdCopyFile (char* command, char* params)
{
    const char * old_name;
    const char * new_name;
    int ret;

    (void) command;

    if (params)
    {
        old_name = PARSER_getNextParam();
        new_name = PARSER_getNextParam();
        if (old_name && new_name)
        {
            printf("Copying file '%s' to '%s'... ", old_name, new_name);
            ret = pifs_copy(old_name, new_name);
            if (ret == 0)
            {
                printf("Done.\r\n");
            }
            else
            {
                printf("Error: %i\r\n", ret);
            }
        }
    }
}


void cmdRenameFile (char* command, char* params)
{
    const char * old_name;
    const char * new_name;
    int ret;

    (void) command;

    if (params)
    {
        old_name = PARSER_getNextParam();
        new_name = PARSER_getNextParam();
        if (old_name && new_name)
        {
            printf("Renaming file '%s' to '%s'... ", old_name, new_name);
            ret = pifs_rename(old_name, new_name);
            if (ret == 0)
            {
                printf("Done.\r\n");
            }
            else
            {
                printf("Error: %i\r\n", ret);
            }
        }
    }
}

#if PIFS_ENABLE_DIRECTORIES
void cmdMakeDir (char* command, char* params)
{
    const char * name;
    int ret;

    (void) command;

    if (params)
    {
        name = PARSER_getNextParam();
        if (name)
        {
            printf("Creating directory '%s'... ", name);
            ret = pifs_mkdir(name);
            if (ret == 0)
            {
                printf("Done.\r\n");
            }
            else
            {
                printf("Error: %i\r\n", ret);
            }
        }
    }
}

void cmdChDir (char* command, char* params)
{
    char cwd[PIFS_PATH_LEN_MAX];
    const char * name;
    int ret;

    (void) command;

    if (params)
    {
        name = PARSER_getNextParam();
        if (name)
        {
            printf("Changing directory to '%s'... ", name);
            ret = pifs_chdir(name);
            if (ret == 0)
            {
                printf("Done.\r\n");
                printf("Current working directory: %s\r\n", pifs_getcwd(cwd, sizeof(cwd)));
            }
            else
            {
                printf("Error: %i\r\n", ret);
            }
        }
    }
}

void cmdRemoveDir (char* command, char* params)
{
    const char * name;
    int ret;

    (void) command;

    if (params)
    {
        name = PARSER_getNextParam();
        if (name)
        {
            printf("Removing directory '%s'... ", name);
            ret = pifs_rmdir(name);
            if (ret == 0)
            {
                printf("Done.\r\n");
            }
            else
            {
                printf("Error: %i\r\n", ret);
            }
        }
    }
}

void cmdGetCurrWorkDir(char* command, char* params)
{
    char cwd[PIFS_PATH_LEN_MAX];

    (void) command;
    (void) params;

    printf("Current working directory: %s\r\n", pifs_getcwd(cwd, sizeof(cwd)));
}

#endif

void cmdDumpPage (char* command, char* params)
{
    unsigned long int    addr = 0;
    pifs_block_address_t ba;
    pifs_page_address_t  pa;
    pifs_page_offset_t   po;
    pifs_status_t        ret;
    char               * param;
    pifs_size_t          cntr = 1;

    (void) command;

    if (params)
    {
        //printf("Params: [%s]\r\n", params);
        param = PARSER_getNextParam();
        addr = strtoul(param, NULL, 0);
        param = PARSER_getNextParam();
        if (param)
        {
            cntr = strtoul(param, NULL, 0);
        }
        //printf("Addr: 0x%X\r\n", addr);
        po = addr % PIFS_FLASH_PAGE_SIZE_BYTE;
        pa = (addr / PIFS_FLASH_PAGE_SIZE_BYTE) % PIFS_FLASH_PAGE_PER_BLOCK;
        ba = (addr / PIFS_FLASH_PAGE_SIZE_BYTE) / PIFS_FLASH_PAGE_PER_BLOCK;
        do
        {
            printf("Dump page %s\r\n", pifs_flash_ba_pa2str(ba, pa));
            if (ba < PIFS_FLASH_BLOCK_NUM_ALL)
            {
                ret = pifs_flash_read(ba, pa, po, buf_r, sizeof(buf_r));
                if (ret == PIFS_SUCCESS)
                {
                    print_buffer(buf_r, sizeof(buf_r), ba * PIFS_FLASH_BLOCK_SIZE_BYTE
                                 + pa * PIFS_FLASH_PAGE_SIZE_BYTE + po);
                }
                else
                {
                    printf("ERROR: Cannot read from flash memory, error code: %i!\r\n", ret);
                }
            }
            else
            {
                printf("ERROR: Invalid address!\r\n");
                break;
            }
            pifs_inc_ba_pa(&ba, &pa);
        } while (--cntr);
    }
    else
    {
        printf("ERROR: Missing parameter!\r\n");
    }
}

void cmdMap (char* command, char* params)
{
    unsigned long int    addr = 0;
    pifs_block_address_t ba;
    pifs_page_address_t  pa;
    //pifs_page_offset_t   po;

    (void) command;

    if (params)
    {
        addr = strtoul(params, NULL, 0);
        //printf("Addr: 0x%X\r\n", addr);
        //po = addr % PIFS_LOGICAL_PAGE_SIZE_BYTE;
        pa = (addr / PIFS_LOGICAL_PAGE_SIZE_BYTE) % PIFS_LOGICAL_PAGE_PER_BLOCK;
        ba = (addr / PIFS_LOGICAL_PAGE_SIZE_BYTE) / PIFS_LOGICAL_PAGE_PER_BLOCK;

        (void)pifs_print_map_page(ba, pa, UINT32_MAX);
    }
}

void cmdFindDelta (char* command, char* params)
{
    unsigned long int    addr = 0;
    pifs_block_address_t ba;
    pifs_page_address_t  pa;
    pifs_status_t        ret;
    pifs_block_address_t dba;
    pifs_page_address_t  dpa;
    bool_t               is_map_full;

    (void) command;

    if (params)
    {
        addr = strtoul(params, NULL, 0);
        //printf("Addr: 0x%X\r\n", addr);
        pa = (addr / PIFS_LOGICAL_PAGE_SIZE_BYTE) % PIFS_LOGICAL_PAGE_PER_BLOCK;
        ba = (addr / PIFS_LOGICAL_PAGE_SIZE_BYTE) / PIFS_LOGICAL_PAGE_PER_BLOCK;
        printf("Find delta of page %s\r\n", pifs_ba_pa2str(ba, pa));
        ret = pifs_find_delta_page(ba, pa, &dba, &dpa, &is_map_full,
                                   &pifs.header);
        if (ret == PIFS_SUCCESS)
        {
            printf("                -> %s\r\n\r\nMap is full: %i\r\n",
                   pifs_ba_pa2str(dba, dpa), is_map_full);
        }
        else
        {
            printf("ERROR: Cannot find delta page, error code: %i\r\n", ret);
        }
    }
}

void cmdFileInfo (char* command, char* params)
{
    pifs_file_t * file;
    size_t   read;
    size_t   file_size;
    pifs_address_t address = { PIFS_BLOCK_ADDRESS_INVALID, PIFS_PAGE_ADDRESS_INVALID };

    (void) command;

    if (params)
    {
        file_size = pifs_filesize(params);
        printf("File name: '%s'\r\n", params);
        printf("File size: %i bytes\r\n", file_size);
        file = pifs_fopen(params, "r");
        if (file)
        {
            do
            {
                if ((address.block_address != file->actual_map_address.block_address)
                        && (address.page_address != file->actual_map_address.page_address))
                {
                    printf("Map address: %s\r\n", pifs_address2str(&file->actual_map_address));
                    address = file->actual_map_address;
                }
                printf("Position address: %s\r\n", pifs_address2str(&file->rw_address));
                read = pifs_fread(buf_r, 1, sizeof(buf_r), file);
            } while (read > 0);
            printf("End position: %i bytes\r\n", pifs_ftell(file));
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

const char * block_type2str(pifs_block_address_t a_block_address)
{
    const char * str = "Data";
    if (pifs_is_block_type(a_block_address, PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT, &pifs.header))
    {
        str = "PriMgmt";
    }
    else if (pifs_is_block_type(a_block_address, PIFS_BLOCK_TYPE_SECONDARY_MANAGEMENT, &pifs.header))
    {
        str = "SecMgmt";
    }
#if PIFS_FLASH_BLOCK_RESERVED_NUM
    else if (pifs_is_block_type(a_block_address, PIFS_BLOCK_TYPE_RESERVED, &pifs.header))
    {
        str = "Reservd";
    }
#endif

    return str;
}

void cmdBlockInfo (char* command, char* params)
{
    unsigned long int    addr = PIFS_FLASH_BLOCK_RESERVED_NUM * PIFS_FLASH_BLOCK_SIZE_BYTE;
    pifs_block_address_t ba;
    char               * param;
    pifs_size_t          cntr = PIFS_FLASH_BLOCK_NUM_FS;
    pifs_size_t          free_data_pages;
    pifs_size_t          free_management_pages;
    pifs_size_t          to_be_released_data_pages;
    pifs_size_t          to_be_released_management_pages;
    pifs_wear_level_entry_t wear_level;
    pifs_status_t        ret = PIFS_SUCCESS;

    (void) command;

    if (params)
    {
        //printf("Params: [%s]\r\n", params);
        param = PARSER_getNextParam();
        if (param[0] == '-')
        {
            if (param[1] == 'a')
            {
                addr = PIFS_FLASH_BLOCK_RESERVED_NUM * PIFS_FLASH_BLOCK_SIZE_BYTE;
                cntr = PIFS_FLASH_BLOCK_NUM_FS;
            }
        }
        else
        {
            addr = strtoul(param, NULL, 0);
            param = PARSER_getNextParam();
            if (param)
            {
                cntr = strtoul(param, NULL, 0);
            }
        }
    }
    //printf("Addr: 0x%X\r\n", addr);
    //po = addr % PIFS_LOGICAL_PAGE_SIZE_BYTE;
    ba = (addr / PIFS_LOGICAL_PAGE_SIZE_BYTE) / PIFS_LOGICAL_PAGE_PER_BLOCK;

    printf("      | Type    | Wear  | Free pages  | TBR pages\r\n");
    printf("Block |         | Level | Data | Mgmt | Data | Mgmt\r\n");
    printf("------+---------+-------+------+------+------+------\r\n");
    while (cntr-- && (ret == PIFS_SUCCESS || ret == PIFS_ERROR_NO_MORE_SPACE))
    {
        ret = pifs_get_pages(TRUE, ba, 1, &free_management_pages, &free_data_pages);
        if (ret == PIFS_SUCCESS || ret == PIFS_ERROR_NO_MORE_SPACE)
        {
            ret = pifs_get_pages(FALSE, ba, 1,
                                 &to_be_released_management_pages,
                                 &to_be_released_data_pages);
        }
        if (ret == PIFS_SUCCESS || ret == PIFS_ERROR_NO_MORE_SPACE)
        {
            ret = pifs_get_wear_level(ba, &pifs.header, &wear_level);
        }
        if (ret == PIFS_SUCCESS)
        {
            printf("%5i | %-7s | %5i | %4i | %4i | %4i | %4i\r\n", ba,
                   block_type2str(ba),
                   wear_level.wear_level_cntr,
                   free_data_pages,
                   free_management_pages,
                   to_be_released_data_pages,
                   to_be_released_management_pages);
        }
        else
        {
            printf("ERROR: Cannot get page count or wear level!\r\n");
        }
        ba++;
    }
}

void cmdFlashStat (char* command, char* params)
{
    (void) command;
    (void) params;

    pifs_flash_print_stat();
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
 * Test HardFault handler.
 */
void cmdHardFaultTest (char* command, char* params)
{
    int x;
    int y;
    int z;

    (void) command;
    (void) params;

    printf("Causing division by zero error in one second!\r\n");
    PIFS_OS_DELAY_MS(1000);

    y = 10;
    z = 0;
    x = y / z;

    printf("x: %i\r\n", x);
}

/**
 * Quit from program.
 */
void cmdQuit (char* command, char* params)
{
    (void) command;
    (void) params;

    printf("Quitting...\r\n");
    pifs_delete();
    exit(pifs_status);
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
    {"c",           "Check file system",                cmdCheckFilesystem},
    {"ls",          "List directory",                   cmdListDir},
    {"l",           "List directory",                   cmdListDir},
#if ENABLE_DOS_ALIAS
    {"dir",         "List directory",                   cmdListDir},
#endif
    {"rm",          "Remove file",                      cmdRemove},
#if ENABLE_DOS_ALIAS
    {"del",         "Remove file",                      cmdRemove},
#endif
    {"dumpf",       "Dump file in hexadecimal format",  cmdDumpFile},
    {"df",          "Dump file in hexadecimal format",  cmdDumpFile},
    {"cat",         "Read file",                        cmdReadFile},
#if ENABLE_DOS_ALIAS
    {"type",        "Read file",                        cmdReadFile},
#endif
    {"create",      "Create file, write until 'q'",     cmdCreateFile},
    {"append",      "Append file, write until 'q'",     cmdAppendFile},
    {"cp",          "Copy file",                        cmdCopyFile},
    {"rename",      "Rename file",                      cmdRenameFile},
#if PIFS_ENABLE_DIRECTORIES
    {"mkdir",       "Make directory",                   cmdMakeDir},
    {"cd",          "Change directory",                 cmdChDir},
    {"rmdir",       "Remove directory",                 cmdRemoveDir},
    {"cwd",         "Get current working directory",    cmdGetCurrWorkDir},
#endif
    {"dump",        "Dump flash page in hexadecimal format", cmdDumpPage},
    {"d",           "Dump flash page in hexadecimal format", cmdDumpPage},
    {"map",         "Print map page",                   cmdMap},
    {"m",           "Print map page",                   cmdMap},
    {"fd",          "Find delta pages of a page",       cmdFindDelta},
    {"fi",          "Print info about a file",          cmdFileInfo},
    {"info",        "Print info of Pi file system",     cmdPifsInfo},
    {"i",           "Print info of Pi file system",     cmdPifsInfo},
    {"free",        "Print info of free space",         cmdFreeSpaceInfo},
    {"f",           "Print info of free space",         cmdFreeSpaceInfo},
    {"bi",          "Print info of block",              cmdBlockInfo},
    {"pi",          "Check if page is free/to be released/erased", cmdPageInfo},
    {"w",           "Print wear level list",            cmdWearLevel},
    {"lw",          "Print least weared blocks' list",  cmdLeastWearedBlocks},
    {"mw",          "Print most weared blocks' list",   cmdMostWearedBlocks},
    {"eb",          "Empty block",                      cmdEmptyBlock},
    {"sw",          "Static wear leveling",             cmdStaticWear},
    {"fs",          "Print flash's statistics",         cmdFlashStat},
    {"erase",       "Erase flash",                      cmdErase},
    {"e",           "Erase flash",                      cmdErase},
    {"tstflash",    "Test flash",                       cmdTestFlash},
    {"tstpifs",     "Test Pi file system: all",         cmdTestPifs},
    {"tp",          "Test Pi file system: all",         cmdTestPifs},
    {"tb",          "Test Pi file system: basic",       cmdTestPifsBasic},
    {"ts",          "Test Pi file system: small files", cmdTestPifsSmall},
    {"tl",          "Test Pi file system: large file",  cmdTestPifsLarge},
    {"tf",          "Test Pi file system: full write",  cmdTestPifsFull},
    {"tfrag",       "Test Pi file system: fragment",    cmdTestPifsFragment},
    {"tsk",         "Test Pi file system: seek",        cmdTestPifsSeek},
    {"td",          "Test Pi file system: delta",       cmdTestPifsDelta},
#if PIFS_ENABLE_DIRECTORIES
    {"tdir",        "Test Pi file system: directories", cmdTestPifsDir},
#endif
    {"y",           "Debug command",                    cmdDebug},
#if ENABLE_DHT
    {"dht",         "Measure humidity",                 cmdDht},
    {"cpdat",       "Copy measured data to SD/MMC card", cmdCpDat},
#endif
#if ENABLE_DATETIME
    {"date",        "Set/get date",                     cmdDate},
    {"time",        "Set/get time",                     cmdTime},
#endif
    {"hf",          "HardFault test",                   cmdHardFaultTest},
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

#if STM32F4xx || STM32F1xx
    UART_getLine(a_buf, a_buf_size);
#else
    fgets((char*)a_buf, a_buf_size, stdin);
#endif
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
    printf("Pi file system v%i.%i terminal\r\n",
           PIFS_MAJOR_VERSION, PIFS_MINOR_VERSION);
    printf("Compiled on " __DATE__ " " __TIME__ "\r\n");
    printf("Copyright (C) Peter Ivanov <ivanovp@gmail.com>, 2017\r\n");
    printf("\r\n");
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


