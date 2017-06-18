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

#define FLASH_DEBUG     1

#if FLASH_DEBUG
#define FLASH_ERROR_MSG(...)    do { \
        fflush(stdout); \
        fprintf(stderr, "%s ERROR: ", __FUNCTION__); \
        fprintf(stderr, __VA_ARGS__); \
        fflush(stderr); \
    } while (0);
#else
#define FLASH_ERROR_MSG(...)
#endif

#define FLASH_EMU_FILENAME      "flash.bin" /**< Name of memory file */

static FILE * flash_file = NULL;
static uint8_t flash_page_buf[PIFS_FLASH_PAGE_SIZE_BYTE] = { 0 };

pifs_status_t pifs_flash_init(void)
{
    pifs_status_t ret = PIFS_ERROR_FLASH_INIT;
    pifs_block_address_t ba;

    flash_file = fopen(FLASH_EMU_FILENAME, "rb+");
    if (flash_file)
    {
        /* Memory file already exists */
        ret = PIFS_SUCCESS;
    }
    else
    {
        /* Memory file has not created yet */
        flash_file = fopen(FLASH_EMU_FILENAME, "wb+");
        if (flash_file)
        {
            /* Erase whole memory */
            for (ba = PIFS_FLASH_BLOCK_RESERVED_NUM; ba < PIFS_FLASH_BLOCK_NUM_ALL; ba++)
            {
                ret = pifs_flash_erase(ba);
            }

            ret = PIFS_SUCCESS;
        }
    }

    return ret;
}

pifs_status_t pifs_flash_delete(void)
{
    pifs_status_t ret = PIFS_ERROR;

    if (flash_file)
    {
        if (!fclose(flash_file))
        {
            ret = PIFS_SUCCESS;
        }
        flash_file = NULL;
    }

    return ret;
}

pifs_status_t pifs_flash_read(pifs_block_address_t a_block_address, pifs_page_address_t a_page_address, pifs_page_offset_t a_page_offset, void * const a_buf, size_t a_buf_size)
{
    pifs_status_t ret = PIFS_ERROR;
    long int offset = a_block_address * PIFS_FLASH_BLOCK_SIZE_BYTE 
        + a_page_address * PIFS_FLASH_PAGE_SIZE_BYTE
        + a_page_offset;
    size_t read_count = 0;

    PIFS_ASSERT(flash_file);
    if (offset >= (PIFS_FLASH_BLOCK_RESERVED_NUM * PIFS_FLASH_BLOCK_SIZE_BYTE)
            && (offset + a_buf_size) <= PIFS_FLASH_SIZE_BYTE_ALL)
    {
        PIFS_ASSERT(fseek(flash_file, offset, SEEK_SET) == 0);
        read_count = fread(a_buf, 1, a_buf_size, flash_file);
        if (read_count == a_buf_size)
        {
            ret = PIFS_SUCCESS;
        }
    }
    else
    {
        FLASH_ERROR_MSG("Trying to read from invalid flash adress! BA%i/PA%i/OFS%i\r\n",
                        a_block_address, a_page_address, a_page_offset);
    }

    return ret;
}

pifs_status_t pifs_flash_write(pifs_block_address_t a_block_address, pifs_page_address_t a_page_address, pifs_page_address_t a_page_offset, const void * const a_buf, size_t a_buf_size)
{
    pifs_status_t ret = PIFS_ERROR;
    long int offset = a_block_address * PIFS_FLASH_BLOCK_SIZE_BYTE 
        + a_page_address * PIFS_FLASH_PAGE_SIZE_BYTE
        + a_page_offset;
    size_t write_count = 0;
    size_t read_count = 0;
    pifs_page_offset_t i;
    uint8_t * buf8 = (uint8_t*) a_buf;
    
    PIFS_ASSERT(flash_file);
    if (offset >= (PIFS_FLASH_BLOCK_RESERVED_NUM * PIFS_FLASH_BLOCK_SIZE_BYTE)
            && (offset + a_buf_size) <= PIFS_FLASH_SIZE_BYTE_ALL)
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
                    FLASH_ERROR_MSG("Cannot program 0x%02X to 0x%02X. BA%i/PA%i/OFS%i\r\n",
                                    buf8[i], flash_page_buf[i],
                                    a_block_address, a_page_address, a_page_offset + i );
                    ret = PIFS_ERROR;
                }
            }
        }
        if (ret == PIFS_SUCCESS)
        {
            PIFS_ASSERT(fseek(flash_file, offset, SEEK_SET) == 0);
            write_count = fwrite(a_buf, 1, a_buf_size, flash_file);
            if (write_count != a_buf_size)
            {
                ret = PIFS_ERROR;
            }
        }
    }
    else
    {
        FLASH_ERROR_MSG("Trying to write to invalid flash address! BA%i/PA%i/OFS%i\r\n",
                        a_block_address, a_page_address, a_page_offset);
    }

    return ret;
}

pifs_status_t pifs_flash_erase(pifs_block_address_t a_block_address)
{
    pifs_status_t ret = PIFS_ERROR;
    long int offset = a_block_address * PIFS_FLASH_BLOCK_SIZE_BYTE;
    size_t write_count = 0;
    pifs_page_address_t i;
    
    PIFS_ASSERT(flash_file);
    if (offset >= (PIFS_FLASH_BLOCK_RESERVED_NUM * PIFS_FLASH_BLOCK_SIZE_BYTE)
            && (offset + PIFS_FLASH_BLOCK_SIZE_BYTE) <= PIFS_FLASH_SIZE_BYTE_ALL)
    {
        PIFS_ASSERT(fseek(flash_file, offset, SEEK_SET) == 0);
        ret = PIFS_SUCCESS;
        memset(flash_page_buf, 0xFFu, sizeof(flash_page_buf));
        for (i = 0; i < PIFS_FLASH_PAGE_PER_BLOCK; i++)
        {
            write_count = fwrite(flash_page_buf, 1, sizeof(flash_page_buf), flash_file);
            if (write_count != sizeof(flash_page_buf))
            {
                ret = PIFS_ERROR;
                break;
            }
        }
    }
    else
    {
        FLASH_ERROR_MSG("Trying to erase invalid flash address! BA%i\r\n",
                        a_block_address);
    }

    return ret;
}

