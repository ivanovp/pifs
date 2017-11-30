/**
 * @file        pifs_helper.c
 * @brief       Pi file system: utility functions
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-11-30 17:43:12 ivanovp {Time-stamp}
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
#include <string.h>

#include "api_pifs.h"
#include "flash.h"
#include "flash_config.h"
#include "pifs.h"
#include "pifs_fsbm.h"
#include "pifs_map.h"
#include "pifs_helper.h"
#include "pifs_crc8.h"
#include "buffer.h"

#define PIFS_DEBUG_LEVEL 2
#include "pifs_debug.h"

/**
 * @brief pifs_calc_checksum Calculate checksum or CRC of a buffer.
 *
 * @param[in] a_buf         Pointer to the buffer.
 * @param[in] a_buf_size    Length of buffer.
 * @return The calculated checksum.
 */
pifs_checksum_t pifs_calc_checksum(void * a_buf, size_t a_buf_size)
{
#if PIFS_ENABLE_CRC
    pifs_checksum_t checksum = (pifs_checksum_t) crc_init();

    checksum = crc_update(checksum, a_buf, a_buf_size);
    checksum = crc_finalize(checksum);
#else
    uint8_t * ptr = (uint8_t*) a_buf;
    pifs_checksum_t checksum = (pifs_checksum_t) PIFS_CHECKSUM_ERASED;
    uint8_t cntr = a_buf_size;

    while (cntr--)
    {
        checksum = crc;
        ptr++;
    }
#endif

    if (checksum == PIFS_CHECKSUM_ERASED)
    {
        checksum++;
    }

    return checksum;
}


#if PIFS_DEBUG_LEVEL >= 1
/**
 * @brief pifs_address2str Convert logical address to human readable string.
 *
 * @param[in] a_address Address to convert.
 * @return The created string.
 */
char * pifs_address2str(pifs_address_t * a_address)
{
    static char str[32];

    snprintf(str, sizeof(str), "BA%i/PA%i @0x%X", a_address->block_address, a_address->page_address,
           a_address->block_address * PIFS_FLASH_BLOCK_SIZE_BYTE
           + a_address->page_address * PIFS_LOGICAL_PAGE_SIZE_BYTE);

    return str;
}

/**
 * @brief pifs_ba_pa2str Convert logical address to human readable string.
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
           + a_page_address * PIFS_LOGICAL_PAGE_SIZE_BYTE);

    return str;
}

/**
 * @brief pifs_address2str Convert flash address to human readable string.
 *
 * @param[in] a_address Address to convert.
 * @return The created string.
 */
char * pifs_flash_address2str(pifs_address_t * a_address)
{
    static char str[32];

    snprintf(str, sizeof(str), "BA%i/PA%i @0x%X", a_address->block_address, a_address->page_address,
           a_address->block_address * PIFS_FLASH_BLOCK_SIZE_BYTE
           + a_address->page_address * PIFS_FLASH_PAGE_SIZE_BYTE);

    return str;
}

/**
 * @brief pifs_ba_pa2str Convert flash address to human readable string.
 *
 * @param[in] a_block_address Block address.
 * @param[in] a_page_address Page address.
 * @return The created string.
 */
char * pifs_flash_ba_pa2str(pifs_block_address_t a_block_address, pifs_page_address_t a_page_address)
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
                 + pifs.cache_page_buf_address.page_address * PIFS_LOGICAL_PAGE_SIZE_BYTE);
#endif
}

/**
 * Convert boolean to human readable string.
 *
 * @param[in] expression    True or false.
 *
 * @return "Yes" if true. Otherwise "No".
 */
char * pifs_yes_no(bool_t expression)
{
    if (expression)
    {
        return "Yes";
    }
    else
    {
        return "No";
    }
}

/**
 * @brief pifs_print_page_info Print information about logical page(s).
 *
 * @param[in] a_addr Address of page.
 * @param[in] a_cntr Number of pages.
 */
void pifs_print_page_info(size_t a_addr, pifs_size_t a_cntr)
{
    pifs_block_address_t ba;
    pifs_page_address_t  pa;

    pa = (a_addr / PIFS_LOGICAL_PAGE_SIZE_BYTE) % PIFS_LOGICAL_PAGE_PER_BLOCK;
    ba = (a_addr / PIFS_LOGICAL_PAGE_SIZE_BYTE) / PIFS_LOGICAL_PAGE_PER_BLOCK;
    printf("Page                    Free    TBR     Erased\r\n");
    do
    {
        printf("%-24s", pifs_ba_pa2str(ba, pa));
        if (ba < PIFS_FLASH_BLOCK_NUM_ALL)
        {
            printf("%-8s", pifs_yes_no(pifs_is_page_free(ba, pa)));
            printf("%-8s", pifs_yes_no(pifs_is_page_to_be_released(ba, pa)));
            printf("%-8s\r\n", pifs_yes_no(pifs_is_page_erased(ba, pa)));
        }
        else
        {
            printf("ERROR: Invalid address!\r\n");
            break;
        }
        if (a_cntr > 1)
        {
            pifs_inc_ba_pa(&ba, &pa);
        }
    } while (--a_cntr);
}

/**
 * @brief pifs_print_map_page Read and print map page's data.
 *
 * @param[in] a_block_address Block address of page.
 * @param[in] a_page_address  Page address of page.
 * @param[in] a_count         Number of pages.
 * @return PIFS_SUCCESS if all pages were read successfully.
 */
pifs_status_t pifs_print_map_page(pifs_block_address_t a_block_address,
                                  pifs_page_address_t a_page_address,
                                  pifs_size_t a_count)
{
    pifs_map_header_t   map_header;
    pifs_map_entry_t    map_entry;
    pifs_status_t       ret;
    size_t              i;
    bool_t              end = FALSE;

    printf("Map page %s\r\n\r\n", pifs_ba_pa2str(a_block_address, a_page_address));

    map_header.next_map_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
    map_header.next_map_address.page_address = PIFS_PAGE_ADDRESS_INVALID;

    do
    {
        if (map_header.next_map_address.block_address < PIFS_BLOCK_ADDRESS_INVALID
                && map_header.next_map_address.page_address < PIFS_PAGE_ADDRESS_INVALID)
        {
            a_block_address = map_header.next_map_address.block_address;
            a_page_address = map_header.next_map_address.page_address;
        }
        ret = pifs_read(a_block_address, a_page_address, 0, &map_header, PIFS_MAP_HEADER_SIZE_BYTE);
        if (ret == PIFS_SUCCESS)
        {
            printf("Previous map: %s\r\n", pifs_address2str(&map_header.prev_map_address));
            printf("Next map:     %s\r\n\r\n", pifs_address2str(&map_header.next_map_address));

            for (i = 0; i < PIFS_MAP_ENTRY_PER_PAGE && !end && ret == PIFS_SUCCESS; i++)
            {
                /* Go through all map entries in the page */
                ret = pifs_read(a_block_address, a_page_address, PIFS_MAP_HEADER_SIZE_BYTE + i * PIFS_MAP_ENTRY_SIZE_BYTE,
                                &map_entry, PIFS_MAP_ENTRY_SIZE_BYTE);
                if (ret == PIFS_SUCCESS)
                {
                    if (!pifs_is_buffer_erased(&map_entry, PIFS_MAP_ENTRY_SIZE_BYTE))
                    {
                        printf("%s  page count: %i\r\n",
                               pifs_address2str(&map_entry.address),
                               map_entry.page_count);
                    }
                    else
                    {
                        /* Map entry is unused */
                        end = TRUE;
                    }
                }
            }
        }
        else
        {
            printf("ERROR: Cannot read page, error code: %i\r\n", ret);
        }
    } while (a_count--
             && map_header.next_map_address.block_address < PIFS_BLOCK_ADDRESS_INVALID
             && map_header.next_map_address.page_address < PIFS_PAGE_ADDRESS_INVALID);

    return ret;
}

/**
 * @brief pifs_is_address_valid Check if address is valid.
 *
 * @param[in] a_address Pointer to address to check.
 * @return TRUE: address is valid. FALSE: address is invalid.
 */
bool_t pifs_is_address_valid(pifs_address_t * a_address)
{
    bool_t valid = (a_address->block_address < PIFS_FLASH_BLOCK_NUM_ALL)
            && (a_address->page_address < PIFS_LOGICAL_PAGE_PER_BLOCK);

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
    bool_t is_block_type = FALSE;

#if PIFS_MANAGEMENT_BLOCK_NUM > 1
    if (a_block_address >= a_header->management_block_address
            && a_block_address < (a_header->management_block_address + PIFS_MANAGEMENT_BLOCK_NUM))
#else
    if (a_header->management_block_address == a_block_address)
#endif
    {
        is_block_type |= (a_block_type & PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT);
        if (a_block_type & PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT)
        {
            //PIFS_WARNING_MSG("Primary management block: %i\r\n", a_block_address);
        }
    }
    else
#if PIFS_MANAGEMENT_BLOCK_NUM > 1
    if (a_block_address >= a_header->next_management_block_address
            && a_block_address < (a_header->next_management_block_address + PIFS_MANAGEMENT_BLOCK_NUM))
#else
    if (a_header->next_management_block_address == a_block_address)
#endif
    {
        is_block_type |= (a_block_type & PIFS_BLOCK_TYPE_SECONDARY_MANAGEMENT);
        if (a_block_type & PIFS_BLOCK_TYPE_SECONDARY_MANAGEMENT)
        {
            //PIFS_WARNING_MSG("Secondary management block: %i\r\n", a_block_address);
        }
    }
    else
#if PIFS_FLASH_BLOCK_RESERVED_NUM
    if (a_block_address < PIFS_FLASH_BLOCK_RESERVED_NUM)
    {
        is_block_type |= (a_block_type & PIFS_BLOCK_TYPE_RESERVED);
    }
    else
#endif
    {
        is_block_type |= (a_block_type & PIFS_BLOCK_TYPE_DATA);
        if (a_block_type & PIFS_BLOCK_TYPE_SECONDARY_MANAGEMENT)
        {
            //PIFS_WARNING_MSG("Data block: %i\r\n", a_block_address);
        }
    }

    return is_block_type;
}

/**
 * @brief pifs_is_buffer_erased Check if buffer is erased or contains
 * programmed bytes.
 *
 * @param[in] a_buf         Pointer to buffer.
 * @param[in] a_buf_size    Size of buffer.
 * @return TRUE: if buffer is erased.
 * FALSE: if buffer contains at least one programmed bit.
 */
bool_t pifs_is_buffer_erased(const void * a_buf, pifs_size_t a_buf_size)
{
    const uint8_t * buf = (uint8_t*) a_buf;
    pifs_size_t     i;
    bool_t          ret = TRUE;

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
        is_erased = pifs_is_buffer_erased(pifs.cache_page_buf, PIFS_LOGICAL_PAGE_SIZE_BYTE);
    }
    return is_erased;
}

/**
 * @brief pifs_is_buffer_programmable Check if buffer is programmable or erase
 * is needed.
 *
 * @param[in] a_orig_buf    Pointer to original buffer.
 * @param[in] a_new_buf     Pointer to new buffer.
 * @param[in] a_buf_size    Size of buffer.
 * @return TRUE: if buffer is programmable.
 * FALSE: if buffer contains at least one programmed bit.
 */
bool_t pifs_is_buffer_programmable(const void * a_orig_buf, const void * a_new_buf, pifs_size_t a_buf_size)
{
    const uint8_t * orig_buf = (uint8_t*) a_orig_buf;
    const uint8_t * new_buf = (uint8_t*) a_new_buf;
    pifs_size_t     i;
    bool_t          ret = TRUE;

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
 * @param[in] a_buf         Pointer to buffer.
 * @param[in] a_buf_size    Size of buffer.
 * @return TRUE: if buffer is programmed.
 * FALSE: if buffer contains at least one programmed bit.
 */
bool_t pifs_is_buffer_programmed(const void * a_buf, pifs_size_t a_buf_size)
{
    const uint8_t * buf = (uint8_t*) a_buf;
    pifs_size_t     i;
    bool_t          ret = TRUE;

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
 * @param[in] a_file    Pointer to file's internal structure.
 * @param[in] a_modes   String of mode.
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
    a_file->mode_deleted = FALSE;
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
                a_file->mode_write = TRUE;
                break;
            case 'b':
                /* Binary, all operations are binary! */
                break;
            case 'd':
                /* Non-standard mode, open deleted file */
                a_file->mode_deleted = TRUE;
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
 * TODO think about return code handling when this function called!
 *
 * @param[out] a_address Pointer to address to increment.
 *
 * @return PIFS_SUCCESS if successfully incremented.
 * PIFS_ERROR_INTERNAL_RANGE if end of flash is reached.
 */
pifs_status_t pifs_inc_address(pifs_address_t * a_address)
{
    pifs_status_t ret = PIFS_SUCCESS;

    a_address->page_address++;
    if (a_address->page_address == PIFS_LOGICAL_PAGE_PER_BLOCK)
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
 * @brief pifs_add_address Add page count to an address.
 * TODO think about return code handling when this function called!
 *
 * @param[out] a_address    Pointer to address to increment.
 * @param[in] a_page_count  Number of pages to add.
 *
 * @return PIFS_SUCCESS if successfully added.
 * PIFS_ERROR_INTERNAL_RANGE if end of flash is reached.
 */
pifs_status_t pifs_add_address(pifs_address_t * a_address, pifs_size_t a_page_count)
{
    pifs_status_t ret = PIFS_SUCCESS;
    pifs_size_t   pa = a_address->page_address;

    pa += a_page_count;
    a_address->block_address += pa / PIFS_LOGICAL_PAGE_PER_BLOCK;
    pa %= PIFS_LOGICAL_PAGE_PER_BLOCK;
    a_address->page_address = pa;
    if (a_address->block_address >= PIFS_FLASH_BLOCK_NUM_ALL)
    {
        PIFS_ERROR_MSG("End of flash: %s\r\n", pifs_address2str(a_address));
        ret = PIFS_ERROR_INTERNAL_RANGE;
    }
    PIFS_DEBUG_MSG("%s\r\n", pifs_address2str(a_address));

    return ret;
}

/**
 * @brief pifs_inc_ba_pa Increment address.
 * TODO think about return code handling when this function called!
 * @param[out] a_block_address Pointer to block address to increment.
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
    if (*a_page_address == PIFS_LOGICAL_PAGE_PER_BLOCK)
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
 * @brief pifs_add_ba_pa Add address.
 * TODO think about return code handling when this function called!
 * @param[out] a_block_address Pointer to block address to add.
 * @param[out] a_page_address  Pointer to page address to addr.
 * @param[in] a_page_count     Number of pages to add.
 *
 * @return PIFS_SUCCESS if successfully added.
 * PIFS_ERROR_INTERNAL_RANGE if end of flash is reached.
 */
pifs_status_t pifs_add_ba_pa(pifs_block_address_t * a_block_address,
                             pifs_page_address_t * a_page_address,
                             pifs_size_t a_page_count)
{
    pifs_status_t ret = PIFS_SUCCESS;
    pifs_size_t   pa = (*a_page_address);

    pa += a_page_count;
    (*a_block_address) += pa / PIFS_LOGICAL_PAGE_PER_BLOCK;
    pa %= PIFS_LOGICAL_PAGE_PER_BLOCK;
    *a_page_address = pa;
    if (*a_block_address >= PIFS_FLASH_BLOCK_NUM_ALL)
    {
        PIFS_ERROR_MSG("End of flash: %s\r\n",
                       pifs_ba_pa2str(*a_block_address, *a_page_address));
        ret = PIFS_ERROR_INTERNAL_RANGE;
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
#if PIFS_PATH_SEPARATOR_CHAR == '/'
    /* Backslash is invalid */
    const pifs_char_t invalid_chars[] = "\"'*,:;<=>?[\\]|";
#elif PIFS_PATH_SEPARATOR_CHAR == '\\'
    /* Slash is invalid */
    const pifs_char_t invalid_chars[] = "\"'*,/:;<=>?[]|";
#endif

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

typedef struct
{
    pifs_block_address_t * blocks;
    pifs_size_t            blocks_size;
} pifs_block_walker_t;

static pifs_status_t pifs_block_walker(pifs_file_t * a_file,
                                       pifs_block_address_t a_block_address,
                                       pifs_page_address_t a_page_address,
                                       pifs_block_address_t a_delta_block_address,
                                       pifs_page_address_t a_delta_page_address,
                                       bool_t a_map_page,
                                       void * a_func_data)
{
    pifs_block_walker_t * params = (pifs_block_walker_t*) a_func_data;
    pifs_size_t           i;
    bool_t                end = FALSE;

    (void) a_file;
    (void) a_block_address;
    (void) a_page_address;
    (void) a_delta_page_address;

    printf("%i: %i/%i -> %i/%i\r\n", a_map_page,
            a_block_address, a_page_address,
            a_delta_block_address, a_delta_page_address);
    if (!a_map_page)
    {
        for (i = 0; i < params->blocks_size && !end; i++)
        {
            if (params->blocks[i] == a_delta_block_address)
            {
                end = TRUE;
            }
            else if (params->blocks[i] == PIFS_BLOCK_ADDRESS_INVALID)
            {
                params->blocks[i] = a_delta_block_address;
                end = TRUE;
            }
        }
    }

    return PIFS_SUCCESS;
}

/**
 * @brief pifs_get_file_blocks Get list of blocks which holds file's
 * content. 
 * Note: This function is used for debugging.
 *
 * @param[in] a_filename    File name to check.
 * @param[out] a_blocks     List of used blocks.
 * @param[in] a_blocks_size Size of a_blocks.
 * @param[out] a_blocks_num Number of blocks found and written to list.
 */
pifs_status_t pifs_get_file_blocks(pifs_char_t * a_filename,
                                   pifs_block_address_t * a_blocks,
                                   pifs_size_t a_blocks_size,
                                   pifs_size_t * a_blocks_num)
{
    pifs_file_t        * file;
    pifs_status_t        ret = PIFS_ERROR_GENERAL;
    pifs_block_walker_t  block_walker;
    pifs_size_t          i;
    bool_t               end = FALSE;

    *a_blocks_num = 0;
        printf("Opened %s\r\n", a_filename);
    file = pifs_fopen(a_filename, "r");
    if (file)
    {
        block_walker.blocks = a_blocks;
        block_walker.blocks_size = a_blocks_size;
        for (i = 0; i < a_blocks_size; i++)
        {
            a_blocks[i] = PIFS_BLOCK_ADDRESS_INVALID;
        }
        ret = pifs_walk_file_pages(file, pifs_block_walker, &block_walker);
        if (ret == PIFS_SUCCESS)
        {
            for (i = 0; i < a_blocks_size && !end; i++)
            {
                if (a_blocks[i] != PIFS_BLOCK_ADDRESS_INVALID)
                {
                    (*a_blocks_num)++;
                }
                else
                {
                    end = TRUE;
                }
            }
        }
        pifs_fclose(file);
    }
    else
    {
        ret = PIFS_ERROR_FILE_NOT_FOUND;
        //ret = pifs_errno;
    }

    return ret;
}
