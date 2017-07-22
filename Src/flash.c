/**
 * @file        flash.c
 * @brief       NOR flash emulator
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-15 12:28:40 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#include <stdio.h>
#include <string.h>

#include "api_pifs.h"
#include "flash.h"
#include "pifs_debug.h"
#include "pifs_helper.h"
#include "buffer.h"

#define FLASH_DEBUG     1

#if FLASH_DEBUG
#define FLASH_ERROR_MSG(...)    do { \
        printf("%s ERROR: ", __FUNCTION__); \
        printf(__VA_ARGS__); \
        fflush(stdout); \
        exit(-1); \
    } while (0);
#else
#define FLASH_ERROR_MSG(...)
#endif

#define FLASH_EMU_FILENAME      "flash.bin" /**< Name of memory file */
#define FLASH_STAT_FILENAME     "flash.stt" /**< Name of statistics file */

typedef struct
{
    size_t read_cntr;
    size_t write_cntr;
    size_t erase_cntr;
} flash_page_stat_t;

static FILE * flash_file = NULL;
static FILE * stat_file = NULL;
static uint8_t flash_page_buf[PIFS_FLASH_PAGE_SIZE_BYTE] = { 0 };
static flash_page_stat_t flash_stat[PIFS_FLASH_BLOCK_NUM_ALL][PIFS_FLASH_PAGE_PER_BLOCK] = { { { 0  } } };

pifs_status_t pifs_flash_init(void)
{
    pifs_status_t ret = PIFS_ERROR_FLASH_INIT;
    pifs_block_address_t ba;

    PIFS_ASSERT(flash_file == NULL);
    flash_file = fopen(FLASH_EMU_FILENAME, "rb+");
    if (flash_file)
    {
        stat_file = fopen(FLASH_STAT_FILENAME, "rb+");
        if (stat_file)
        {
            /* Memory and statistics file already exists */
            if (fread(flash_stat, 1, sizeof(flash_stat), stat_file) == sizeof(flash_stat))
            {
                ret = PIFS_SUCCESS;
            }
        }
        else
        {
            FLASH_ERROR_MSG("Flash file exists, but statistics file is missing!\r\n");
        }
    }
    else
    {
        /* Memory file has not created yet */
        flash_file = fopen(FLASH_EMU_FILENAME, "wb+");
        if (flash_file)
        {
            ret = PIFS_SUCCESS;
            /* Erase whole memory */
            for (ba = PIFS_FLASH_BLOCK_RESERVED_NUM; ba < PIFS_FLASH_BLOCK_NUM_ALL && ret == PIFS_SUCCESS; ba++)
            {
                ret = pifs_flash_erase(ba);
            }

            if (ret == PIFS_SUCCESS)
            {
                stat_file = fopen(FLASH_STAT_FILENAME, "wb+");
                if (!stat_file)
                {
                    FLASH_ERROR_MSG("Cannot create statistics file!\r\n");
                    ret = PIFS_ERROR_FLASH_INIT;
                }
            }
        }
        else
        {
            FLASH_ERROR_MSG("Cannot create flash file!\r\n");
        }
    }

    return ret;
}

pifs_status_t pifs_flash_delete(void)
{
    pifs_status_t ret = PIFS_ERROR_GENERAL;

    pifs_flash_print_stat();
    if (flash_file)
    {
        if (!fclose(flash_file))
        {
            ret = PIFS_SUCCESS;
        }
        flash_file = NULL;
    }
    if (stat_file)
    {
        ret = PIFS_ERROR_GENERAL;

        fseek(stat_file, 0, SEEK_SET);
        if (fwrite(flash_stat, 1, sizeof(flash_stat), stat_file) == sizeof(flash_stat))
        {
            if (!fclose(stat_file))
            {
                ret = PIFS_SUCCESS;
            }
        }
    }

    return ret;
}

pifs_status_t pifs_flash_read(pifs_block_address_t a_block_address, pifs_page_address_t a_page_address, pifs_page_offset_t a_page_offset, void * const a_buf, size_t a_buf_size)
{
    pifs_status_t ret = PIFS_ERROR_FLASH_READ;
    long unsigned int offset = a_block_address * PIFS_FLASH_BLOCK_SIZE_BYTE
            + a_page_address * PIFS_FLASH_PAGE_SIZE_BYTE
            + a_page_offset;
    size_t read_count = 0;

    PIFS_ASSERT(flash_file);
    if ((offset + a_buf_size) <= PIFS_FLASH_SIZE_BYTE_ALL
        #if PIFS_FLASH_BLOCK_RESERVED_NUM
            && offset >= (PIFS_FLASH_BLOCK_RESERVED_NUM * PIFS_FLASH_BLOCK_SIZE_BYTE)
        #endif
            )
    {
        PIFS_ASSERT(fseek(flash_file, offset, SEEK_SET) == 0);
        read_count = fread(a_buf, 1, a_buf_size, flash_file);
        if (read_count == a_buf_size)
        {
            ret = PIFS_SUCCESS;
        }
        flash_stat[a_block_address][a_page_address].read_cntr++;
    }
    else
    {
        FLASH_ERROR_MSG("Trying to read from invalid flash address! BA%i/PA%i/OFS%i\r\n",
                        a_block_address, a_page_address, a_page_offset);
    }

    return ret;
}

pifs_status_t pifs_flash_write(pifs_block_address_t a_block_address, pifs_page_address_t a_page_address, pifs_page_address_t a_page_offset, const void * const a_buf, size_t a_buf_size)
{
    pifs_status_t ret = PIFS_ERROR_FLASH_WRITE;
    long unsigned int offset = a_block_address * PIFS_FLASH_BLOCK_SIZE_BYTE
            + a_page_address * PIFS_FLASH_PAGE_SIZE_BYTE
            + a_page_offset;
    size_t write_count = 0;
    size_t read_count = 0;
    pifs_page_offset_t i;
    uint8_t * buf8 = (uint8_t*) a_buf;
    
    PIFS_ASSERT(flash_file);
    if ((offset + a_buf_size) <= PIFS_FLASH_SIZE_BYTE_ALL
        #if PIFS_FLASH_BLOCK_RESERVED_NUM
            && offset >= (PIFS_FLASH_BLOCK_RESERVED_NUM * PIFS_FLASH_BLOCK_SIZE_BYTE)
        #endif
            )
    {
        PIFS_ASSERT(fseek(flash_file, offset, SEEK_SET) == 0);
        /* Check if write is possible */
        read_count = fread(flash_page_buf, 1, a_buf_size, flash_file);
        if (read_count == a_buf_size)
        {
            ret = PIFS_SUCCESS;
            for (i = 0; i < a_buf_size; i++)
            {
                /* Check if we are trying to change bit 0 to 1 */
                /* Write can only set bit 1 to 0. */
                if (((buf8[i] ^ flash_page_buf[i]) & buf8[i]) != 0)
                {
                    /* Error: cannot write data */
                    printf("Original page:\r\n");
                    print_buffer(flash_page_buf, PIFS_FLASH_PAGE_SIZE_BYTE,
                                 a_block_address * PIFS_FLASH_BLOCK_SIZE_BYTE
                                 + a_page_address * PIFS_FLASH_PAGE_SIZE_BYTE);
                    printf("New page:\r\n");
                    print_buffer(buf8, PIFS_FLASH_PAGE_SIZE_BYTE,
                                 a_block_address * PIFS_FLASH_BLOCK_SIZE_BYTE
                                 + a_page_address * PIFS_FLASH_PAGE_SIZE_BYTE);
                    FLASH_ERROR_MSG("Cannot program 0x%02X to 0x%02X. %s offset: %i, 0x%02X\r\n",
                                    flash_page_buf[i], buf8[i],
                                    pifs_ba_pa2str(a_block_address, a_page_address),
                                    a_page_offset + i, a_page_offset + i);
                    ret = PIFS_ERROR_FLASH_WRITE;
                }
            }
            flash_stat[a_block_address][a_page_address].write_cntr++;
        }
        if (ret == PIFS_SUCCESS)
        {
            PIFS_ASSERT(fseek(flash_file, offset, SEEK_SET) == 0);
            write_count = fwrite(a_buf, 1, a_buf_size, flash_file);
            if (write_count != a_buf_size)
            {
                ret = PIFS_ERROR_FLASH_WRITE;
            }
        }
    }
    else
    {
        FLASH_ERROR_MSG("Trying to write to invalid flash address! %s offset: %i\r\n",
                        pifs_ba_pa2str(a_block_address, a_page_address), a_page_offset);
    }

    return ret;
}

pifs_status_t pifs_flash_erase(pifs_block_address_t a_block_address)
{
    pifs_status_t ret = PIFS_ERROR_FLASH_ERASE;
    long unsigned int offset = a_block_address * PIFS_FLASH_BLOCK_SIZE_BYTE;
    size_t write_count = 0;
    pifs_page_address_t i;
    
    PIFS_ASSERT(flash_file);
    if ((offset + PIFS_FLASH_BLOCK_SIZE_BYTE) <= PIFS_FLASH_SIZE_BYTE_ALL
        #if PIFS_FLASH_BLOCK_RESERVED_NUM
            && offset >= (PIFS_FLASH_BLOCK_RESERVED_NUM * PIFS_FLASH_BLOCK_SIZE_BYTE)
        #endif
            )
    {
        PIFS_ASSERT(fseek(flash_file, offset, SEEK_SET) == 0);
        ret = PIFS_SUCCESS;
        memset(flash_page_buf, 0xFFu, sizeof(flash_page_buf));
        for (i = 0; i < PIFS_FLASH_PAGE_PER_BLOCK; i++)
        {
            write_count = fwrite(flash_page_buf, 1, sizeof(flash_page_buf), flash_file);
            if (write_count != sizeof(flash_page_buf))
            {
                ret = PIFS_ERROR_FLASH_ERASE;
                break;
            }
            flash_stat[a_block_address][i].erase_cntr++;
        }
    }
    else
    {
        FLASH_ERROR_MSG("Trying to erase invalid flash address! BA%i\r\n",
                        a_block_address);
    }

    return ret;
}

void pifs_flash_print_stat(void)
{
    pifs_block_address_t ba;
    pifs_page_address_t  pa;
    flash_page_stat_t    min = { SIZE_MAX, SIZE_MAX, SIZE_MAX };
    flash_page_stat_t    max = { 0, 0, 0 };
    float                erase_sum = 0;
    float                read_sum = 0;
    float                write_sum = 0;
    float                avg_erase = 0;
    float                avg_read = 0;
    float                avg_write = 0;

    printf("Flash statistics\r\n");
    printf("================\r\n");
    printf("\r\n");

    printf("Block | Erase count\r\n");
    printf("------+------------\r\n");
    for (ba = 0; ba < PIFS_FLASH_BLOCK_NUM_ALL; ba++)
    {
        printf("%-5i | %i\r\n", ba, flash_stat[ba][0].erase_cntr);
        for (pa = 0; pa < PIFS_FLASH_PAGE_PER_BLOCK; pa++)
        {
            erase_sum += flash_stat[ba][pa].erase_cntr;
            read_sum += flash_stat[ba][pa].read_cntr;
            write_sum += flash_stat[ba][pa].write_cntr;
            if (flash_stat[ba][pa].erase_cntr < min.erase_cntr)
            {
                min.erase_cntr = flash_stat[ba][pa].erase_cntr;
            }
            if (flash_stat[ba][pa].read_cntr < min.read_cntr)
            {
                min.read_cntr = flash_stat[ba][pa].read_cntr;
            }
            if (flash_stat[ba][pa].write_cntr < min.write_cntr)
            {
                min.write_cntr = flash_stat[ba][pa].write_cntr;
            }
            if (flash_stat[ba][pa].erase_cntr > max.erase_cntr)
            {
                max.erase_cntr = flash_stat[ba][pa].erase_cntr;
            }
            if (flash_stat[ba][pa].read_cntr > max.read_cntr)
            {
                max.read_cntr = flash_stat[ba][pa].read_cntr;
            }
            if (flash_stat[ba][pa].write_cntr > max.write_cntr)
            {
                max.write_cntr = flash_stat[ba][pa].write_cntr;
            }
        }
    }

    avg_erase = erase_sum / PIFS_FLASH_PAGE_NUM_ALL;
    avg_read = read_sum / PIFS_FLASH_PAGE_NUM_ALL;
    avg_write = write_sum / PIFS_FLASH_PAGE_NUM_ALL;

    printf("\r\n");
    printf("                    Min |    Average |        Max\r\n");
    printf("------------------------+------------+-----------\r\n");
    printf("Erase count  %10i | %10.1f | %10i\r\n", min.erase_cntr, avg_erase, max.erase_cntr);
    printf("Read count   %10i | %10.1f | %10i\r\n", min.read_cntr, avg_read, max.read_cntr);
    printf("Write count  %10i | %10.1f | %10i\r\n", min.write_cntr, avg_write, max.write_cntr);
    printf("\r\n");
}
