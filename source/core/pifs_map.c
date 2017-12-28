/**
 * @file        pifs_map.c
 * @brief       Pi file system: file map handling functions
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-11-30 17:43:15 ivanovp {Time-stamp}
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

#define PIFS_DEBUG_LEVEL 2
#include "pifs_debug.h"

#include "api_pifs.h"
#include "flash.h"
#include "flash_config.h"
#include "pifs.h"
#include "pifs_fsbm.h"
#include "pifs_helper.h"
#include "pifs_delta.h"
#include "pifs_map.h"

/**
 * @brief pifs_read_first_map_entry Read first map's first map entry.
 *
 * @param[in] a_file Pointer to opened file.
 * @return PIFS_SUCCESS if entry is read and valid.
 */
pifs_status_t pifs_read_first_map_entry(pifs_file_t * a_file)
{
    pifs_checksum_t checksum;
    bool_t          is_erased;

    a_file->map_entry_idx = 0;
    a_file->actual_map_address = a_file->entry.first_map_address;
    PIFS_DEBUG_MSG("Map address %s\r\n",
                   pifs_address2str(&a_file->actual_map_address));
    a_file->status = pifs_read(a_file->entry.first_map_address.block_address,
                             a_file->entry.first_map_address.page_address,
                             0, &a_file->map_header, PIFS_MAP_HEADER_SIZE_BYTE);
    if (a_file->status == PIFS_SUCCESS)
    {
        a_file->status = pifs_read(a_file->entry.first_map_address.block_address,
                                 a_file->entry.first_map_address.page_address,
                                 PIFS_MAP_HEADER_SIZE_BYTE, &a_file->map_entry,
                                 PIFS_MAP_ENTRY_SIZE_BYTE);
    }
    if (a_file->status == PIFS_SUCCESS)
    {
        is_erased = pifs_is_buffer_erased(&a_file->map_entry, PIFS_MAP_ENTRY_SIZE_BYTE);
        if (!is_erased)
        {
            checksum = pifs_calc_checksum(&a_file->map_entry,
                                          PIFS_MAP_ENTRY_SIZE_BYTE - PIFS_CHECKSUM_SIZE_BYTE);
            if (checksum != a_file->map_entry.checksum)
            {
                a_file->status = PIFS_ERROR_CHECKSUM;
            }
        }
    }
    if (a_file->status == PIFS_SUCCESS)
    {
        PIFS_DEBUG_MSG("Map entry %s, page count: %i\r\n",
                       pifs_address2str(&a_file->map_entry.address),
                       a_file->map_entry.page_count);
    }

    return a_file->status;
}

/**
 * @brief pifs_read_next_map_entry Read next map entry.
 * pifs_read_first_map_entry() shall be called before calling this function!
 *
 * @param[in] a_file Pointer to opened file.
 * @return PIFS_SUCCESS if entry is read and valid.
 * PIFS_ERROR_END_OF_FILE if end of file reached.
 */
pifs_status_t pifs_read_next_map_entry(pifs_file_t * a_file)
{
    pifs_checksum_t checksum;
    bool_t          is_erased;

    a_file->map_entry_idx++;
    if (a_file->map_entry_idx >= PIFS_MAP_ENTRY_PER_PAGE)
    {
        a_file->map_entry_idx = 0;
        checksum = pifs_calc_checksum(&a_file->map_header.next_map_address,
                                      PIFS_ADDRESS_SIZE_BYTE);
        if (a_file->map_header.next_map_address.block_address < PIFS_BLOCK_ADDRESS_INVALID
                && a_file->map_header.next_map_address.page_address < PIFS_PAGE_ADDRESS_INVALID)
        {
            if (checksum == a_file->map_header.next_map_checksum)
            {
                a_file->actual_map_address = a_file->map_header.next_map_address;
                //            PIFS_DEBUG_MSG("Next map address %s\r\n",
                //                           pifs_address2str(&a_file->actual_map_address));
                a_file->status = pifs_read(a_file->actual_map_address.block_address,
                                           a_file->actual_map_address.page_address,
                                           0, &a_file->map_header, PIFS_MAP_HEADER_SIZE_BYTE);
            }
            else
            {
                a_file->status = PIFS_ERROR_CHECKSUM;
            }
        }
        else
        {
            a_file->status = PIFS_ERROR_END_OF_FILE;
        }
    }
    if (a_file->status == PIFS_SUCCESS)
    {
        PIFS_ASSERT(pifs_is_address_valid(&a_file->actual_map_address));
        a_file->status = pifs_read(a_file->actual_map_address.block_address,
                                   a_file->actual_map_address.page_address,
                                   PIFS_MAP_HEADER_SIZE_BYTE + a_file->map_entry_idx * PIFS_MAP_ENTRY_SIZE_BYTE,
                                   &a_file->map_entry,
                                   PIFS_MAP_ENTRY_SIZE_BYTE);
    }
    if (a_file->status == PIFS_SUCCESS)
    {
        is_erased = pifs_is_buffer_erased(&a_file->map_entry, PIFS_MAP_ENTRY_SIZE_BYTE);
        if (!is_erased)
        {
            checksum = pifs_calc_checksum(&a_file->map_entry,
                                          PIFS_MAP_ENTRY_SIZE_BYTE - PIFS_CHECKSUM_SIZE_BYTE);
            if (checksum != a_file->map_entry.checksum)
            {
                a_file->status = PIFS_ERROR_CHECKSUM;
            }
        }
    }
    if (a_file->status == PIFS_SUCCESS)
    {
        PIFS_DEBUG_MSG("Map entry %s, page count: %i\r\n",
                       pifs_address2str(&a_file->map_entry.address),
                       a_file->map_entry.page_count);
    }

    return a_file->status;
}

/**
 * @brief pifs_is_free_map_entry Check if free map entry exists in the actual
 * map.
 *
 * @param[in] a_file               Pointer to file to use.
 * @param[out] a_is_free_map_entry TRUE: free map entry exists.
 * @return PIFS_SUCCESS if entry was successfully written.
 */
pifs_status_t pifs_is_free_map_entry(pifs_file_t * a_file,
                                     bool_t * a_is_free_map_entry)
{
    pifs_block_address_t    ba = a_file->actual_map_address.block_address;
    pifs_page_address_t     pa = a_file->actual_map_address.page_address;
    bool_t                  empty_entry_found = FALSE;
    pifs_map_entry_t        map_entry;
    pifs_size_t             i;

    PIFS_DEBUG_MSG("Actual map address %s\r\n",
                   pifs_address2str(&a_file->actual_map_address));
    for (i = 0; i < PIFS_MAP_ENTRY_PER_PAGE && !empty_entry_found && a_file->status == PIFS_SUCCESS; i++)
    {
        a_file->status = pifs_read(ba, pa, i * PIFS_MAP_ENTRY_SIZE_BYTE,
                                   &map_entry, PIFS_MAP_ENTRY_SIZE_BYTE);
        if (pifs_is_buffer_erased(&map_entry, PIFS_MAP_ENTRY_SIZE_BYTE))
        {
            empty_entry_found = TRUE;
        }
    }
    PIFS_DEBUG_MSG("Empty entry found: %i\r\n", empty_entry_found);
    *a_is_free_map_entry = empty_entry_found;

    return a_file->status;
}

/**
 * @brief pifs_append_map_entry Add an entry to the file's map.
 * This function is called when file is growing and new space is needed.
 *
 * @param[in] a_file            Pointer to file to use.
 * @param[in] a_block_address   Block address of new file pages to added.
 * @param[in] a_page_address    Page address of new file pages to added.
 * @param[in] a_page_count      Number of new file pages.
 * @return PIFS_SUCCESS if entry was successfully written.
 */
pifs_status_t pifs_append_map_entry(pifs_file_t * a_file,
                                    pifs_block_address_t a_block_address,
                                    pifs_page_address_t a_page_address,
                                    pifs_page_count_t a_page_count)
{
    pifs_block_address_t    ba = a_file->actual_map_address.block_address;
    pifs_page_address_t     pa = a_file->actual_map_address.page_address;
    bool_t                  empty_entry_found = FALSE;
    pifs_page_count_t       page_count_found = 0;

    PIFS_NOTICE_MSG("Actual map address %s\r\n",
                    pifs_address2str(&a_file->actual_map_address));
    PIFS_ASSERT(pifs_is_address_valid(&a_file->actual_map_address));
    do
    {
        if (pifs_is_buffer_erased(&a_file->map_entry, PIFS_MAP_ENTRY_SIZE_BYTE))
        {
            empty_entry_found = TRUE;
        }
        else
        {
            a_file->status = pifs_read_next_map_entry(a_file);
        }
    } while (!empty_entry_found && a_file->status == PIFS_SUCCESS);
    if (a_file->status == PIFS_ERROR_END_OF_FILE) // || a_file->status == PIFS_ERROR_CHECKSUM)
    {
        PIFS_DEBUG_MSG("End of map, new map will be created\r\n");
        a_file->status = pifs_find_free_page_wl(PIFS_MAP_PAGE_NUM, PIFS_MAP_PAGE_NUM,
                                                PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT,
                                                &ba, &pa, &page_count_found);
        if (a_file->status == PIFS_SUCCESS)
        {
            a_file->status = pifs_read(a_file->actual_map_address.block_address,
                                       a_file->actual_map_address.page_address,
                                       0,
                                       &a_file->map_header,
                                       PIFS_MAP_HEADER_SIZE_BYTE);
        }
        if (a_file->status == PIFS_SUCCESS)
        {
            /* Set jump address for last map */
            a_file->map_header.next_map_address.block_address = ba;
            a_file->map_header.next_map_address.page_address = pa;
            a_file->map_header.next_map_checksum = pifs_calc_checksum(&a_file->map_header.next_map_address,
                                                                      PIFS_ADDRESS_SIZE_BYTE);
            a_file->status = pifs_write(a_file->actual_map_address.block_address,
                                        a_file->actual_map_address.page_address,
                                        0,
                                        &a_file->map_header,
                                        PIFS_MAP_HEADER_SIZE_BYTE);
            PIFS_DEBUG_MSG("### Map full, set jump address %s ###\r\n",
                           pifs_address2str(&a_file->actual_map_address));
//            pifs_print_cache();
        }
        if (a_file->status == PIFS_SUCCESS)
        {
            /* Set jump address to previous map */
            memset(&a_file->map_header, PIFS_FLASH_ERASED_BYTE_VALUE, PIFS_MAP_HEADER_SIZE_BYTE);
            a_file->map_header.prev_map_address.block_address = a_file->actual_map_address.block_address;
            a_file->map_header.prev_map_address.page_address = a_file->actual_map_address.page_address;
            a_file->map_header.prev_map_checksum = pifs_calc_checksum(&a_file->map_header.prev_map_address,
                                                                      PIFS_ADDRESS_SIZE_BYTE);
            /* Update actual map's address */
            a_file->actual_map_address.block_address = ba;
            a_file->actual_map_address.page_address = pa;
            a_file->status = pifs_write(ba, pa, 0, &a_file->map_header,
                                        PIFS_MAP_HEADER_SIZE_BYTE);
            PIFS_DEBUG_MSG("### New map %s ###\r\n",
                           pifs_address2str(&a_file->actual_map_address));
//            pifs_print_cache();
            a_file->map_entry_idx = 0;
        }
        if (a_file->status == PIFS_SUCCESS)
        {
            PIFS_DEBUG_MSG("### Mark page %s ###\r\n",
                           pifs_address2str(&a_file->actual_map_address));
            a_file->status = pifs_mark_page(ba, pa, PIFS_MAP_PAGE_NUM, TRUE, FALSE);
            empty_entry_found = TRUE;
        }
    }
    if (empty_entry_found)
    {
        /* Empty map entry found */
        a_file->map_entry.address.block_address = a_block_address;
        a_file->map_entry.address.page_address = a_page_address;
        a_file->map_entry.page_count = a_page_count;
        a_file->map_entry.checksum = pifs_calc_checksum(&a_file->map_entry,
                                                        PIFS_MAP_ENTRY_SIZE_BYTE - PIFS_CHECKSUM_SIZE_BYTE);
        PIFS_DEBUG_MSG("Create map entry #%lu for %s\r\n", a_file->map_entry_idx,
                       pifs_ba_pa2str(a_block_address, a_page_address));
        a_file->status = pifs_write(ba, pa, PIFS_MAP_HEADER_SIZE_BYTE
                                    + a_file->map_entry_idx * PIFS_MAP_ENTRY_SIZE_BYTE,
                                    &a_file->map_entry,
                                    PIFS_MAP_ENTRY_SIZE_BYTE);
        PIFS_DEBUG_MSG("### New map entry %s ###\r\n",
                       pifs_ba_pa2str(ba, pa));
//        pifs_print_cache();
    }
    else
    {
        a_file->status = PIFS_ERROR_NO_MORE_SPACE;
        PIFS_ERROR_MSG("Cannot create new map!\r\n");
    }

    return a_file->status;
}

/**
 * @brief pifs_release_file_pages Mark file map and file's pages to be released.
 *
 * @param[in] a_file             Pointer of file structure.
 * @param[in] a_file_walker_func Function to call when map or page found.
 * @param[in] a_func_data        User-data to pass to a_file_walker_func.
 * @return TRUE: if all pages were succesfully marked to be released.
 */
pifs_status_t pifs_walk_file_pages(pifs_file_t * a_file, 
                                   pifs_file_walker_func_t a_file_walker_func,
                                   void * a_func_data)
{
    pifs_block_address_t    ba = a_file->entry.first_map_address.block_address;
    pifs_page_address_t     pa = a_file->entry.first_map_address.page_address;
    pifs_block_address_t    mba;
    pifs_page_address_t     mpa;
    pifs_block_address_t    delta_ba;
    pifs_page_address_t     delta_pa;
    pifs_page_count_t       page_count;
    pifs_size_t             i;
    bool_t                  erased = FALSE;
    pifs_page_offset_t      po = PIFS_MAP_HEADER_SIZE_BYTE;
    pifs_checksum_t         checksum;
    bool_t                  end = FALSE;

    PIFS_ASSERT(a_file_walker_func);
    PIFS_DEBUG_MSG("Searching in map entry at %s\r\n", pifs_ba_pa2str(ba, pa));

    do
    {
        a_file->status = pifs_read(ba, pa, 0, &a_file->map_header, PIFS_MAP_HEADER_SIZE_BYTE);

        if (a_file->status == PIFS_SUCCESS)
        {
            if (pifs_is_buffer_erased(&a_file->map_header, PIFS_MAP_HEADER_SIZE_BYTE))
            {
                PIFS_DEBUG_MSG("map header is empty!\r\n");
            }
            for (i = 0; i < PIFS_MAP_ENTRY_PER_PAGE && !erased && a_file->status == PIFS_SUCCESS; i++)
            {
                a_file->status = pifs_read(ba, pa, po, &a_file->map_entry, PIFS_MAP_ENTRY_SIZE_BYTE);
                if (pifs_is_buffer_erased(&a_file->map_entry, PIFS_MAP_ENTRY_SIZE_BYTE))
                {
                    erased = TRUE;
                }
                else
                {
                    checksum = pifs_calc_checksum(&a_file->map_entry,
                                                  PIFS_MAP_ENTRY_SIZE_BYTE - PIFS_CHECKSUM_SIZE_BYTE);
                    if (checksum == a_file->map_entry.checksum)
                    {
                        /* Map entry found */
                        mba = a_file->map_entry.address.block_address;
                        mpa = a_file->map_entry.address.page_address;
                        page_count = a_file->map_entry.page_count;
                        PIFS_DEBUG_MSG("Map entry %s, page count: %i\r\n",
                                       pifs_ba_pa2str(mba, mpa),
                                       page_count);
                        if (page_count && page_count < PIFS_MAP_PAGE_COUNT_INVALID)
                        {
                            a_file->status = pifs_find_delta_page(mba, mpa,
                                                                  &delta_ba, &delta_pa, NULL,
                                                                  &pifs.header);
                            while (page_count-- && a_file->status == PIFS_SUCCESS)
                            {
                                /* Call callback function */
                                a_file->status = (*a_file_walker_func)(a_file, mba, mpa,
                                                                       delta_ba, delta_pa,
                                                                       FALSE, a_func_data);
                                if (page_count)
                                {
                                    if (a_file->status == PIFS_SUCCESS)
                                    {
                                        a_file->status = pifs_inc_ba_pa(&delta_ba, &delta_pa);
                                    }
                                    if (a_file->status == PIFS_SUCCESS)
                                    {
                                        a_file->status = pifs_inc_ba_pa(&mba, &mpa);
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        a_file->status = PIFS_ERROR_CHECKSUM;
                    }
                }
                po += PIFS_MAP_ENTRY_SIZE_BYTE;
            }
        }
        if (a_file->status == PIFS_SUCCESS)
        {
            /* Mark map page to be released */
            a_file->status = (*a_file_walker_func)(a_file, ba, pa,
                                                   PIFS_BLOCK_ADDRESS_INVALID,
                                                   PIFS_PAGE_ADDRESS_INVALID,
                                                   TRUE, a_func_data);
            if (!pifs_is_buffer_erased(&a_file->map_header.next_map_address, PIFS_ADDRESS_SIZE_BYTE))
            {
                checksum = pifs_calc_checksum(&a_file->map_header.next_map_address,
                                              PIFS_ADDRESS_SIZE_BYTE);
                if (checksum == a_file->map_header.next_map_checksum)
                {
                    /* Jump to the next map page */
                    ba = a_file->map_header.next_map_address.block_address;
                    pa = a_file->map_header.next_map_address.page_address;
                    po = PIFS_MAP_HEADER_SIZE_BYTE;
                }
                else
                {
                    a_file->status = PIFS_ERROR_CHECKSUM;
                }
            }
            else
            {
                end = TRUE;
            }
        }
    } while (!end && a_file->status == PIFS_SUCCESS);

    return a_file->status;
}

/**
 * @brief pifs_release_file_page Mark page as to be released.
 * Callback function for pifs_walk_file_pages().
 * If delta block and page are equal to original block address, no delta
 * page is used.
 *
 * @param[in] a_file                 Pointer to file.
 * @param[in] a_block_address        Original block address.
 * @param[in] a_page_address         Original block address.
 * @param[in] a_delta_block_address  Delta block address.
 * @param[in] a_delta_page_address   Delta page address.
 * @param[in] a_map_page             TRUE: the page is map page.
 *                                   FALSE: data page.
 * @param[in] a_func_data            User-data, which is unused.
 *
 * @return PIFS_SUCCESS if page marked successfully.
 */
pifs_status_t pifs_release_file_page(pifs_file_t * a_file,
                                     pifs_block_address_t a_block_address,
                                     pifs_page_address_t a_page_address,
                                     pifs_block_address_t a_delta_block_address,
                                     pifs_page_address_t a_delta_page_address,
                                     bool_t a_map_page,
                                     void * a_func_data)
{
    pifs_status_t ret;

    (void) a_file;
    (void) a_func_data;

    if (a_map_page)
    {
        PIFS_DEBUG_MSG("Release map page %s\r\n",
                       pifs_ba_pa2str(a_block_address, a_page_address));
        ret = pifs_mark_page(a_block_address, a_page_address, PIFS_MAP_PAGE_NUM, FALSE, TRUE);
    }
    else
    {
        PIFS_DEBUG_MSG("Release map entry %s\r\n",
                       pifs_ba_pa2str(a_delta_block_address, a_delta_page_address));
        /* Only delta page shall be released as the original is released *
         * when delta page is added. */
        ret = pifs_mark_page(a_delta_block_address, a_delta_page_address, 1, FALSE, TRUE);
    }

    return ret;
}

/**
 * @brief pifs_release_file_pages Mark file map and file's pages to be released.
 *
 * @param[in] a_file Pointer of file structure.
 * @return TRUE: if all pages were succesfully marked to be released.
 */
pifs_status_t pifs_release_file_pages(pifs_file_t * a_file)
{
    return pifs_walk_file_pages(a_file, pifs_release_file_page, NULL);
}

