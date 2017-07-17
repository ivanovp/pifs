/**
 * @file        pifs_helper.c
 * @brief       Pi file system: utility functions
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-07-06 19:12:58 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api_pifs.h"
#include "flash.h"
#include "flash_config.h"
#include "pifs.h"
#include "pifs_helper.h"
#include "buffer.h"

#define PIFS_DEBUG_LEVEL 3
#include "pifs_debug.h"

#if PIFS_DEBUG_LEVEL >= 1
/**
 * @brief pifs_address2str Convert address to human readable string.
 *
 * @param[in] a_address Address to convert.
 * @return The created string.
 */
char * pifs_address2str(pifs_address_t * a_address)
{
    static char str[32];

    snprintf(str, sizeof(str), "BA%i/PA%i @0x%X", a_address->block_address, a_address->page_address,
           a_address->block_address * PIFS_FLASH_BLOCK_SIZE_BYTE
           + a_address->page_address * PIFS_FLASH_PAGE_SIZE_BYTE);

    return str;
}

/**
 * @brief pifs_ba_pa2str Convert adress to human readable string.
 *
 * @param[in] a_block_address Block address.
 * @param[in] a_page_address Page address.
 * @return The created string.
 */
char * pifs_ba_pa2str(pifs_block_address_t a_block_address, pifs_page_address_t a_page_address)
{
    static char str[32];

    snprintf(str, sizeof(str), "BA%i/PA%i @0x%X", a_block_address, a_page_address,
           a_block_address * PIFS_FLASH_BLOCK_SIZE_BYTE
           + a_page_address * PIFS_FLASH_PAGE_SIZE_BYTE);

    return str;
}

/**
 * @brief pifs_byte2bin_str Convert a byte to binary string.
 *
 * @param[in] byte  Byte to convert.
 * @return Binary string.
 */
char * pifs_byte2bin_str(uint8_t byte)
{
    uint8_t i;
    static char s[12];

    s[0] = 0;
    for (i = 0; i < PIFS_BYTE_BITS; i++)
    {
        strncat(s, (byte & 0x80) ? "1" : "0", sizeof(s));
        byte <<= 1;
    }
    return s;
}
#endif

/**
 * @brief pifs_print_cache Print content of page buffer.
 */
void pifs_print_cache(void)
{
#if PIFS_DEBUG_LEVEL >= 3
    PIFS_NOTICE_MSG("Cache page buffer:\r\n");
    print_buffer(pifs.cache_page_buf, sizeof(pifs.cache_page_buf),
                 pifs.cache_page_buf_address.block_address * PIFS_FLASH_BLOCK_SIZE_BYTE
                 + pifs.cache_page_buf_address.page_address * PIFS_FLASH_PAGE_SIZE_BYTE);
#endif
}

bool_t pifs_is_address_valid(pifs_address_t * a_address)
{
    bool_t valid = (a_address->block_address < PIFS_FLASH_BLOCK_NUM_ALL)
            && (a_address->page_address < PIFS_FLASH_PAGE_PER_BLOCK);

    return valid;
}

/**
 * @brief pifs_is_block_type Checks if the given block address is block type.
 * @param[in] a_block_address Block address to check.
 * @param[in] a_block_type    Block type.
 * @param[in] a_header        Pointer to file system's header.
 * @return TRUE: If block address is equal to block type.
 */
bool_t pifs_is_block_type(pifs_block_address_t a_block_address,
                          pifs_block_type_t a_block_type,
                          pifs_header_t * a_header)
{
    pifs_size_t          i = 0;
    bool_t               is_block_type = TRUE;

    if (a_block_type == PIFS_BLOCK_TYPE_ANY)
    {
        is_block_type = TRUE;
    }
    else
    {
        is_block_type = (a_block_type == PIFS_BLOCK_TYPE_DATA);
#if PIFS_MANAGEMENT_BLOCKS > 1
        for (i = 0; i < PIFS_MANAGEMENT_BLOCKS; i++)
#endif
        {
            if (a_header->management_blocks[i] == a_block_address)
            {
                is_block_type = (a_block_type == PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT);
            }
            if (a_header->next_management_blocks[i] == a_block_address)
            {
                is_block_type = (a_block_type == PIFS_BLOCK_TYPE_SECONDARY_MANAGEMENT);
            }
        }
#if PIFS_FLASH_BLOCK_RESERVED_NUM
        if (a_block_address < PIFS_FLASH_BLOCK_RESERVED_NUM)
        {
            is_block_type = (a_block_type == PIFS_BLOCK_TYPE_RESERVED);
        }
#endif
    }

    return is_block_type;
}

/**
 * @brief pifs_is_buffer_erased Check if buffer is erased or contains
 * programmed bytes.
 *
 * @param[in] a_buf[in]         Pointer to buffer.
 * @param[in] a_buf_size[in]    Size of buffer.
 * @return TRUE: if buffer is erased.
 * FALSE: if buffer contains at least one programmed bit.
 */
bool_t pifs_is_buffer_erased(const void * a_buf, pifs_size_t a_buf_size)
{
    uint8_t   * buf = (uint8_t*) a_buf;
    pifs_size_t i;
    bool_t      ret = TRUE;

    for (i = 0; i < a_buf_size && ret; i++)
    {
        if (buf[i] != PIFS_FLASH_ERASED_BYTE_VALUE)
        {
            ret = FALSE;
        }
    }

    return ret;
}

/**
 * @brief pifs_is_page_erased Checks if the given block address is block type.
 * @param[in] a_block_address Block address to check.
 * @param[in] a_page_address  Page address to check.
 * @return TRUE: If page is erased.
 */
bool_t pifs_is_page_erased(pifs_block_address_t a_block_address,
                           pifs_page_address_t a_page_address)
{
    pifs_status_t status;
    bool_t is_erased = FALSE;

    status = pifs_read(a_block_address, a_page_address, 0, NULL, 0);
    if (status == PIFS_SUCCESS)
    {
        is_erased = pifs_is_buffer_erased(pifs.cache_page_buf, PIFS_FLASH_PAGE_SIZE_BYTE);
    }
    return is_erased;
}

/**
 * @brief pifs_is_buffer_programmable Check if buffer is programmable or erase
 * is needed.
 *
 * @param[in] a_orig_buf[in]    Pointer to original buffer.
 * @param[in] a_new_buf[in]     Pointer to new buffer.
 * @param[in] a_buf_size[in]    Size of buffer.
 * @return TRUE: if buffer is programmable.
 * FALSE: if buffer contains at least one programmed bit.
 */
bool_t pifs_is_buffer_programmable(const void * a_orig_buf, const void * a_new_buf, pifs_size_t a_buf_size)
{
    uint8_t   * orig_buf = (uint8_t*) a_orig_buf;
    uint8_t   * new_buf = (uint8_t*) a_new_buf;
    pifs_size_t i;
    bool_t      ret = TRUE;

    for (i = 0; i < a_buf_size && ret; i++)
    {
        /* Checking if only bit programming is needed.
         * When erased value if 0xFF, bit programming generates falling edge
         * on data.
         */
#if PIFS_FLASH_ERASED_BYTE_VALUE == 0xFF
        if ((orig_buf[i] ^ new_buf[i]) & new_buf[i])  /* Detecting rising edge */
#else
        if ((orig_buf[i] ^ new_buf[i]) & orig_buf[i]) /* Detecting falling edge */
#endif
        {
            /* Bit erasing would be necessary to store the data,
             * therefore it cannot be programmed.
             */
            ret = FALSE;
            PIFS_DEBUG_MSG("Not programmable! orig: 0x%x new: 0x%X\r\n",
                           orig_buf[i], new_buf[i]);
        }
    }

    return ret;
}

/**
 * @brief pifs_is_buffer_programmed Check if buffer's all bits are programmed.
 *
 * @param[in] a_buf[in]         Pointer to buffer.
 * @param[in] a_buf_size[in]    Size of buffer.
 * @return TRUE: if buffer is programmed.
 * FALSE: if buffer contains at least one programmed bit.
 */
bool_t pifs_is_buffer_programmed(const void * a_buf, pifs_size_t a_buf_size)
{
    uint8_t   * buf = (uint8_t*) a_buf;
    pifs_size_t i;
    bool_t      ret = TRUE;

    for (i = 0; i < a_buf_size && ret; i++)
    {
        if (buf[i] != PIFS_FLASH_PROGRAMMED_BYTE_VALUE)
        {
            ret = FALSE;
        }
    }

    return ret;
}


/**
 * @brief pifs_parse_open_mode Parse string of open mode.
 *
 * @param a_file[in]    Pointer to file's internal structure.
 * @param a_modes[in]   String of mode.
 */
void pifs_parse_open_mode(pifs_file_t * a_file, const pifs_char_t * a_modes)
{
    uint8_t i;

    /* Reset mode */
    a_file->mode_create_new_file = FALSE;
    a_file->mode_read = FALSE;
    a_file->mode_write = FALSE;
    a_file->mode_append = FALSE;
    a_file->mode_file_shall_exist = FALSE;
    for (i = 0; a_modes[i] && i < 4; i++)
    {
        switch (a_modes[i])
        {
            case 'r':
                /* Read */
                a_file->mode_read = TRUE;
                a_file->mode_file_shall_exist = TRUE;
                break;
            case 'w':
                /* Write */
                a_file->mode_write = TRUE;
                a_file->mode_create_new_file = TRUE;
                break;
            case '+':
                if (a_file->mode_write)
                {
                    /* mode "w+" */
                    a_file->mode_read = TRUE;
                    a_file->mode_create_new_file = TRUE;
                }
                else if (a_file->mode_read)
                {
                    /* mode "r+" */
                    a_file->mode_write = TRUE;
                    a_file->mode_file_shall_exist = TRUE;
                }
                else if (a_file->mode_append)
                {
                    /* mode "a+" */
                    a_file->mode_read = TRUE;
                }
                break;
            case 'a':
                a_file->mode_append = TRUE;
                break;
            case 'b':
                /* Binary, all operations are binary! */
                break;
            default:
                a_file->status = PIFS_ERROR_INVALID_OPEN_MODE;
                PIFS_ERROR_MSG("Invalid open mode '%s'\r\n", a_modes);
                break;
        }
    }
    PIFS_DEBUG_MSG("create_new_file: %i\r\n", a_file->mode_create_new_file);
    PIFS_DEBUG_MSG("read: %i\r\n", a_file->mode_read);
    PIFS_DEBUG_MSG("write: %i\r\n", a_file->mode_write);
    PIFS_DEBUG_MSG("append: %i\r\n", a_file->mode_append);
    PIFS_DEBUG_MSG("file_shall_exist: %i\r\n", a_file->mode_file_shall_exist);
}

/**
 * @brief pifs_inc_address Increment address.
 *
 * @param[in] a_address Pointer to address to increment.
 * @return PIFS_SUCCESS if successfully incremented.
 * PIFS_ERROR_INTERNAL_RANGE if end of flash is reached.
 */
pifs_status_t pifs_inc_address(pifs_address_t * a_address)
{
    pifs_status_t ret = PIFS_SUCCESS;

    a_address->page_address++;
    if (a_address->page_address == PIFS_FLASH_PAGE_PER_BLOCK)
    {
        a_address->page_address = 0;
        a_address->block_address++;
        if (a_address->block_address >= PIFS_FLASH_BLOCK_NUM_ALL)
        {
            PIFS_ERROR_MSG("End of flash: %s\r\n", pifs_address2str(a_address));
            ret = PIFS_ERROR_INTERNAL_RANGE;
        }
    }
    PIFS_DEBUG_MSG("%s\r\n", pifs_address2str(a_address));

    return ret;
}

/**
 * @brief pifs_inc_ba_pa Increment address.
 * @param[in] a_block_address  Pointer to block address to increment.
 * @param[in] a_page_address   Pointer to page address to increment.
 *
 * @return PIFS_SUCCESS if successfully incremented.
 * PIFS_ERROR_INTERNAL_RANGE if end of flash is reached.
 */
pifs_status_t pifs_inc_ba_pa(pifs_block_address_t * a_block_address,
                             pifs_page_address_t * a_page_address)
{
    pifs_status_t ret = PIFS_SUCCESS;

    (*a_page_address)++;
    if (*a_page_address == PIFS_FLASH_PAGE_PER_BLOCK)
    {
        *a_page_address = 0;
        (*a_block_address)++;
        if (*a_block_address >= PIFS_FLASH_BLOCK_NUM_ALL)
        {
            PIFS_ERROR_MSG("End of flash: %s\r\n",
                           pifs_ba_pa2str(*a_block_address, *a_page_address));
            ret = PIFS_ERROR_INTERNAL_RANGE;
        }
    }
    PIFS_DEBUG_MSG("%s\r\n", pifs_ba_pa2str(*a_block_address, *a_page_address));

    return ret;
}

/**
 * @brief pifs_check_filename Check if file name is valid.
 * @param[in] a_filename    Pointer to the file name to be checked.
 * @return PIFS_SUCCESS if filename does not contain invalid characters.
 */
pifs_status_t pifs_check_filename(const pifs_char_t * a_filename)
{
    pifs_status_t     ret = PIFS_ERROR_NOT_INITIALIZED;
    pifs_size_t       i;
    pifs_size_t       len = strlen(a_filename);
    const pifs_char_t invalid_chars[] = "\"'*,/:;<=>?[]|";

    if (len > 0)
    {
        ret = PIFS_SUCCESS;
        for (i = 0; i < len && ret == PIFS_SUCCESS; i++)
        {
            if (strchr(invalid_chars, a_filename[i]) != NULL)
            {
                ret = PIFS_ERROR_INVALID_FILE_NAME;
            }
        }
    }
    else
    {
        ret = PIFS_ERROR_INVALID_FILE_NAME;
    }

    return ret;
}

/**
 * @brief pifs_get_file Find an unused file structure (file handle).
 *
 * @param[out] a_file Pointer to pointer of file structure to fill.
 * @return PIFS_SUCCESS if unused file structure found.
 */
pifs_status_t pifs_get_file(pifs_file_t * * a_file)
{
    pifs_status_t ret = PIFS_ERROR_NO_MORE_RESOURCE;
    pifs_size_t   i;
    pifs_file_t * file = NULL;

    for (i = 0; i < PIFS_OPEN_FILE_NUM_MAX && ret != PIFS_SUCCESS; i++)
    {
        if (!pifs.file[i].is_used)
        {
            file = &pifs.file[i];
            file->is_used = TRUE;
            ret = PIFS_SUCCESS;
        }
    }
    *a_file = file;

    return ret;
}
