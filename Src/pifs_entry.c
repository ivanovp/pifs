/**
 * @file        pifs_entry.c
 * @brief       Pi file system: entry list handling functions
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
#include "pifs_fsbm.h"
#include "pifs_helper.h"
#include "pifs_delta.h"
#include "pifs_entry.h"
#include "buffer.h" /* DEBUG */

#define PIFS_DEBUG_LEVEL    5
#include "pifs_debug.h"

/**
 * @brief pifs_append_entry Add an item to the entry list.
 *
 * @param a_entry[in] Pointer to the entry to be added.
 * @return PIFS_SUCCESS if entry successfully added.
 * PIFS_ERROR_NO_MORE_SPACE if entry list is full.
 * PIFS_ERROR_FLASH_WRITE if flash write failed.
 */
pifs_status_t pifs_append_entry(pifs_entry_t * a_entry)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba = pifs.header.entry_list_address.block_address;
    pifs_page_address_t  pa = pifs.header.entry_list_address.page_address;
    bool_t               created = FALSE;
    pifs_entry_t       * entry = (pifs_entry_t*) pifs.cache_page_buf;
    pifs_size_t          i;

    /* Invert attribute bits */
    a_entry->attrib ^= PIFS_ATTRIB_ALL;
    while (ba < pifs.header.entry_list_address.block_address + PIFS_MANAGEMENT_BLOCKS
           && !created && ret == PIFS_SUCCESS)
    {
        while (pa < PIFS_FLASH_PAGE_PER_BLOCK && !created && ret == PIFS_SUCCESS)
        {
            ret = pifs_read(ba, pa, 0, NULL, PIFS_ENTRY_PER_PAGE * PIFS_ENTRY_SIZE_BYTE);
            for (i = 0; i < PIFS_ENTRY_PER_PAGE && !created && ret == PIFS_SUCCESS; i++)
            {
                /* Check if this area is used */
                if (pifs_is_buffer_erased(&entry[i], sizeof(entry)))
                {
                    /* Empty entry found */
                    ret = pifs_write(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, a_entry, sizeof(pifs_entry_t));
                    if (ret == PIFS_SUCCESS)
                    {
                        created = TRUE;
                    }
                    else
                    {
                        PIFS_ERROR_MSG("Cannot create entry!");
                        ret = PIFS_ERROR_FLASH_WRITE;
                    }
                }
            }
            pa++;
        }
        ba++;
        pa = 0;
    }
    if (!created)
    {
        ret = PIFS_ERROR_NO_MORE_SPACE;
    }
    /* Restore attribute bits */
    a_entry->attrib ^= PIFS_ATTRIB_ALL;

    return ret;
}

/**
 * @brief pifs_update_entry Find entry in entry list and update its content.
 *
 * @param a_name[in]    Pointer to name to find.
 * @param a_entry[in]   Pointer to entry to update. NULL: clear entry.
 * @return PIFS_SUCCESS if entry found.
 * PIFS_ERROR_FILE_NOT_FOUND if entry not found.
 */
pifs_status_t pifs_update_entry(const pifs_char_t * a_name, pifs_entry_t * const a_entry)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba = pifs.header.entry_list_address.block_address;
    pifs_page_address_t  pa = pifs.header.entry_list_address.page_address;
    bool_t               found = FALSE;
    pifs_entry_t       * entry = (pifs_entry_t*) pifs.cache_page_buf;
    pifs_size_t          i;

    /* Invert entry bits as it is stored inverted */
    a_entry->attrib ^= PIFS_ATTRIB_ALL;
    while (ba < pifs.header.entry_list_address.block_address + PIFS_MANAGEMENT_BLOCKS
           && !found && ret == PIFS_SUCCESS)
    {
        while (pa < PIFS_FLASH_PAGE_PER_BLOCK && !found && ret == PIFS_SUCCESS)
        {
            ret = pifs_read(ba, pa, 0, NULL, PIFS_ENTRY_PER_PAGE * PIFS_ENTRY_SIZE_BYTE);
            for (i = 0; i < PIFS_ENTRY_PER_PAGE && !found; i++)
            {
                /* Check if name matches */
                if (strncmp((char*)entry[i].name, a_name, sizeof(entry[i].name)) == 0)
                {
                    /* Entry found */
                    /* Copy entry */
                    memcpy(&entry[i], a_entry, sizeof(pifs_entry_t));
                    ret = pifs_write(ba, pa, 0, NULL, PIFS_ENTRY_PER_PAGE * PIFS_ENTRY_SIZE_BYTE);
                    found = TRUE;
                }
            }
            pa++;
        }
        pa = 0;
        ba++;
    }

    if (ret == PIFS_SUCCESS && !found)
    {
        ret = PIFS_ERROR_FILE_NOT_FOUND;
    }

    return ret;
}

/**
 * @brief pifs_find_entry Find entry in entry list.
 *
 * @param a_name[in]    Pointer to name to find.
 * @param a_entry[out]  Pointer to entry to fill. NULL: clear entry.
 * @return PIFS_SUCCESS if entry found.
 * PIFS_ERROR_FILE_NOT_FOUND if entry not found.
 */
pifs_status_t pifs_find_entry(const pifs_char_t * a_name, pifs_entry_t * const a_entry)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba = pifs.header.entry_list_address.block_address;
    pifs_page_address_t  pa = pifs.header.entry_list_address.page_address;
    bool_t               found = FALSE;
    pifs_entry_t       * entry = (pifs_entry_t*) pifs.cache_page_buf;
    pifs_size_t          i;

    while (ba < pifs.header.entry_list_address.block_address + PIFS_MANAGEMENT_BLOCKS
           && !found && ret == PIFS_SUCCESS)
    {
        while (pa < PIFS_FLASH_PAGE_PER_BLOCK && !found && ret == PIFS_SUCCESS)
        {
            ret = pifs_read(ba, pa, 0, NULL, PIFS_ENTRY_PER_PAGE * PIFS_ENTRY_SIZE_BYTE);
            for (i = 0; i < PIFS_ENTRY_PER_PAGE && !found; i++)
            {
                /* Check if name matches */
                if (strncmp((char*)entry[i].name, a_name, sizeof(entry[i].name)) == 0)
                {
                    /* Entry found */
                    if (a_entry)
                    {
                        /* Copy entry */
                        memcpy(a_entry, &entry[i], sizeof(pifs_entry_t));
                        /* Invert entry bits as it is stored inverted */
                        a_entry->attrib ^= PIFS_ATTRIB_ALL;

#if 0
                        if  (a_entry->file_size == PIFS_FILE_SIZE_ERASED)
                        {
                            printf("FILE SIZE IS NOT FILLED!\r\n");
                        }
#endif
                        PIFS_DEBUG_MSG("file size: %i bytes\r\n", a_entry->file_size);
                    }
                    else
                    {
                        /* Clear entry */
                        memset(&entry[i], PIFS_FLASH_PROGRAMMED_BYTE_VALUE, sizeof(pifs_entry_t));
                        ret = pifs_write(ba, pa, 0, NULL, PIFS_ENTRY_PER_PAGE * PIFS_ENTRY_SIZE_BYTE);
                    }
                    found = TRUE;
                }
            }
            pa++;
        }
        pa = 0;
        ba++;
    }

    if (ret == PIFS_SUCCESS && !found)
    {
        ret = PIFS_ERROR_FILE_NOT_FOUND;
    }

    return ret;
}

/**
 * @brief pifs_clear_entry Find entry in entry list and invalidate it.
 * FIXME this function should not be used! pifs_write_delta() should be used
 * instead!
 *
 * @param a_name[in]    Pointer to name to find.
 * @return PIFS_SUCCESS if entry found and cleared.
 * PIFS_ERROR_FILE_NOT_FOUND if entry not found.
 */
pifs_status_t pifs_clear_entry(const pifs_char_t * a_name)
{
    return pifs_find_entry(a_name, NULL);
}
