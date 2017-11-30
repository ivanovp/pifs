/**
 * @file        pifs_merge.c
 * @brief       Pi file system: management and data merging functions
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-11-30 17:43:19 ivanovp {Time-stamp}
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
#include "pifs_merge.h"
#include "pifs_entry.h"
#include "pifs_map.h"
#include "pifs_wear.h"
#include "pifs_file.h"
#include "pifs_dir.h"
#include "buffer.h" /* DEBUG */

#define PIFS_COPY_FSBM   1

/**
 * @brief pifs_copy_fsbm Copy free space bitmap and process to be released pages.
 * It finds 'to be released' pages according to old free space bitmap and
 * erase them if they are in the same block.
 *
 * @param[in] a_new_header Pointer to previous file system's header.
 * @return PIFS_SUCCESS if erase was successful.
 */
static pifs_status_t pifs_copy_fsbm(pifs_header_t * a_old_header, pifs_header_t * a_new_header)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t fba = PIFS_FLASH_BLOCK_RESERVED_NUM;
    pifs_page_address_t  fpa = 0;
    pifs_block_address_t old_fsbm_ba = a_old_header->free_space_bitmap_address.block_address;
    pifs_page_address_t  old_fsbm_pa = a_old_header->free_space_bitmap_address.page_address;
#if PIFS_COPY_FSBM
    pifs_block_address_t new_fsbm_ba = a_new_header->free_space_bitmap_address.block_address;
    pifs_page_address_t  new_fsbm_pa = a_new_header->free_space_bitmap_address.page_address;
#endif
    pifs_size_t          i;
    bool_t               find = TRUE;
#if PIFS_COPY_FSBM
    bool_t               mark_block_free = FALSE;
#endif
    pifs_block_address_t to_be_released_ba;

    do
    {
#if PIFS_COPY_FSBM
        /* Read free space bitmap */
        ret = pifs_read(old_fsbm_ba, old_fsbm_pa, 0, &pifs.dmw_page_buf, PIFS_LOGICAL_PAGE_SIZE_BYTE);

        PIFS_DEBUG_MSG("Old free space bitmap:\r\n");
#if PIFS_DEBUG_LEVEL >= 5
        print_buffer(pifs.dmw_page_buf, PIFS_LOGICAL_PAGE_SIZE_BYTE,
                     old_fsbm_ba * PIFS_FLASH_BLOCK_SIZE_BYTE + old_fsbm_pa * PIFS_LOGICAL_PAGE_SIZE_BYTE);
#endif
#endif
        for (i = 0; i < PIFS_LOGICAL_PAGE_SIZE_BYTE && ret == PIFS_SUCCESS; i++)
        {
            if (ret == PIFS_SUCCESS && find)
            {
                /* Find to be released pages for whole block */
                ret = pifs_find_to_be_released_block(1, PIFS_BLOCK_TYPE_DATA,
                                                     fba, fba,
                                                     a_old_header,
                                                     &to_be_released_ba);
                find = FALSE;
                if (ret == PIFS_SUCCESS)
                {
                    /* Check if the found block is the actual block */
                    if (to_be_released_ba == fba)
                    {
                        if (pifs_is_block_type(fba, PIFS_BLOCK_TYPE_DATA, a_old_header))
                        {
                            ret = pifs_erase(fba, a_old_header, a_new_header);
#if PIFS_COPY_FSBM
                            /* Block erased, it can be marked as free */
                            mark_block_free = TRUE;
#endif
                            PIFS_WARNING_MSG("Block %i erased\r\n", fba);
                        }
                        else
                        {
                            PIFS_NOTICE_MSG("Block %i not erased, not data block\r\n", fba);
                        }
                    }
                    else
                    {
                        PIFS_ERROR_MSG("Internal error! Wrong block: %i\r\n", to_be_released_ba);
                    }
                }
                else
                {
                    /* If no blocks found, it is not an error */
                    ret = PIFS_SUCCESS;
                }
#if PIFS_COPY_FSBM
                /* Mark management block as free because */
                /* #1 The old management blocks (primary) will be erased, */
                /*    at the end of merge. */
                /* #2 The new management blocks (secondary) will be allocated, */
                /*    when the new header is written. */
                if (pifs_is_block_type(fba, PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT, a_old_header)
                        || pifs_is_block_type(fba, PIFS_BLOCK_TYPE_SECONDARY_MANAGEMENT, a_old_header))
                {
                    PIFS_NOTICE_MSG("Block %i is management, mark free in bitmap\r\n", fba);
                    /* Block will be allocated later or erased later, it can be */
                    /* marked as free */
                    mark_block_free = TRUE;
                }
#endif
            }

#if PIFS_COPY_FSBM
            if (mark_block_free)
            {
                pifs.dmw_page_buf[i] = PIFS_FLASH_ERASED_BYTE_VALUE;
            }
#endif

            fpa += PIFS_BYTE_BITS / PIFS_FSBM_BITS_PER_PAGE;
            if (fpa == PIFS_LOGICAL_PAGE_PER_BLOCK)
            {
                fpa = 0;
                fba++;
                find = TRUE;
#if PIFS_COPY_FSBM
                mark_block_free = FALSE;
#endif
            }
        }

#if PIFS_COPY_FSBM
        if (ret == PIFS_SUCCESS)
        {
            ret = pifs_write(new_fsbm_ba, new_fsbm_pa, 0, &pifs.dmw_page_buf, PIFS_LOGICAL_PAGE_SIZE_BYTE);
            PIFS_DEBUG_MSG("New free space bitmap:\r\n");
#if PIFS_DEBUG_LEVEL >= 5
            print_buffer(pifs.dmw_page_buf, PIFS_LOGICAL_PAGE_SIZE_BYTE,
                         new_fsbm_ba * PIFS_FLASH_BLOCK_SIZE_BYTE + new_fsbm_pa * PIFS_LOGICAL_PAGE_SIZE_BYTE);
#endif
        }
#endif

        if (fba < PIFS_FLASH_BLOCK_NUM_ALL)
        {
            if (ret == PIFS_SUCCESS)
            {
                ret = pifs_inc_ba_pa(&old_fsbm_ba, &old_fsbm_pa);
            }
#if PIFS_COPY_FSBM
            if (ret == PIFS_SUCCESS)
            {
                ret = pifs_inc_ba_pa(&new_fsbm_ba, &new_fsbm_pa);
            }
#endif
        }
    } while (ret == PIFS_SUCCESS && fba < PIFS_FLASH_BLOCK_NUM_ALL);

    return ret;
}

/**
 * @brief pifs_copy_map Copy map of a file.
 * It can compact map entries when pages are in sequence.
 * @param[in] a_old_entry Entry to be copied.
 * @return PIFS_SUCCESS if map was copied successfully.
 */
static pifs_status_t pifs_copy_map(pifs_entry_t * a_old_entry,
                                   pifs_header_t * a_old_header, pifs_header_t * a_new_header)
{
    pifs_status_t        ret = PIFS_ERROR_GENERAL;
    pifs_status_t        ret2;
    pifs_size_t          i;
    pifs_size_t          j;
    pifs_block_address_t old_map_ba = a_old_entry->first_map_address.block_address;
    pifs_page_address_t  old_map_pa = a_old_entry->first_map_address.page_address;
    pifs_map_header_t    old_map_header;
    pifs_map_entry_t     old_map_entry;
    pifs_map_entry_t     new_map_entry;
    bool_t               end = FALSE;
    pifs_address_t       delta_address;
    pifs_address_t       test_address;

    (void) a_new_header;

    PIFS_NOTICE_MSG("start\r\n");

    /* Re-create file in the new management block */
    ret = pifs_internal_open(&pifs.internal_file, a_old_entry->name, "w", FALSE);
    pifs.internal_file.entry.file_size = a_old_entry->file_size;
#if PIFS_ENABLE_ATTRIBUTES
    pifs.internal_file.entry.attrib = a_old_entry->attrib;
#endif
#if PIFS_ENABLE_USER_DATA
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_internal_fsetuserdata(&pifs.internal_file, &a_old_entry->user_data, FALSE);
    }
#endif
    if (pifs.internal_file.entry.file_size != PIFS_FILE_SIZE_ERASED)
    {
        pifs.internal_file.is_entry_changed = TRUE;
    }
    if (ret == PIFS_SUCCESS)
    {
        new_map_entry.address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
        new_map_entry.address.page_address = PIFS_PAGE_ADDRESS_INVALID;
        new_map_entry.page_count = 0;
        do
        {
            /* Read old map's header */
            ret = pifs_read(old_map_ba, old_map_pa, 0, &old_map_header, PIFS_MAP_HEADER_SIZE_BYTE);
            for (i = 0; i < PIFS_MAP_ENTRY_PER_PAGE && !end && ret == PIFS_SUCCESS; i++)
            {
                /* Go through all map entries in the page */
                ret = pifs_read(old_map_ba, old_map_pa,
                                PIFS_MAP_HEADER_SIZE_BYTE + i * PIFS_MAP_ENTRY_SIZE_BYTE,
                                &old_map_entry, PIFS_MAP_ENTRY_SIZE_BYTE);
                if (ret == PIFS_SUCCESS)
                {
                    if (!pifs_is_buffer_erased(&old_map_entry, PIFS_MAP_ENTRY_SIZE_BYTE))
                    {
                        PIFS_DEBUG_MSG("old map entry %s, page_count: %i\r\n",
                                       pifs_ba_pa2str(old_map_entry.address.block_address,
                                                      old_map_entry.address.page_address),
                                       old_map_entry.page_count);
                        /* Map entry is valid */
                        /* Check if original page was overwritten and */
                        /* delta page was used */
                        /* If map entries are sequential pages then page_count */
                        /* is increased. */
                        for (j = 0; j < old_map_entry.page_count && ret == PIFS_SUCCESS; j++)
                        {
                            ret = pifs_find_delta_page(old_map_entry.address.block_address,
                                                       old_map_entry.address.page_address,
                                                       &delta_address.block_address,
                                                       &delta_address.page_address, NULL,
                                                       a_old_header);
                            if (ret == PIFS_SUCCESS)
                            {
#if 1
                                if (old_map_entry.address.block_address != delta_address.block_address
                                        || old_map_entry.address.page_address != delta_address.page_address)
                                {
                                    PIFS_WARNING_MSG("delta %s -> %s\r\n",
                                                     pifs_address2str(&old_map_entry.address),
                                                     pifs_ba_pa2str(delta_address.block_address,
                                                                    delta_address.page_address));
                                }
#endif
                                if (!new_map_entry.page_count)
                                {
                                    new_map_entry.address = delta_address;
                                    new_map_entry.page_count = 1;
                                    test_address = delta_address;
                                }
                                else
                                {
                                    ret2 = pifs_inc_address(&test_address);
                                    /* Check if map page shall be written: */
                                    /* #1 End of flash reached */
                                    /* #2 Delta page was used */
                                    /* #3 Too much pages in page entry */
                                    if (ret2 != PIFS_SUCCESS /* End of flash reached! */
                                            || test_address.block_address != delta_address.block_address
                                            || test_address.page_address != delta_address.page_address
                                            || new_map_entry.page_count == PIFS_MAP_PAGE_COUNT_INVALID - 1)
                                    {
                                        PIFS_DEBUG_MSG("===> new map entry %s, page_count: %i\r\n",
                                                       pifs_ba_pa2str(new_map_entry.address.block_address,
                                                                      new_map_entry.address.page_address),
                                                       new_map_entry.page_count);
                                        ret = pifs_append_map_entry(&pifs.internal_file,
                                                                    new_map_entry.address.block_address,
                                                                    new_map_entry.address.page_address,
                                                                    new_map_entry.page_count);
#if PIFS_COPY_FSBM == 0
                                        if (ret == PIFS_SUCCESS)
                                        {
                                            ret = pifs_mark_page(new_map_entry.address.block_address,
                                                                 new_map_entry.address.page_address,
                                                                 new_map_entry.page_count, TRUE);
                                        }
#endif
                                        new_map_entry.address = delta_address;
                                        new_map_entry.page_count = 1;
                                        test_address = delta_address;
                                    }
                                    else
                                    {
                                        new_map_entry.page_count++;
                                    }
                                }
                            }
                            if (ret == PIFS_SUCCESS && j < old_map_entry.page_count)
                            {
                                /* Deliberately avoiding return code: */
                                /* it is not an error if we reach the end of */
                                /* flash memory */
                                (void)pifs_inc_address(&old_map_entry.address);
                            }
                        }
                    }
                    else
                    {
                        /* Map entry is unused */
                        end = TRUE;
                    }
                }
            }
            if (!pifs_is_buffer_erased(&old_map_header.next_map_address, sizeof(pifs_address_t)))
            {
                old_map_ba = old_map_header.next_map_address.block_address;
                old_map_pa = old_map_header.next_map_address.page_address;
            }
            else
            {
                end = TRUE;
            }
        } while (!end && ret == PIFS_SUCCESS);
        /* Check if there is un-written map entry */
        if (new_map_entry.page_count)
        {
            /* Write last map entry */
            PIFS_DEBUG_MSG("Last new map entry %s, page_count: %i\r\n",
                           pifs_ba_pa2str(new_map_entry.address.block_address,
                                          new_map_entry.address.page_address),
                           new_map_entry.page_count);
            ret = pifs_append_map_entry(&pifs.internal_file,
                                        new_map_entry.address.block_address,
                                        new_map_entry.address.page_address,
                                        new_map_entry.page_count);
#if PIFS_COPY_FSBM == 0
            if (ret == PIFS_SUCCESS)
            {
                ret = pifs_mark_page(new_map_entry.address.block_address,
                                     new_map_entry.address.page_address,
                                     new_map_entry.page_count, TRUE);
            }
#endif
            new_map_entry.page_count = 0;
        }
        /* Close internal file */
        ret = pifs_internal_fclose(&pifs.internal_file, FALSE);
        PIFS_ASSERT(ret == PIFS_SUCCESS);
    }

    return ret;
}

/**
 * @brief pifs_copy_entry_list copy list of files (entry list) from previous
 * management block.
 *
 * @param[in] a_old_header Pointer to previous file system's header.
 * @param[in] a_new_header Pointer to new file system's header.
 * @param[in] a_old_entry_list_address Pointer to old address of entry list.
 * @param[in] a_new_entry_list_address Pointer to new address of entry list.
 * @return PIFS_SUCCESS if copy was successful.
 */
static pifs_status_t pifs_copy_entry_list(pifs_header_t * a_old_header,
                                          pifs_header_t * a_new_header,
                                          pifs_address_t * a_old_entry_list_address,
                                          pifs_address_t * a_new_entry_list_address)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_size_t          i;
    pifs_size_t          j;
#if PIFS_DEBUG_LEVEL >= 3
    pifs_size_t          k = 0;
#endif
    bool_t               end = FALSE;
    pifs_block_address_t new_entry_list_ba = a_new_entry_list_address->block_address;
    pifs_page_address_t  new_entry_list_pa = a_new_entry_list_address->page_address;
    pifs_block_address_t old_entry_list_ba = a_old_entry_list_address->block_address;
    pifs_page_address_t  old_entry_list_pa = a_old_entry_list_address->page_address;
    pifs_entry_t         entry;
#if PIFS_ENABLE_DIRECTORIES
    /* TODO save cwd and restore? */
    pifs_address_t       current_entry_list_address;
    pifs_entry_t         new_dir_entry;
#endif

    PIFS_NOTICE_MSG("start\r\n");
    for (j = 0; j < PIFS_ENTRY_LIST_SIZE_PAGE && ret == PIFS_SUCCESS && !end; j++)
    {
        for (i = 0; i < PIFS_ENTRY_PER_PAGE && ret == PIFS_SUCCESS && !end; i++)
        {
            ret = pifs_read(old_entry_list_ba, old_entry_list_pa, i * PIFS_ENTRY_SIZE_BYTE, &entry,
                            PIFS_ENTRY_SIZE_BYTE);
            /* Check if entry is valid */
            if (!pifs_is_buffer_erased(&entry, PIFS_ENTRY_SIZE_BYTE))
            {
                PIFS_NOTICE_MSG("name: %s, size: %i, attrib: 0x%02X\r\n",
                                entry.name, entry.file_size, entry.attrib);
                if (!pifs_is_entry_deleted(&entry))
                {
#if PIFS_ENABLE_DIRECTORIES
                    if (PIFS_IS_DIR(entry.attrib))
                    {
                        if (!PIFS_IS_DOT_DIR(entry.name))
                        {
                            /* Copy directory */
                            ret = pifs_internal_mkdir(entry.name, FALSE);
                            if (ret == PIFS_SUCCESS)
                            {
                                current_entry_list_address = *pifs_get_task_current_entry_list_address();
                                ret = pifs_find_entry(PIFS_FIND_ENTRY, entry.name,
                                                      &new_dir_entry,
                                                      current_entry_list_address.block_address,
                                                      current_entry_list_address.page_address);
                            }
                            if (ret == PIFS_SUCCESS)
                            {
                                /* Enter new directory */
                                ret = pifs_internal_chdir(entry.name);
                            }
                            if (ret == PIFS_SUCCESS)
                            {
                                /* Copy entry list of directory */
                                ret = pifs_copy_entry_list(a_old_header, a_new_header,
                                                           &entry.first_map_address,
                                                           &new_dir_entry.first_map_address);
                            }
                            if (ret == PIFS_SUCCESS)
                            {
                                /* Go back to this directory */
                                ret = pifs_internal_chdir(PIFS_DOUBLE_DOT_STR);
                            }
                        }
                    }
                    else
#endif
                    {
                        /* Create file in the new management area and copy map */
                        ret = pifs_copy_map(&entry, a_old_header, a_new_header);
#if PIFS_DEBUG_LEVEL >= 3
                        k++;
                        if (((k + 1) % PIFS_ENTRY_PER_PAGE) == 0)
                        {
                            PIFS_NOTICE_MSG("%s\r\n", pifs_ba_pa2str(new_entry_list_ba, new_entry_list_pa));
#if PIFS_DEBUG_LEVEL >= 5
                            print_buffer(pifs.cache_page_buf, sizeof(pifs.cache_page_buf),
                                         new_entry_list_ba * PIFS_FLASH_BLOCK_SIZE_BYTE + new_entry_list_pa * PIFS_LOGICAL_PAGE_SIZE_BYTE);
#endif
                        }
#endif
                    }
                }
                else
                {
                    PIFS_NOTICE_MSG("name %s DELETED\r\n", entry.name);
                }
            }
            else
            {
                end = TRUE;
            }
        }
        if (ret == PIFS_SUCCESS)
        {
            ret = pifs_inc_ba_pa(&new_entry_list_ba, &new_entry_list_pa);
        }
        if (ret == PIFS_SUCCESS)
        {
            ret = pifs_inc_ba_pa(&old_entry_list_ba, &old_entry_list_pa);
        }
    }

    return ret;
}

/**
 * @brief pifs_merge Merge management and data pages. Erase to be released pages.
 * Note: the caller shall provide mutex protection!
 *
 * Steps of merging:
 * #0 Close opened files, but store actual file position.
 * #1 Erase next management blocks.
 * #2 Initialize file system's header, but not write. Next management blocks'
 *    address is not initialized and checksum is not calculated.
 * #3 Copy wear level list.
 * #4 Copy free space bitmap (FSBM) from old management blocks to new ones and
 *    erase to be released blocks. New free space bitmap will be updated.
 * #5 Write new management blocks. Address of next management blocks and
 *    checksum shall not be written.
 * #6 Copy file entries from old to new management blocks. Maps are also copied,
 *    so map blocks are allocated from new management area (FSBM is needed).
 * #7 Erase delta page mirror in RAM.
 * #8 Find free blocks for next management block in the new file system header.
 * #9 Add next management block's address to the new file system header and
 *    calculate checksum.
 * #10 Update page of new file system header. Checksum is written, so the new
 *    file system header is valid from this point.
 * #11 Erase old management blocks.
 * #12 Re-open files and seek to the stored position.
 *     Therefore actual_map_address, map_header, etc. will be updated.
 *
 * @return PIFS_SUCCESS when merge was successful.
 */
pifs_status_t pifs_merge(void)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t next_mgmt_ba = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_block_address_t new_header_ba = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_page_address_t  new_header_pa = PIFS_PAGE_ADDRESS_INVALID;
    pifs_header_t        old_header = pifs.header;
    pifs_header_t        new_header;
    pifs_size_t          i;
    pifs_file_t        * file;
    bool_t               file_is_opened[PIFS_OPEN_FILE_NUM_MAX] = { 0 };
    pifs_size_t          file_pos[PIFS_OPEN_FILE_NUM_MAX] = { 0 };

    PIFS_WARNING_MSG("start\r\n");
    PIFS_ASSERT(!pifs.is_merging);
    pifs.is_merging = TRUE;
    /* #0 */
    for (i = 0; i < PIFS_OPEN_FILE_NUM_MAX; i++)
    {
        file = &pifs.file[i];
        file_is_opened[i] = file->is_opened;
        if (file_is_opened[i])
        {
            /* Store position in file */
            PIFS_WARNING_MSG("rw_pos: %i\r\n", file->rw_pos);
            file_pos[i] = file->rw_pos;
            /* This will never cause data loss. There shall be enough free
             * entries in the entry list. */
            (void)pifs_internal_fclose(file, FALSE);
        }
    }
    /* #1 */
    for (i = 0; i < PIFS_MANAGEMENT_BLOCK_NUM && ret == PIFS_SUCCESS; i++)
    {
        ret = pifs_erase(old_header.next_management_block_address + i, NULL, NULL);
    }
    /* #2 */
    if (ret == PIFS_SUCCESS)
    {
        new_header.counter = old_header.counter;
        new_header_ba = old_header.next_management_block_address;
        new_header_pa = 0;
        ret = pifs_header_init(new_header_ba, new_header_pa, PIFS_BLOCK_ADDRESS_ERASED, &new_header);
    }
    /* #3 */
    if (ret == PIFS_SUCCESS)
    {
        /* Copy wear level list */
        ret = pifs_copy_wear_level_list(&old_header, &new_header);
    }
    for (i = 0; i < PIFS_MANAGEMENT_BLOCK_NUM && ret == PIFS_SUCCESS; i++)
    {
        ret = pifs_inc_wear_level(new_header.management_block_address + i, &new_header);
    }
    /* #4 */
    if (ret == PIFS_SUCCESS)
    {
        /* Copy free space bitmap */
        /* This should be before calling pifs_header_write(..., TRUE) */
        /* because that call will mark management area in the free space */
        /* bitmap as used space. */
        ret = pifs_copy_fsbm(&old_header, &new_header);
    }
    /* #5 */
    if (ret == PIFS_SUCCESS)
    {
        /* Activate new file system header */
        pifs.header = new_header;
        /* Write new management area's header and mark header, entry list, */
        /* free space bitmap, delta pages, wear level list as used space. */
        ret = pifs_header_write(new_header_ba, new_header_pa, &pifs.header, TRUE);
    }
    /* #6 */
    if (ret == PIFS_SUCCESS)
    {
        /* Copy file entry list and process delta pages */
        /* This should be before resetting delta pages! */
#if PIFS_ENABLE_DIRECTORIES
        for (i = 0; i < PIFS_TASK_COUNT_MAX; i++)
        {
            pifs.current_entry_list_address[i] = new_header.root_entry_list_address;
        }
#endif
        ret = pifs_copy_entry_list(&old_header, &new_header,
                                   &old_header.root_entry_list_address,
                                   &new_header.root_entry_list_address);
    }
    /* #7 */
    if (ret == PIFS_SUCCESS)
    {
        /* Reset delta map after processing delta pages */
        pifs_reset_delta();
    }
    /* #8 */
    if (ret == PIFS_SUCCESS)
    {
        /* Find next management blocks address */
        /* Old header's primary management blocks and data blocks are allowed */
        /* This won't find PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT as they are */
        /* marked used. */
        ret = pifs_find_block_wl(PIFS_MANAGEMENT_BLOCK_NUM,
                                 PIFS_BLOCK_TYPE_DATA,
                                 FALSE,
                                 &old_header,
                                 &next_mgmt_ba);
        if (ret == PIFS_ERROR_NO_MORE_SPACE)
        {
            /* Old management area will be the next management block */
            next_mgmt_ba = old_header.management_block_address;
            ret = PIFS_SUCCESS;
        }
    }
    /* #9 */
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_generate_least_weared_blocks(&pifs.header);
    }
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_generate_most_weared_blocks(&pifs.header);
    }
    if (ret == PIFS_SUCCESS)
    {
        PIFS_NOTICE_MSG("Next management block: %i, ret: %i\r\n", next_mgmt_ba, ret);
        /* Add next management block's address to the current header */
        /* and calculate checksum */
        ret = pifs_header_init(new_header_ba, new_header_pa, next_mgmt_ba, &pifs.header);
    }
    /* #10 */
    if (ret == PIFS_SUCCESS)
    {
        /* Write new management area's header with next management block's address */
        /* and write checksum */
        ret = pifs_header_write(new_header_ba, new_header_pa, &pifs.header, FALSE);
        /* At this point new header is valid */
    }
    /* #11 */
    if (ret == PIFS_SUCCESS)
    {
        PIFS_ASSERT(old_header.management_block_address != new_header.management_block_address);
        /* Erase old management area */
        for (i = 0; i < PIFS_MANAGEMENT_BLOCK_NUM && ret == PIFS_SUCCESS; i++)
        {
            PIFS_WARNING_MSG("Erasing old management block %i\r\n", old_header.management_block_address + i);
            ret = pifs_erase(old_header.management_block_address + i, &old_header, &new_header);
        }
    }
    /* #12 */
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_get_free_pages(&i, &pifs.free_data_page_num);
    }
    if (ret == PIFS_SUCCESS)
    {
        /* Re-open files */
        for (i = 0; i < PIFS_OPEN_FILE_NUM_MAX && ret == PIFS_SUCCESS; i++)
        {
            file = &pifs.file[i];
            if (file_is_opened[i])
            {
                /* Do not create new file, as it has already done */
                /* TODO save and restore mode_create_new_file and mode_file_shall_exist?
                 * to their original value? */
                file->mode_create_new_file = FALSE;
                file->mode_file_shall_exist = TRUE;
                /* TODO file->entry.name is not enough, full path should be stored!? */
                ret = pifs_internal_open(file, file->entry.name, NULL, FALSE);
                if (ret == PIFS_SUCCESS && file_pos[i])
                {
                    PIFS_WARNING_MSG("Seeking to %i\r\n", file_pos[i]);
                    /* Seek to the stored position */
                    ret = pifs_internal_fseek(file, file_pos[i], PIFS_SEEK_SET);
                    if (ret != 0)
                    {
                        PIFS_ERROR_MSG("Seek error: %i\r\n", ret);
                    }
                }
            }
        }
    }
    pifs.is_merging = FALSE;
    PIFS_ASSERT(ret == PIFS_SUCCESS);
    PIFS_WARNING_MSG("stop\r\n");

    return ret;
}

/**
 * @brief pifs_merge_check Check if data merge is needed and perform it.
 *
 * @param[in] a_file                    Pointer to actual file or NULL.
 * @param[in] a_data_page_count_minimum Number of data pages needed by caller.
 * @return PIFS_SUCCESS if merge was not necessary or merge was successfull.
 */
pifs_status_t pifs_merge_check(pifs_file_t * a_file, pifs_size_t a_data_page_count_minimum)
{
    pifs_status_t ret = PIFS_SUCCESS;
    pifs_size_t   free_management_pages = 0;
    pifs_size_t   free_data_pages = 0;
    pifs_size_t   to_be_released_management_pages = 0;
    pifs_size_t   to_be_released_data_pages = 0;
    bool_t        merge = FALSE;
    //bool_t        is_free_map_entry = TRUE;
    pifs_block_address_t to_be_released_ba;
    pifs_size_t   free_entries = 0;
    pifs_size_t   to_be_released_entries = 0;

    PIFS_DEBUG_MSG("name: %s, data page min: %i\r\n",
                   a_file ? a_file->entry.name : "NULL", a_data_page_count_minimum);
    /* Get number of free management and data pages */
    ret = pifs_get_free_pages(&free_management_pages, &free_data_pages);
    PIFS_NOTICE_MSG("free_data_pages: %lu, free_management_pages: %lu\r\n",
                    free_data_pages, free_management_pages);
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_count_entries(&free_entries, &to_be_released_entries,
                                 pifs.header.root_entry_list_address.block_address,
                                 pifs.header.root_entry_list_address.page_address);
//        PIFS_NOTICE_MSG("free_entries: %lu, to_be_released_entries: %lu\r\n",
//                        free_entries, to_be_released_entries);
    }
    if (ret == PIFS_SUCCESS && (free_data_pages < a_data_page_count_minimum
                                || free_management_pages == 0 || free_entries <= PIFS_OPEN_FILE_NUM_MAX))
    {
        /* PIFS_OPEN_FILE_NUM_MAX is checked because there should be enough space */
        /* to close all opened files during merge! */
        if (free_entries <= PIFS_OPEN_FILE_NUM_MAX && to_be_released_entries > 0)
        {
            merge = TRUE;
        }
        if (!merge)
        {
            /* Get number of erasable pages */
            ret = pifs_get_to_be_released_pages(&to_be_released_management_pages,
                                                &to_be_released_data_pages);
            PIFS_NOTICE_MSG("to_be_released_data_pages: %lu, to_be_released_management_pages: %lu\r\n",
                            to_be_released_data_pages, to_be_released_management_pages);
            if (ret == PIFS_SUCCESS)
            {
                if (free_data_pages < a_data_page_count_minimum
                        && to_be_released_data_pages >= a_data_page_count_minimum)
                {
                    /* Check if at least one data block can be erased! */
                    /* Otherwise merging will be unmeaning. */
                    ret = pifs_find_to_be_released_block(1, PIFS_BLOCK_TYPE_DATA,
                                                         PIFS_FLASH_BLOCK_RESERVED_NUM,
                                                         PIFS_FLASH_BLOCK_NUM_ALL - 1,
                                                         &pifs.header,
                                                         &to_be_released_ba);
                    if (ret == PIFS_SUCCESS)
                    {
                        PIFS_NOTICE_MSG("To be released block: %i\r\n", to_be_released_ba);
                        merge = TRUE;
                    }
                    else
                    {
                        /* It is not an error when there are no to be released */
                        /* blocks */
                        /* TODO When no blocks can be released, static wear leveling */
                        /* may be useful, but some space shall be reserved as */
                        /* at this point there is no free space! */
                        ret = PIFS_SUCCESS;
                    }
                }
                if (free_management_pages == 0 && to_be_released_management_pages > 0 && !merge)
                {
                    /* TODO number of free map entries should be calculated here! */
#if 0
                    if (a_file)
                    {
                        /* If free_management_pages is 0, number of free map entries */
                        /* are to be checked. If there is free entry in the actual */
                        /* map, no merge is needed. */
                        ret = pifs_is_free_map_entry(a_file, &is_free_map_entry);
                        if (ret == PIFS_SUCCESS && !is_free_map_entry)
                        {
                            merge = TRUE;
                        }
                    }
                    else
#endif
                    {
                        merge = TRUE;
                    }
                }
            }
            else if (ret == PIFS_ERROR_NO_MORE_SPACE)
            {
                /* It is not an error when no TBR pages found. */
                ret = PIFS_SUCCESS;
            }
        }
        if (ret == PIFS_SUCCESS && merge)
        {
            /* Some pages could be erased, do data merge */
            ret = pifs_merge();
            if (a_file)
            {
                /* Update entry list address, as it may changed during merge! */
#if PIFS_ENABLE_DIRECTORIES
                /* TODO all file's entry list address shall be updated! */
                a_file->entry_list_address = *pifs_get_task_current_entry_list_address();
#else
                a_file->entry_list_address = pifs.header.root_entry_list_address;
#endif
            }
        }
        else
        {
            PIFS_NOTICE_MSG("Merge is not needed!\r\n");
        }
    }

    return ret;
}
