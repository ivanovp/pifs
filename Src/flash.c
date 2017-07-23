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
#include <math.h>

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
#define FLASH_STAT_READ_CNTR    0
#define FLASH_STAT_WRITE_CNTR   1
#define FLASH_STAT_ERASE_CNTR   2
#define FLASH_STAT_CNTR_NUM     3   /**< Number of counters */

static FILE * flash_file = NULL;
static FILE * stat_file = NULL;
static uint8_t flash_page_buf[PIFS_FLASH_PAGE_SIZE_BYTE] = { 0 };
static size_t flash_stat[FLASH_STAT_CNTR_NUM][PIFS_FLASH_BLOCK_NUM_ALL][PIFS_FLASH_PAGE_PER_BLOCK] = { { { 0  } } };
static size_t flash_stat_temp[PIFS_FLASH_BLOCK_NUM_ALL * PIFS_FLASH_PAGE_PER_BLOCK] = { 0 };

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
        flash_stat[FLASH_STAT_READ_CNTR][a_block_address][a_page_address]++;
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
            flash_stat[FLASH_STAT_WRITE_CNTR][a_block_address][a_page_address]++;
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
            flash_stat[FLASH_STAT_ERASE_CNTR][a_block_address][i]++;
        }
    }
    else
    {
        FLASH_ERROR_MSG("Trying to erase invalid flash address! BA%i\r\n",
                        a_block_address);
    }

    return ret;
}

void pifs_flash_sort(size_t * a_array, size_t a_array_size)
{
    size_t i;
    size_t temp;
    bool_t exchanged;

    do
    {
        exchanged = FALSE;
        for (i = 0; i < a_array_size - 1; i++)
        {
            if (a_array[i] > a_array[i + 1])
            {
                temp = a_array[i + 1];
                a_array[i + 1] = a_array[i];
                a_array[i] = temp;
                exchanged = TRUE;
            }
        }
    } while (exchanged);

#if 0
    for (i = 0; i < a_array_size - 1; i++)
    {
        printf("%i\r\n", a_array[i]);
    }
    printf("----------\r\n");
#endif
}

void pifs_flash_print_stat(void)
{
    pifs_block_address_t ba;
    pifs_page_address_t  pa;
    size_t               i;
    size_t               min[FLASH_STAT_CNTR_NUM] = { SIZE_MAX, SIZE_MAX, SIZE_MAX };
    size_t               max[FLASH_STAT_CNTR_NUM] = { 0 };
    float                sum[FLASH_STAT_CNTR_NUM] = { 0 };
    float                avg[FLASH_STAT_CNTR_NUM] = { 0 };
    float                median[FLASH_STAT_CNTR_NUM] = { 0 };
    float                std_dev[FLASH_STAT_CNTR_NUM] = { 0 };

    /* Calculate median of counters then standard devitation */
    for (i = 0; i < FLASH_STAT_CNTR_NUM; i++)
    {
        const size_t flash_stat_array_size = sizeof(flash_stat_temp) / sizeof(flash_stat_temp[0]);
        float summa;
        for (ba = PIFS_FLASH_BLOCK_RESERVED_NUM; ba < PIFS_FLASH_BLOCK_NUM_ALL; ba++)
        {
            for (pa = 0; pa < PIFS_FLASH_PAGE_PER_BLOCK; pa++)
            {
                flash_stat_temp[ba * PIFS_FLASH_PAGE_PER_BLOCK + pa] = flash_stat[i][ba][pa];
            }
        }
        pifs_flash_sort(flash_stat_temp, flash_stat_array_size);
        summa = flash_stat_temp[flash_stat_array_size / 2 - 1]
                + flash_stat_temp[flash_stat_array_size / 2];
        median[i] = summa / 2.0;

        /* Calculate sum */
        for (ba = PIFS_FLASH_BLOCK_RESERVED_NUM; ba < PIFS_FLASH_BLOCK_NUM_ALL; ba++)
        {
            for (pa = 0; pa < PIFS_FLASH_PAGE_PER_BLOCK; pa++)
            {
                std_dev[i] += pow(flash_stat[i][ba][pa] - median[i], 2);
            }
        }
        /* Calculate standard deviation */
        std_dev[i] = sqrt(std_dev[i] / (PIFS_FLASH_BLOCK_NUM_FS * PIFS_FLASH_PAGE_PER_BLOCK));
    }

    printf("Flash statistics\r\n");
    printf("================\r\n");
    printf("\r\n");

    printf("Block | Erase count\r\n");
    printf("------+------------\r\n");
    for (ba = 0; ba < PIFS_FLASH_BLOCK_NUM_ALL; ba++)
    {
        printf("%-5i | %i\r\n", ba, flash_stat[FLASH_STAT_ERASE_CNTR][ba][0]);
        for (pa = 0; pa < PIFS_FLASH_PAGE_PER_BLOCK; pa++)
        {
            for (i = 0; i < FLASH_STAT_CNTR_NUM; i++)
            {
                sum[i] += flash_stat[i][ba][pa];
                if (flash_stat[i][ba][pa] < min[i])
                {
                    min[i] = flash_stat[i][ba][pa];
                }
                if (flash_stat[i][ba][pa] > max[i])
                {
                    max[i] = flash_stat[i][ba][pa];
                }
            }
        }
    }

    for (i = 0; i < FLASH_STAT_CNTR_NUM; i++)
    {
        avg[i] = sum[i] / PIFS_FLASH_PAGE_NUM_ALL;
    }

    printf("\r\n");
    printf("Count |      Min |  Average |      Max |  Std dev |   Median\r\n");
    printf("------+----------+----------+----------+----------+---------\r\n");
    printf("Erase | %8i | %8.1f | %8i | %8.1f | %8.1f\r\n", min[FLASH_STAT_ERASE_CNTR],
           avg[FLASH_STAT_ERASE_CNTR], max[FLASH_STAT_ERASE_CNTR],
           std_dev[FLASH_STAT_ERASE_CNTR], median[FLASH_STAT_ERASE_CNTR]);
    printf("Read  | %8i | %8.1f | %8i | %8.1f | %8.1f\r\n", min[FLASH_STAT_READ_CNTR],
           avg[FLASH_STAT_READ_CNTR], max[FLASH_STAT_READ_CNTR],
           std_dev[FLASH_STAT_WRITE_CNTR], median[FLASH_STAT_WRITE_CNTR]);
    printf("Write | %8i | %8.1f | %8i | %8.1f | %8.1f\r\n", min[FLASH_STAT_WRITE_CNTR],
           avg[FLASH_STAT_WRITE_CNTR], max[FLASH_STAT_WRITE_CNTR],
           std_dev[FLASH_STAT_WRITE_CNTR], median[FLASH_STAT_WRITE_CNTR]);
    printf("\r\n");
}
