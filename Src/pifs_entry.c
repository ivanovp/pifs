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

/**
 * @brief pifs_append_entry Add an item to the entry list.
 *
 * @param a_entry[in] Pointer to the entry to be added.
 * @return PIFS_SUCCESS if entry successfully added.
 * PIFS_ERROR_NO_MORE_SPACE if entry list is full.
 * PIFS_ERROR_FLASH_WRITE if flash write failed.
 */
pifs_status_t pifs_append_entry(pifs_entry_t * a_entry,
                                pifs_block_address_t a_list_entry_block_address,
                                pifs_page_address_t a_list_entry_page_address)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba = a_list_entry_block_address;
    pifs_page_address_t  pa = a_list_entry_page_address;
    bool_t               created = FALSE;
    pifs_entry_t         entry;
    pifs_size_t          i;
    pifs_size_t          j;

#if PIFS_ENABLE_ATTRIBUTES && PIFS_INVERT_ATTRIBUTE_BITS
    /* Invert attribute bits */
    a_entry->attrib ^= PIFS_ATTRIB_ALL;
#endif
    for (j = 0; j < PIFS_ENTRY_LIST_SIZE_PAGE && !created && ret == PIFS_SUCCESS; j++)
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
        ret = pifs_inc_ba_pa(&ba, &pa);
    }
    if (!created)
    {
        ret = PIFS_ERROR_NO_MORE_SPACE;
    }
#if PIFS_ENABLE_ATTRIBUTES && PIFS_INVERT_ATTRIBUTE_BITS
    /* Restore attribute bits */
    a_entry->attrib ^= PIFS_ATTRIB_ALL;
#endif

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
                                pifs_block_address_t a_list_entry_block_address,
                                pifs_page_address_t a_list_entry_page_address)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba = a_list_entry_block_address;
    pifs_page_address_t  pa = a_list_entry_page_address;
    bool_t               found = FALSE;
    pifs_entry_t         entry;
    pifs_size_t          i;
    pifs_size_t          j;

    for (j = 0; j < PIFS_ENTRY_LIST_SIZE_PAGE && !found && ret == PIFS_SUCCESS; j++)
    {
        for (i = 0; i < PIFS_ENTRY_PER_PAGE && !found && ret == PIFS_SUCCESS; i++)
        {
#if PIFS_USE_DELTA_FOR_ENTRIES
            ret = pifs_read_delta(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, &entry,
                                  PIFS_ENTRY_SIZE_BYTE);
#else
            ret = pifs_read(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, &entry,
                            PIFS_ENTRY_SIZE_BYTE);
#endif
            /* Check if name matches and not deleted */
            if ((strncmp((char*)entry.name, a_name, sizeof(entry.name)) == 0)
                    && !pifs_is_entry_deleted(&entry))
            {
                /* Entry found */
                /* Copy entry */
#if PIFS_USE_DELTA_FOR_ENTRIES
#if PIFS_ENABLE_ATTRIBUTES && PIFS_INVERT_ATTRIBUTE_BITS
                /* Invert entry bits as it is stored inverted */
                a_entry->attrib ^= PIFS_ATTRIB_ALL;
#endif
                ret = pifs_write_delta(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, a_entry,
                                       PIFS_ENTRY_SIZE_BYTE, NULL);
#if PIFS_ENABLE_ATTRIBUTES && PIFS_INVERT_ATTRIBUTE_BITS
                /* Restore entry bits */
                a_entry->attrib ^= PIFS_ATTRIB_ALL;
#endif
#else
#if PIFS_ENABLE_ATTRIBUTES && PIFS_INVERT_ATTRIBUTE_BITS
                /* Invert entry bits as it is stored inverted */
                a_entry->attrib ^= PIFS_ATTRIB_ALL;
#endif
                if (pifs_is_buffer_programmable(&entry, a_entry, PIFS_ENTRY_SIZE_BYTE))
                {
                    PIFS_NOTICE_MSG("Entry can be updated\r\n");
                    ret = pifs_write(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, a_entry,
                                     PIFS_ENTRY_SIZE_BYTE);
#if PIFS_ENABLE_ATTRIBUTES && PIFS_INVERT_ATTRIBUTE_BITS
                    /* Restore entry bits */
                    a_entry->attrib ^= PIFS_ATTRIB_ALL;
#endif
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
                    /* Clear entry */
                    pifs_mark_entry_deleted(&entry);
                    ret = pifs_write(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, &entry,
                                     PIFS_ENTRY_SIZE_BYTE);
                    if (ret == PIFS_SUCCESS)
                    {
#if PIFS_ENABLE_ATTRIBUTES && PIFS_INVERT_ATTRIBUTE_BITS
                        /* Restore entry bits */
                        a_entry->attrib ^= PIFS_ATTRIB_ALL;
#endif
                        ret = pifs_append_entry(a_entry,
                                                a_list_entry_block_address,
                                                a_list_entry_page_address);
                        if (ret == PIFS_ERROR_NO_MORE_SPACE)
                        {
                            /* If entry cannot be appended, merge the blocks */
                            ret = pifs_merge();
                            if (ret == PIFS_SUCCESS)
                            {
                                ret = pifs_append_entry(a_entry,
                                                        a_list_entry_block_address,
                                                        a_list_entry_page_address);
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
    //memset(a_entry, PIFS_FLASH_PROGRAMMED_BYTE_VALUE, sizeof(pifs_entry_t));
    /* Change only the first character to programmed value */
    /* This way file system check can find it and take into account */
#if PIFS_ENABLE_ATTRIBUTES
    a_entry->attrib ^= PIFS_ATTRIB_DELETED;
#else
    a_entry->name[0] = PIFS_FLASH_PROGRAMMED_BYTE_VALUE;
#endif
}

/**
 * @brief pifs_is_entry_deleted Check if entry is deleted.
 *
 * @param[in] a_entry Entry to be checked.
 * @return TRUE: entry is deleted, FALSE: entry exist.
 */
bool_t pifs_is_entry_deleted(pifs_entry_t * a_entry)
{
    bool_t is_deleted = FALSE;

#if PIFS_ENABLE_ATTRIBUTES
    if (!(a_entry->attrib & PIFS_ATTRIB_DELETED))
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
 * @param a_name[in]    Pointer to name to find.
 * @param a_entry[out]  Pointer to entry to fill. NULL: clear entry.
 * @return PIFS_SUCCESS if entry found.
 * PIFS_ERROR_FILE_NOT_FOUND if entry not found.
 */
pifs_status_t pifs_find_entry(const pifs_char_t * a_name, pifs_entry_t * const a_entry,
                              pifs_block_address_t a_list_entry_block_address,
                              pifs_page_address_t a_list_entry_page_address)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba = a_list_entry_block_address;
    pifs_page_address_t  pa = a_list_entry_page_address;
    bool_t               found = FALSE;
    pifs_entry_t         entry;
    pifs_size_t          i;
    pifs_size_t          j;

    for (j = 0; j < PIFS_ENTRY_LIST_SIZE_PAGE && !found && ret == PIFS_SUCCESS; j++)
    {
        for (i = 0; i < PIFS_ENTRY_PER_PAGE && !found && ret == PIFS_SUCCESS; i++)
        {
#if PIFS_USE_DELTA_FOR_ENTRIES
            ret = pifs_read_delta(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, &entry,
                                  PIFS_ENTRY_SIZE_BYTE);
#else
            ret = pifs_read(ba, pa, i * PIFS_ENTRY_SIZE_BYTE, &entry,
                            PIFS_ENTRY_SIZE_BYTE);
#endif
            /* Check if name matches and not deleted */
            if ((strncmp((char*)entry.name, a_name, sizeof(entry.name)) == 0)
                    && !pifs_is_entry_deleted(&entry))
            {
                /* Entry found */
                if (a_entry)
                {
                    /* Copy entry */
                    memcpy(a_entry, &entry, sizeof(pifs_entry_t));
#if PIFS_ENABLE_ATTRIBUTES && PIFS_INVERT_ATTRIBUTE_BITS
                    /* Invert entry bits as it is stored inverted */
                    a_entry->attrib ^= PIFS_ATTRIB_ALL;
#endif
                    PIFS_DEBUG_MSG("file size: %i bytes\r\n", a_entry->file_size);
                }
                else
                {
                    /* Clear entry */
                    pifs_mark_entry_deleted(&entry);
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
        ret = pifs_inc_ba_pa(&ba, &pa);
    }

    if (ret == PIFS_SUCCESS && !found)
    {
        ret = PIFS_ERROR_FILE_NOT_FOUND;
    }

    return ret;
}

/**
 * @brief pifs_clear_entry Find entry in entry list and invalidate it.
 * This function shoudl be used only when a file is removed.
 *
 * @param a_name[in]    Pointer to name to find.
 * @return PIFS_SUCCESS if entry found and cleared.
 * PIFS_ERROR_FILE_NOT_FOUND if entry not found.
 */
pifs_status_t pifs_clear_entry(const pifs_char_t * a_name,
                               pifs_block_address_t a_list_entry_block_address,
                               pifs_page_address_t a_list_entry_page_address)
{
    return pifs_find_entry(a_name, NULL,
                           a_list_entry_block_address,
                           a_list_entry_page_address);
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
                                pifs_block_address_t a_list_entry_block_address,
                                pifs_page_address_t a_list_entry_page_address)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba = a_list_entry_block_address;
    pifs_page_address_t  pa = a_list_entry_page_address;
    pifs_entry_t         entry;
    pifs_size_t          i;
    pifs_size_t          j;
    pifs_size_t          free_entry_count = 0;
    pifs_size_t          to_be_released_entry_count = 0;

    for (j = 0; j < PIFS_ENTRY_LIST_SIZE_PAGE && ret == PIFS_SUCCESS; j++)
    {
        for (i = 0; i < PIFS_ENTRY_PER_PAGE && ret == PIFS_SUCCESS; i++)
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
                free_entry_count++;
            }
            if (pifs_is_buffer_programmed(&entry, sizeof(entry)))
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
