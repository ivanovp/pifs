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

#define PIFS_DEBUG_LEVEL    4
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
    pifs_entry_t         entry;
    pifs_size_t          i;
    pifs_size_t          page_cntr = PIFS_ENTRY_LIST_SIZE_PAGE;

    /* Invert attribute bits */
    a_entry->attrib ^= PIFS_ATTRIB_ALL;
    /* TODO this while/while/for should be reworked! */
    while (ba < pifs.header.entry_list_address.block_address + PIFS_MANAGEMENT_BLOCKS
           && !created && ret == PIFS_SUCCESS && page_cntr)
    {
        while (pa < PIFS_FLASH_PAGE_PER_BLOCK && !created && ret == PIFS_SUCCESS && page_cntr)
        {
            for (i = 0; i < PIFS_ENTRY_PER_PAGE && !created && ret == PIFS_SUCCESS; i++)
            {
#if PIFS_USE_DELTA_FOR_ENTRIES
                ret = pifs_read_delta(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, &entry,
                                      PIFS_ENTRY_SIZE_BYTE);
#else
                ret = pifs_read(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, &entry,
                                PIFS_ENTRY_SIZE_BYTE);
#endif
                /* Check if this area is used */
                if (pifs_is_buffer_erased(&entry, sizeof(entry)))
                {
                    /* Empty entry found */
#if PIFS_USE_DELTA_FOR_ENTRIES
                    ret = pifs_write_delta(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, a_entry,
                                           sizeof(pifs_entry_t), NULL);
#else
                    ret = pifs_write(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, a_entry,
                                     sizeof(pifs_entry_t));
#endif
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
            page_cntr--;
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
    pifs_entry_t         entry;
    pifs_size_t          i;
    pifs_size_t          page_cntr = PIFS_ENTRY_LIST_SIZE_PAGE;

    /* TODO this while/while/for should be reworked! */
    while (ba < pifs.header.entry_list_address.block_address + PIFS_MANAGEMENT_BLOCKS
           && !found && ret == PIFS_SUCCESS && page_cntr)
    {
        while (pa < PIFS_FLASH_PAGE_PER_BLOCK && !found && ret == PIFS_SUCCESS && page_cntr)
        {
            for (i = 0; i < PIFS_ENTRY_PER_PAGE && !found; i++)
            {
#if PIFS_USE_DELTA_FOR_ENTRIES
                ret = pifs_read_delta(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, &entry,
                                      PIFS_ENTRY_SIZE_BYTE);
#else
                ret = pifs_read(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, &entry,
                                PIFS_ENTRY_SIZE_BYTE);
#endif
                /* Check if name matches */
                if (strncmp((char*)entry.name, a_name, sizeof(entry.name)) == 0)
                {
                    /* Entry found */
                    /* Copy entry */
#if PIFS_USE_DELTA_FOR_ENTRIES
                    /* Invert entry bits as it is stored inverted */
                    a_entry->attrib ^= PIFS_ATTRIB_ALL;
                    ret = pifs_write_delta(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, a_entry,
                                           PIFS_ENTRY_SIZE_BYTE, NULL);
                    /* Invert entry bits as it is stored inverted */
                    a_entry->attrib ^= PIFS_ATTRIB_ALL;
#else
                    if (pifs_is_buffer_programmable(&entry, a_entry, PIFS_ENTRY_SIZE_BYTE))
                    {
                        PIFS_NOTICE_MSG("Entry can be updated\r\n");
                        /* Invert entry bits as it is stored inverted */
                        a_entry->attrib ^= PIFS_ATTRIB_ALL;
                        ret = pifs_write(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, a_entry,
                                         PIFS_ENTRY_SIZE_BYTE);
                        /* Invert entry bits as it is stored inverted */
                        a_entry->attrib ^= PIFS_ATTRIB_ALL;
                    }
                    else
                    {
                        PIFS_NOTICE_MSG("Entry CANNOT be updated!\r\n");
                        /* Clear entry */
                        memset(&entry, PIFS_FLASH_PROGRAMMED_BYTE_VALUE, PIFS_ENTRY_SIZE_BYTE);
                        ret = pifs_write(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, &entry,
                                         PIFS_ENTRY_SIZE_BYTE);
                        if (ret == PIFS_SUCCESS)
                        {
                            ret = pifs_append_entry(a_entry);
                            if (ret == PIFS_ERROR_NO_MORE_SPACE)
                            {
                                /* If entry cannot be appended, merge the blocks */
                                ret = pifs_merge();
                                if (ret == PIFS_SUCCESS)
                                {
                                    ret = pifs_append_entry(a_entry);
                                }
                            }
                        }
                    }
#endif
                    found = TRUE;
                }
            }
            pa++;
            page_cntr--;
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
    pifs_entry_t         entry;
    pifs_size_t          i;
    pifs_size_t          page_cntr = PIFS_ENTRY_LIST_SIZE_PAGE;

    /* TODO this while/while/for should be reworked! */
    while (ba < pifs.header.entry_list_address.block_address + PIFS_MANAGEMENT_BLOCKS
           && !found && ret == PIFS_SUCCESS && page_cntr)
    {
        while (pa < PIFS_FLASH_PAGE_PER_BLOCK && !found && ret == PIFS_SUCCESS && page_cntr)
        {
            for (i = 0; i < PIFS_ENTRY_PER_PAGE && !found; i++)
            {
#if PIFS_USE_DELTA_FOR_ENTRIES
                ret = pifs_read_delta(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, &entry,
                                      PIFS_ENTRY_SIZE_BYTE);
#else
                ret = pifs_read(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, &entry,
                                PIFS_ENTRY_SIZE_BYTE);
#endif
                /* Check if name matches */
                if (strncmp((char*)entry.name, a_name, sizeof(entry.name)) == 0)
                {
                    /* Entry found */
                    if (a_entry)
                    {
                        /* Copy entry */
                        memcpy(a_entry, &entry, sizeof(pifs_entry_t));
                        /* Invert entry bits as it is stored inverted */
                        a_entry->attrib ^= PIFS_ATTRIB_ALL;
                        PIFS_DEBUG_MSG("file size: %i bytes\r\n", a_entry->file_size);
                    }
                    else
                    {
                        /* Clear entry */
                        memset(&entry, PIFS_FLASH_PROGRAMMED_BYTE_VALUE, sizeof(pifs_entry_t));
#if PIFS_USE_DELTA_FOR_ENTRIES
                        ret = pifs_write_delta(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, &entry,
                                               PIFS_ENTRY_SIZE_BYTE, NULL);
#else
                        ret = pifs_write(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, &entry,
                                         PIFS_ENTRY_SIZE_BYTE);
#endif
                    }
                    found = TRUE;
                }
            }
            pa++;
            page_cntr--;
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
