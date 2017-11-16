/**
 * @file        pifs_entry.c
 * @brief       Pi file system: entry list handling functions
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-11-16 19:08:50 ivanovp {Time-stamp}
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
#include "pifs_helper.h"
#include "pifs_delta.h"
#include "pifs_entry.h"
#include "pifs_merge.h"
#include "buffer.h" /* DEBUG */

#define PIFS_DEBUG_LEVEL 2
#include "pifs_debug.h"

/**
 * @brief pifs_read_entry Read one file or directory entry from entry list.
 *
 * @param[in] a_entry_list_block_address Block address of entry list.
 * @param[in] a_entry_list_page_address  Page address of entry list.
 * @param[in] a_entry_idx                Index of entry in the entry list.
 * @param[out] a_entry                   Pointer to entry to fill.
 * @param[out] a_is_erased               TRUE: entry is erased (emptry),
 *                                       FALSE: entry is written.
 * @return PIFS_SUCCESS if entry was read successfully.
 */
pifs_status_t pifs_read_entry(pifs_block_address_t a_entry_list_block_address,
                              pifs_page_address_t a_entry_list_page_address,
                              pifs_size_t a_entry_idx,
                              pifs_entry_t * const a_entry,
                              bool_t * const a_is_erased)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba = a_entry_list_block_address;
    pifs_page_address_t  pa = a_entry_list_page_address;
    pifs_checksum_t      checksum;

#if PIFS_USE_DELTA_FOR_ENTRIES
    ret = pifs_read_delta(ba, pa, a_entry_idx * PIFS_ENTRY_SIZE_BYTE, a_entry,
                          PIFS_ENTRY_SIZE_BYTE);
#else
    ret = pifs_read(ba, pa, a_entry_idx * PIFS_ENTRY_SIZE_BYTE, a_entry,
                    PIFS_ENTRY_SIZE_BYTE);
#endif
    if (ret == PIFS_SUCCESS)
    {
        *a_is_erased = pifs_is_buffer_erased(a_entry, PIFS_ENTRY_SIZE_BYTE);

        if (!(*a_is_erased))
        {
            checksum = pifs_calc_checksum(a_entry, sizeof(pifs_entry_t) - sizeof(pifs_checksum_t));

            if (checksum != a_entry->checksum)
            {
                ret = PIFS_ERROR_CHECKSUM;
            }
        }
    }

    return ret;
}

/**
 * @brief pifs_write_entry Write one file or directory entry to entry list.
 *
 * @param[in] a_entry_list_block_address Block address of entry list.
 * @param[in] a_entry_list_page_address  Page address of entry list.
 * @param[in] a_entry_idx                Index of entry in the entry list.
 * @param[in] a_calc_crc                 TRUE: calculate CRC field
 * @param[out] a_entry                   Pointer to entry to write.
 * @return PIFS_SUCCESS if entry was written successfully.
 */
pifs_status_t pifs_write_entry(pifs_block_address_t a_entry_list_block_address,
                              pifs_page_address_t a_entry_list_page_address,
                              pifs_size_t a_entry_idx,
                              bool_t a_calc_crc,
                              pifs_entry_t * const a_entry)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba = a_entry_list_block_address;
    pifs_page_address_t  pa = a_entry_list_page_address;
            
    if (a_calc_crc)
    {
        a_entry->checksum = pifs_calc_checksum(a_entry, sizeof(pifs_entry_t) - sizeof(pifs_checksum_t));
    }

#if PIFS_USE_DELTA_FOR_ENTRIES
    ret = pifs_write_delta(ba, pa, a_entry_idx * PIFS_ENTRY_SIZE_BYTE, a_entry,
                          PIFS_ENTRY_SIZE_BYTE);
#else
    ret = pifs_write(ba, pa, a_entry_idx * PIFS_ENTRY_SIZE_BYTE, a_entry,
                    PIFS_ENTRY_SIZE_BYTE);
#endif

    return ret;
}

/**
 * @brief pifs_append_entry Add an item to the entry list.
 *
 * @param a_entry[in] Pointer to the entry to be added.
 * @return PIFS_SUCCESS if entry successfully added.
 * PIFS_ERROR_NO_MORE_ENTRY if entry list is full.
 * PIFS_ERROR_FLASH_WRITE if flash write failed.
 */
pifs_status_t pifs_append_entry(pifs_entry_t * const a_entry,
                                pifs_block_address_t a_entry_list_block_address,
                                pifs_page_address_t a_entry_list_page_address)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba = a_entry_list_block_address;
    pifs_page_address_t  pa = a_entry_list_page_address;
    bool_t               created = FALSE;
    bool_t               is_erased = FALSE;
    pifs_entry_t         entry; /* TODO do not store on stack, it can be large! */
    pifs_size_t          i;
    pifs_size_t          j;
    pifs_size_t          free_entry_count;
    pifs_size_t          to_be_released_entry_count;

    PIFS_DEBUG_MSG("name: [%s] entry list address: %s\r\n", a_entry->name,
                   pifs_ba_pa2str(ba, pa));

    if (!pifs.is_merging)
    {
        /* Not merging, normal operation.
         * PIFS_OPEN_FILE_NUM_MAX entries are reserved for merging, check if 
         * there is enough entries left.
         */
        ret = pifs_count_entries(&free_entry_count, &to_be_released_entry_count,
                a_entry_list_block_address,
                a_entry_list_page_address);

        if (ret == PIFS_SUCCESS && free_entry_count <= PIFS_OPEN_FILE_NUM_MAX)
        {
            ret = PIFS_ERROR_NO_MORE_ENTRY;
        }
    }

    for (j = 0; j < PIFS_ENTRY_LIST_SIZE_PAGE && !created && ret == PIFS_SUCCESS; j++)
    {
        for (i = 0; i < PIFS_ENTRY_PER_PAGE && !created && ret == PIFS_SUCCESS; i++)
        {
            is_erased = FALSE;
            (void)pifs_read_entry(ba, pa, i, &entry, &is_erased);
            if (is_erased)
            {
                /* Empty entry found */
                ret = pifs_write_entry(ba, pa, i, TRUE, a_entry);
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
        ret = pifs_inc_ba_pa(&ba, &pa);
    }
    if (ret == PIFS_SUCCESS && !created)
    {
        PIFS_ERROR_MSG("No more space!\r\n");
        ret = PIFS_ERROR_NO_MORE_ENTRY;
    }

    return ret;
}

/**
 * @brief pifs_update_entry Find entry in entry list and update its content.
 *
 * @param[in] a_name[in]    Pointer to name to find.
 * @param[in] a_entry[in]   Pointer to entry to update. NULL: clear entry.
 * @param[in] a_entry_list_block_address Block address of entry list.
 * @param[in] a_entry_list_page_address  Page address of entry list.
 * @param[in] a_is_merge_allowed TRUE: merge is allowed when not enough space. FALSE: merge is not allowed.
 *
 * @return PIFS_SUCCESS if entry found.
 * PIFS_ERROR_FILE_NOT_FOUND if entry not found.
 */
pifs_status_t pifs_update_entry(const pifs_char_t * a_name, pifs_entry_t * const a_entry,
                                pifs_block_address_t a_entry_list_block_address,
                                pifs_page_address_t a_entry_list_page_address,
                                bool_t a_is_merge_allowed)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba = a_entry_list_block_address;
    pifs_page_address_t  pa = a_entry_list_page_address;
    bool_t               found = FALSE;
    bool_t               is_erased = FALSE;
    pifs_entry_t         entry; /* TODO do not store on stack, it can be large! */
    pifs_size_t          i;
    pifs_size_t          j;

    PIFS_DEBUG_MSG("name: [%s] entry list address: %s\r\n", a_name,
                   pifs_ba_pa2str(ba, pa));

    for (j = 0; j < PIFS_ENTRY_LIST_SIZE_PAGE && !found && ret == PIFS_SUCCESS; j++)
    {
        for (i = 0; i < PIFS_ENTRY_PER_PAGE && !found && ret == PIFS_SUCCESS; i++)
        {
            ret = pifs_read_entry(ba, pa, i, &entry, &is_erased);
            /* Check if name matches and not deleted */
            if (ret == PIFS_SUCCESS
                    && !is_erased
                    && (strncmp((char*)entry.name, a_name, sizeof(entry.name)) == 0)
                    && !pifs_is_entry_deleted(&entry))
            {
                /* Entry found */
                /* Copy entry */
#if PIFS_USE_DELTA_FOR_ENTRIES
                ret = pifs_write_entry(ba, pa, i, TRUE, a_entry);
#else
                /* Clear entry because file content will be re-used */
                memset(&entry, PIFS_FLASH_PROGRAMMED_BYTE_VALUE, PIFS_ENTRY_SIZE_BYTE);
                ret = pifs_write_entry(ba, pa, i, FALSE, &entry);
                if (ret == PIFS_SUCCESS)
                {
                    ret = pifs_append_entry(a_entry,
                            a_entry_list_block_address,
                            a_entry_list_page_address);
                    if (ret == PIFS_ERROR_NO_MORE_ENTRY)
                    {
                        /* If there is not enough space, nothing to do */
                        /* pifs_merge_check() tries to release enough space */
                        /* to be able to close all opened files for merge. */
                        PIFS_ERROR_MSG("Cannot update entry!\r\n");
                    }
                    else
                    {
                        PIFS_NOTICE_MSG("Entry appended\r\n");
                    }
                    if (a_is_merge_allowed)
                    {
                        ret = pifs_merge_check(NULL, 0);
                    }
                }
#endif
                found = TRUE;
            }
        }
        ret = pifs_inc_ba_pa(&ba, &pa);
    }

    if (ret == PIFS_SUCCESS && !found)
    {
        ret = PIFS_ERROR_FILE_NOT_FOUND;
    }

    return ret;
}

/**
 * @brief pifs_mark_entry_deleted Delete entry by marking the corresponding
 * attribute bit.
 *
 * @param[in] a_entry   Entry to be deleted.
 */
void pifs_mark_entry_deleted(pifs_entry_t * a_entry)
{
#if PIFS_ENABLE_ATTRIBUTES
    PIFS_SET_ATTRIB(a_entry->attrib, PIFS_ATTRIB_DELETED);
#else
    /* Change only the first character to programmed value */
    /* This way file system check can find it and take into account */
    a_entry->name[0] = PIFS_FLASH_PROGRAMMED_BYTE_VALUE;
#endif
}

/**
 * @brief pifs_is_entry_deleted Check if entry is deleted.
 *
 * @param[in] a_entry           Entry to be checked.
 * @param[in] a_is_raw_entry    TRUE: entry was read with pifs_read(),
 *                              FALSE: entry was read with pifs_read_entry().
 *
 * @return TRUE: entry is deleted, FALSE: entry exist.
 */
bool_t pifs_is_entry_deleted(pifs_entry_t * a_entry)
{
    bool_t is_deleted = FALSE;

#if PIFS_ENABLE_ATTRIBUTES
    if (PIFS_IS_DELETED(a_entry->attrib))
#else
    if (a_entry->name[0] == PIFS_FLASH_PROGRAMMED_BYTE_VALUE)
#endif
    {
        is_deleted = TRUE;
    }

    return is_deleted;
}

/**
 * @brief pifs_find_entry Find entry in entry list.
 *
 * @param[in] a_entry_cmd   Command to run. @see PIFS_FIND_ENTRY, PIFS_DELETE_ENTRY, PIFS_CLEAR_ENTRY
 * @param[in] a_name        Pointer to name to find.
 * @param[out] a_entry      Pointer to entry to fill. NULL: clear entry.
 * @param[in] a_entry_list_block_address Current list entry's address.
 * @param[in] a_entry_list_page_address  Current list entry's address.
 * @return PIFS_SUCCESS     if entry found.
 * PIFS_ERROR_FILE_NOT_FOUND if entry not found.
 */
pifs_status_t pifs_find_entry(pifs_entry_cmd_t a_entry_cmd,
                              const pifs_char_t * a_name, pifs_entry_t * const a_entry,
                              pifs_block_address_t a_entry_list_block_address,
                              pifs_page_address_t a_entry_list_page_address)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba = a_entry_list_block_address;
    pifs_page_address_t  pa = a_entry_list_page_address;
    bool_t               found = FALSE;
    bool_t               is_erased = FALSE;
    pifs_entry_t         entry;
    pifs_size_t          i;
    pifs_size_t          j;

    PIFS_DEBUG_MSG("cmd: %i, name: [%s], entry list address: %s\r\n",
                   a_entry_cmd, a_name, pifs_ba_pa2str(ba, pa));

    for (j = 0; j < PIFS_ENTRY_LIST_SIZE_PAGE && !found && !is_erased && ret == PIFS_SUCCESS; j++)
    {
        for (i = 0; i < PIFS_ENTRY_PER_PAGE && !found && !is_erased && ret == PIFS_SUCCESS; i++)
        {
            ret = pifs_read_entry(ba, pa, i, &entry, &is_erased);
            /* Check if name matches and not deleted */
            if (ret == PIFS_SUCCESS
                    && (strncmp((char*)entry.name, a_name, sizeof(entry.name)) == 0)
                    && !pifs_is_entry_deleted(&entry))
            {
                /* Entry found */
                if (a_entry)
                {
                    /* Copy entry */
                    memcpy(a_entry, &entry, PIFS_ENTRY_SIZE_BYTE);
                    PIFS_DEBUG_MSG("file size: %i bytes\r\n", a_entry->file_size);
                }
                if (a_entry_cmd == PIFS_FIND_ENTRY)
                {
                    /* Already copied */
                }
                else if (a_entry_cmd == PIFS_DELETE_ENTRY || a_entry_cmd == PIFS_CLEAR_ENTRY)
                {
                    // CRC would be wrong!
//                    if (a_entry_cmd == PIFS_DELETE_ENTRY)
//                    {
//                        pifs_mark_entry_deleted(&entry);
//                    }
//                    else
                    {
                        memset(&entry, PIFS_FLASH_PROGRAMMED_BYTE_VALUE, PIFS_ENTRY_SIZE_BYTE);
                    }
                    ret = pifs_write_entry(ba, pa, i, FALSE, &entry);
                }
                found = TRUE;
            }
        }
        if (j < PIFS_ENTRY_LIST_SIZE_PAGE - 1)
        {
            ret = pifs_inc_ba_pa(&ba, &pa);
        }
    }

    if (ret == PIFS_SUCCESS && !found)
    {
        ret = PIFS_ERROR_FILE_NOT_FOUND;
    }

    return ret;
}

/**
 * @brief pifs_delete_entry Find entry in entry list and invalidate it.
 * This function should be used only when a file is removed.
 *
 * @param a_name[in]    Pointer to name to find.
 * @return PIFS_SUCCESS if entry found and deleted.
 * PIFS_ERROR_FILE_NOT_FOUND if entry not found.
 */
pifs_status_t pifs_delete_entry(const pifs_char_t * a_name,
                               pifs_block_address_t a_entry_list_block_address,
                               pifs_page_address_t a_entry_list_page_address)
{
    return pifs_find_entry(PIFS_DELETE_ENTRY,
                           a_name, NULL,
                           a_entry_list_block_address,
                           a_entry_list_page_address);
}

/**
 * @brief pifs_clear_entry Find entry in entry list and zero all bytes of it.
 * This function should be used only when a file is renamed.
 *
 * @param a_name[in]    Pointer to name to find.
 * @return PIFS_SUCCESS if entry found and cleared.
 * PIFS_ERROR_FILE_NOT_FOUND if entry not found.
 */
pifs_status_t pifs_clear_entry(const pifs_char_t * a_name,
                               pifs_block_address_t a_entry_list_block_address,
                               pifs_page_address_t a_entry_list_page_address)
{
    return pifs_find_entry(PIFS_CLEAR_ENTRY,
                           a_name, NULL,
                           a_entry_list_block_address,
                           a_entry_list_page_address);
}

/**
 * @brief pifs_count_entries Count free items in the entry list.
 *
 * @param a_entry[in] Pointer to the entry to be added.
 * @return PIFS_SUCCESS if entry successfully added.
 * PIFS_ERROR_NO_MORE_ENTRY if entry list is full.
 * PIFS_ERROR_FLASH_WRITE if flash write failed.
 */
pifs_status_t pifs_count_entries(pifs_size_t * a_free_entry_count, pifs_size_t * a_to_be_released_entry_count,
                                pifs_block_address_t a_entry_list_block_address,
                                pifs_page_address_t a_entry_list_page_address)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba = a_entry_list_block_address;
    pifs_page_address_t  pa = a_entry_list_page_address;
    pifs_entry_t         entry; /* TODO do not store on stack! */
    pifs_size_t          i;
    pifs_size_t          j;
    pifs_size_t          free_entry_count = 0;
    pifs_size_t          to_be_released_entry_count = 0;
    bool_t               is_erased = FALSE;

    for (j = 0; j < PIFS_ENTRY_LIST_SIZE_PAGE && ret == PIFS_SUCCESS; j++)
    {
        for (i = 0; i < PIFS_ENTRY_PER_PAGE && ret == PIFS_SUCCESS; i++)
        {
            ret = pifs_read_entry(ba, pa, i, &entry, &is_erased);
            /* Check if this area is used */
            if (is_erased)
            {
                /* Empty entry found */
                free_entry_count++;
            }
            if (pifs_is_entry_deleted(&entry))
            {
                /* Cleared entry found */
                to_be_released_entry_count++;
            }
        }
        ret = pifs_inc_ba_pa(&ba, &pa);
    }
    *a_free_entry_count = free_entry_count;
    *a_to_be_released_entry_count = to_be_released_entry_count;

    return ret;
}
