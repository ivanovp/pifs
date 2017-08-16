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

#define PIFS_DEBUG_LEVEL 2
#include "pifs_debug.h"

pifs_status_t pifs_read_entry(pifs_block_address_t a_entry_list_block_address,
                              pifs_page_address_t a_entry_list_page_address,
                              pifs_size_t a_entry_idx,
                              pifs_entry_t * const a_entry,
                              bool_t * const a_is_erased)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba = a_entry_list_block_address;
    pifs_page_address_t  pa = a_entry_list_page_address;

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
    }

    return ret;
}

pifs_status_t pifs_write_entry(pifs_block_address_t a_entry_list_block_address,
                              pifs_page_address_t a_entry_list_page_address,
                              pifs_size_t a_entry_idx,
                              const pifs_entry_t * const a_entry)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba = a_entry_list_block_address;
    pifs_page_address_t  pa = a_entry_list_page_address;

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
 * PIFS_ERROR_NO_MORE_SPACE if entry list is full.
 * PIFS_ERROR_FLASH_WRITE if flash write failed.
 */
pifs_status_t pifs_append_entry(const pifs_entry_t * const a_entry,
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

    PIFS_DEBUG_MSG("name: [%s] entry list address: %s\r\n", a_entry->name,
                   pifs_ba_pa2str(ba, pa));

    for (j = 0; j < PIFS_ENTRY_LIST_SIZE_PAGE && !created && ret == PIFS_SUCCESS; j++)
    {
        for (i = 0; i < PIFS_ENTRY_PER_PAGE && !created && ret == PIFS_SUCCESS; i++)
        {
            ret = pifs_read_entry(ba, pa, i, &entry, &is_erased);
            if (is_erased)
            {
                /* Empty entry found */
                ret = pifs_write_entry(ba, pa, i, a_entry);
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
    if (!created)
    {
        ret = PIFS_ERROR_NO_MORE_SPACE;
    }

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
pifs_status_t pifs_update_entry(const pifs_char_t * a_name, pifs_entry_t * const a_entry,
                                pifs_block_address_t a_entry_list_block_address,
                                pifs_page_address_t a_entry_list_page_address)
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
                ret = pifs_write_entry(ba, pa, i, a_entry);
#else
                if (pifs_is_buffer_programmable(&entry, a_entry, PIFS_ENTRY_SIZE_BYTE))
                {
                    PIFS_NOTICE_MSG("Entry can be updated\r\n");
                    ret = pifs_write_entry(ba, pa, i, a_entry);
                }
                else
                {
                    PIFS_NOTICE_MSG("Original entry:\r\n");
#if PIFS_DEBUG_LEVEL >= 3
                    print_buffer(&entry, PIFS_ENTRY_SIZE_BYTE, 0);
#endif
                    PIFS_NOTICE_MSG("New entry:\r\n");
#if PIFS_DEBUG_LEVEL >= 3
                    print_buffer(a_entry, PIFS_ENTRY_SIZE_BYTE, 0);
#endif
                    PIFS_NOTICE_MSG("Entry CANNOT be updated!\r\n");
                    /* Clear entry because file content will be re-used */
                    memset(&entry, PIFS_FLASH_PROGRAMMED_BYTE_VALUE, PIFS_ENTRY_SIZE_BYTE);
                    ret = pifs_write_entry(ba, pa, i, &entry);
                    if (ret == PIFS_SUCCESS)
                    {
                        ret = pifs_append_entry(a_entry,
                                                a_entry_list_block_address,
                                                a_entry_list_page_address);
                        if (ret == PIFS_ERROR_NO_MORE_SPACE)
                        {
                            /* If entry cannot be appended, merge the blocks */
                            ret = pifs_merge();
                            if (ret == PIFS_SUCCESS)
                            {
                                ret = pifs_append_entry(a_entry,
                                                        a_entry_list_block_address,
                                                        a_entry_list_page_address);
                            }
                            if (ret == PIFS_SUCCESS)
                            {
                                PIFS_NOTICE_MSG("Entry appended\r\n");
                            }
                        }
                        else
                        {
                            PIFS_NOTICE_MSG("Entry appended\r\n");
                        }
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
                    if (a_entry_cmd == PIFS_DELETE_ENTRY)
                    {
                        pifs_mark_entry_deleted(&entry);
                    }
                    else
                    {
                        memset(&entry, PIFS_FLASH_PROGRAMMED_BYTE_VALUE, PIFS_ENTRY_SIZE_BYTE);
                    }
                    ret = pifs_write_entry(ba, pa, i, &entry);
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
 * PIFS_ERROR_NO_MORE_SPACE if entry list is full.
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
