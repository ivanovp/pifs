/**
 * @file        pifs_delta.c
 * @brief       Pi file system: delta page funtions
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

#define PIFS_DEBUG_LEVEL    2

#include "pifs_debug.h"

/**
 * @brief pifs_read_delta_map_page Read delta map pages to memory buffer.
 *
 * @return PIFS_SUCCES if read successfully.
 */
static pifs_status_t pifs_read_delta_map_page(void)
{
    pifs_block_address_t ba = pifs.header.delta_map_address.block_address;
    pifs_page_address_t  pa = pifs.header.delta_map_address.page_address;
    pifs_size_t          i;
    pifs_status_t        ret = PIFS_SUCCESS;

    for (i = 0; i < PIFS_DELTA_MAP_PAGE_NUM && ret == PIFS_SUCCESS; i++)
    {
        ret = pifs_read(ba, pa, 0, &pifs.delta_map_page_buf[i], PIFS_FLASH_PAGE_SIZE_BYTE);
        if (ret == PIFS_SUCCESS)
        {
            ret = pifs_inc_ba_pa(&ba, &pa);
        }
    }

    return ret;
}

/**
 * @brief pifs_write_delta_map_page Write delta map pages to flash memory.
 *
 * @return PIFS_SUCCES if written successfully.
 */
static pifs_status_t pifs_write_delta_map_page(pifs_size_t delta_map_page_idx)
{
    pifs_block_address_t ba = pifs.header.delta_map_address.block_address;
    pifs_page_address_t  pa = pifs.header.delta_map_address.page_address;
    pifs_size_t          i;
    pifs_status_t        ret = PIFS_SUCCESS;

    /* TODO calculate delta page address instead of increasing */
    for (i = 0; i < PIFS_DELTA_MAP_PAGE_NUM && ret == PIFS_SUCCESS; i++)
    {
        if (i == delta_map_page_idx)
        {
            ret = pifs_write(ba, pa, 0, &pifs.delta_map_page_buf[i], PIFS_FLASH_PAGE_SIZE_BYTE);
            PIFS_DEBUG_MSG("%s ret: %i\r\n", pifs_ba_pa2str(ba, pa), ret);
            break;
        }
        if (ret == PIFS_SUCCESS)
        {
            ret = pifs_inc_ba_pa(&ba, &pa);
        }
    }

    return ret;
}

/**
 * @brief pifs_find_delta_page Look for a delta page of a specified page.
 * a_delta_block_address and a_delta_page_address will contain the
 * address of delta page if found or the original address (a_block_address,
 * a_page_address) if not found.
 *
 * @param[in] a_block_address           Block address to search.
 * @param[in] a_page_address            Page address to search.
 * @param[out] a_delta_block_address    Pointer to block address to fill.
 * @param[out] a_delta_page_address     Pointer to page address to fill.
 * @param[out] a_is_map_full            TRUE: if delta map is full.
 *                                      FALSE: there is space in delta map.
 * @return PIFS_SUCCESS: if delta map read successfully.
 */
pifs_status_t pifs_find_delta_page(pifs_block_address_t a_block_address,
                                   pifs_page_address_t a_page_address,
                                   pifs_block_address_t * a_delta_block_address,
                                   pifs_page_address_t * a_delta_page_address,
                                   bool_t * a_is_map_full)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_size_t          i;
    pifs_size_t          j;
    pifs_delta_entry_t * delta_entry;
    pifs_block_address_t ba = a_block_address;
    pifs_page_address_t  pa = a_page_address;

    PIFS_ASSERT(a_block_address < PIFS_BLOCK_ADDRESS_INVALID);
    PIFS_ASSERT(a_page_address < PIFS_PAGE_ADDRESS_INVALID);

    if (!pifs.delta_map_page_is_read)
    {
        ret = pifs_read_delta_map_page();
    }
    if (ret == PIFS_SUCCESS)
    {
        if (a_is_map_full)
        {
            *a_is_map_full = TRUE;
        }
        /* All delta pages shall be checked to find latest delta page! */
        for (i = 0; i < PIFS_DELTA_MAP_PAGE_NUM; i++)
        {
            delta_entry = (pifs_delta_entry_t*) &pifs.delta_map_page_buf[i];
            for (j = 0; j < PIFS_DELTA_ENTRY_PER_PAGE; j++)
            {
                if (delta_entry[j].orig_address.block_address == a_block_address
                        && delta_entry[j].orig_address.page_address == a_page_address)
                {
                    ba = delta_entry[j].delta_address.block_address;
                    pa = delta_entry[j].delta_address.page_address;
                    PIFS_DEBUG_MSG("delta found %s -> ",
                                   pifs_ba_pa2str(a_block_address, a_page_address));
                    PIFS_DEBUG_MSG("%s\r\n",
                                   pifs_ba_pa2str(ba, pa));
                }
                if (a_is_map_full && *a_is_map_full
                        && pifs_is_buffer_erased(&delta_entry[j], sizeof(PIFS_DELTA_ENTRY_SIZE_BYTE)))
                {
                    *a_is_map_full = FALSE;
                }
            }
        }
        *a_delta_block_address = ba;
        *a_delta_page_address = pa;
    }

    return ret;
}

/**
 * @brief pifs_append_delta_map_entry Add an entry to the delta map.
 *
 * @param[in] a_new_delta_entry Pointer to the new entry.
 * @return PIFS_SUCCESS if entry was added. PIFS_ERROR_NO_MORE_SPACE if map
 * is full.
 */
static pifs_status_t pifs_append_delta_map_entry(pifs_delta_entry_t * a_new_delta_entry)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_size_t          i;
    pifs_size_t          j;
    pifs_delta_entry_t * delta_entry;
    bool_t               delta_written = FALSE;

    if (!pifs.delta_map_page_is_read)
    {
        ret = pifs_read_delta_map_page();
    }
    if (ret == PIFS_SUCCESS)
    {
        for (i = 0; i < PIFS_DELTA_MAP_PAGE_NUM && !delta_written && ret == PIFS_SUCCESS; i++)
        {
            delta_entry = (pifs_delta_entry_t*) &pifs.delta_map_page_buf[i];
            for (j = 0; j < PIFS_DELTA_ENTRY_PER_PAGE && !delta_written && ret == PIFS_SUCCESS; j++)
            {
                if (pifs_is_buffer_erased(&delta_entry[j], PIFS_DELTA_ENTRY_SIZE_BYTE))
                {
                    delta_entry[j] = *a_new_delta_entry;
                    ret = pifs_write_delta_map_page(i);
                    delta_written = TRUE;
                }
            }
        }
        if (!delta_written)
        {
            ret = PIFS_ERROR_NO_MORE_SPACE;
        }
    }

    return ret;
}

/**
 * @brief pifs_read_delta  Cached read with delta page handling.
 *
 * @param[in] a_block_address   Block address of page to read.
 * @param[in] a_page_address    Page address of page to read.
 * @param[in] a_page_offset     Offset in page.
 * @param[out] a_buf            Pointer to buffer to fill or NULL if
 *                              pifs.cache_page_buf is used.
 * @param[in] a_buf_size        Size of buffer. Ignored if a_buf is NULL.
 * @return PIFS_SUCCESS if data read successfully.
 */
pifs_status_t pifs_read_delta(pifs_block_address_t a_block_address,
                              pifs_page_address_t a_page_address,
                              pifs_page_offset_t a_page_offset,
                              void * const a_buf,
                              pifs_size_t a_buf_size)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba;
    pifs_page_address_t  pa;

    ret = pifs_find_delta_page(a_block_address, a_page_address, &ba, &pa, NULL);
    if (ret == PIFS_SUCCESS)
    {
//        PIFS_DEBUG_MSG("%s\r\n", pifs_ba_pa2str(ba, pa));
        ret = pifs_read(ba, pa, a_page_offset, a_buf, a_buf_size);
    }

    return ret;
}

/**
 * @brief pifs_write  Cached write with delta page handling.
 * Note: marks written page as used!
 *
 * @param[in]a_block_address   Block address of page to write.
 * @param[in]a_page_address    Page address of page to write.
 * @param[in]a_page_offset     Offset in page.
 * @param[in]a_buf             Pointer to buffer to write or NULL if
 *                             pifs.cache_page_buf is directly written.
 * @param[in]a_buf_size        Size of buffer. Ignored if a_buf is NULL.
 * @param[out]a_is_delta       TRUE: Delta page was written. FALSE: Normal page was written.
 * @return PIFS_SUCCESS if data write successfully.
 */
pifs_status_t pifs_write_delta(pifs_block_address_t a_block_address,
                               pifs_page_address_t a_page_address,
                               pifs_page_offset_t a_page_offset,
                               const void * const a_buf,
                               pifs_size_t a_buf_size,
                               bool_t * a_is_delta)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_delta_entry_t   delta_entry;
    bool_t               delta_needed = FALSE;
    pifs_block_address_t ba;
    pifs_page_address_t  pa;
    pifs_block_address_t fba;
    pifs_page_address_t  fpa;
    pifs_page_count_t    page_count_found;
    bool_t               is_delta_map_full;

    ret = pifs_find_delta_page(a_block_address, a_page_address, &ba, &pa, &is_delta_map_full);
    if (ret == PIFS_SUCCESS)
    {
        /* Read to page buffer */
        ret = pifs_read(ba, pa, 0, &pifs.page_buf, PIFS_FLASH_PAGE_SIZE_BYTE);
    }
    if (ret == PIFS_SUCCESS)
    {
        delta_needed = !pifs_is_buffer_programmable(&pifs.page_buf[a_page_offset],
                                                    a_buf, a_buf_size);
    }
    if (ret == PIFS_SUCCESS)
    {
        if (delta_needed)
        {
            /* Find a new data page */
            if (a_is_delta)
            {
                *a_is_delta = TRUE;
            }
            if (is_delta_map_full)
            {
                PIFS_WARNING_MSG("Management blocks shall be merged!\r\n");
                ret = pifs_merge();
            }
            if (ret == PIFS_SUCCESS)
            {
                ret = pifs_find_free_page(1, PIFS_BLOCK_TYPE_DATA,
                                          &fba, &fpa, &page_count_found);
            }
            if (ret == PIFS_SUCCESS)
            {
                PIFS_DEBUG_MSG("free page %s\r\n", pifs_ba_pa2str(fba, fpa));

                delta_entry.orig_address.block_address = a_block_address;
                delta_entry.orig_address.page_address = a_page_address;
                delta_entry.delta_address.block_address = fba;
                delta_entry.delta_address.page_address = fpa;
                PIFS_DEBUG_MSG("delta page %s -> ",
                               pifs_ba_pa2str(a_block_address, a_page_address));
                PIFS_DEBUG_MSG("%s\r\n",
                               pifs_ba_pa2str(fba, fpa));
                ret = pifs_write(fba, fpa, a_page_offset, a_buf, PIFS_FLASH_PAGE_SIZE_BYTE);
                if (ret == PIFS_SUCCESS)
                {
                    ret = pifs_append_delta_map_entry(&delta_entry);
                }
                if (ret == PIFS_SUCCESS)
                {
                    /* Mark new page as used */
                    ret = pifs_mark_page(fba, fpa, 1, TRUE);
                    PIFS_DEBUG_MSG("Mark page %s as used: %i\r\n", pifs_ba_pa2str(fba, fpa), ret);
                }
                if (ret == PIFS_SUCCESS)
                {
                    /* Mark old page (original or previous delta)
                     * as to be released */
                    ret = pifs_mark_page(ba, pa, 1, FALSE);
                    PIFS_DEBUG_MSG("Mark page %s as to be released: %i\r\n", pifs_ba_pa2str(a_block_address, a_page_address), ret);
                }
            }
        }
        else
        {
            /* No delta page needed, simple write */
            if (a_is_delta)
            {
                *a_is_delta = FALSE;
            }
            ret = pifs_write(ba, pa, a_page_offset, a_buf, a_buf_size);
            if (ret == PIFS_SUCCESS && pifs_is_page_free(ba, pa))
            {
                /* Mark new page as used */
                ret = pifs_mark_page(ba, pa, 1, TRUE);
            }
        }
    }

    return ret;
}
