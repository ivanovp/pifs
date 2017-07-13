/**
 * @file        pifs_merge.c
 * @brief       Pi file system: management and data merging functions
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
#include "pifs_merge.h"
#include "pifs_map.h"
#include "buffer.h" /* DEBUG */

#define PIFS_DEBUG_LEVEL    1
#include "pifs_debug.h"

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
    pifs_status_t        ret = PIFS_ERROR;
    pifs_block_address_t fba = PIFS_FLASH_BLOCK_RESERVED_NUM;
    pifs_page_address_t  fpa = 0;
    pifs_block_address_t old_fsbm_ba = a_old_header->free_space_bitmap_address.block_address;
    pifs_page_address_t  old_fsbm_pa = a_old_header->free_space_bitmap_address.page_address;
    pifs_block_address_t new_fsbm_ba = a_new_header->free_space_bitmap_address.block_address;
    pifs_page_address_t  new_fsbm_pa = a_new_header->free_space_bitmap_address.page_address;
    pifs_size_t          i;
    bool_t               find = TRUE;
    bool_t               mark_block_free = FALSE;
    pifs_block_address_t to_be_released_ba;

    PIFS_ASSERT(pifs.is_header_found);

    do
    {
        /* Read free space bitmap */
        ret = pifs_read(old_fsbm_ba, old_fsbm_pa, 0, &pifs.page_buf, PIFS_FLASH_PAGE_SIZE_BYTE);

        PIFS_DEBUG_MSG("Free space bitmap:\r\n");
        print_buffer(pifs.page_buf, PIFS_FLASH_PAGE_SIZE_BYTE,
                     old_fsbm_ba * PIFS_FLASH_BLOCK_SIZE_BYTE + old_fsbm_pa * PIFS_FLASH_PAGE_SIZE_BYTE);

        for (i = 0; i < PIFS_FLASH_PAGE_SIZE_BYTE && ret == PIFS_SUCCESS; i++)
        {
            if (ret == PIFS_SUCCESS && find)
            {
                /* Find to be released pages for whole block */
                ret = pifs_find_to_be_released_block(1, PIFS_BLOCK_TYPE_DATA, fba,
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
                            ret = pifs_erase(fba);
                            /* Block erased, it can be marked as free */
                            mark_block_free = TRUE;
                            PIFS_NOTICE_MSG("Block %i erased\r\n", fba);
                        }
                        else
                        {
                            PIFS_NOTICE_MSG("Block %i not erased, not data block\r\n", fba);
                        }
                    }
                }
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
            }

            if (mark_block_free)
            {
                pifs.page_buf[i] = PIFS_FLASH_ERASED_BYTE_VALUE;
            }

            fpa += PIFS_BYTE_BITS / PIFS_FSBM_BITS_PER_PAGE;
            if (fpa == PIFS_FLASH_PAGE_PER_BLOCK)
            {
                fpa = 0;
                fba++;
                find = TRUE;
                mark_block_free = FALSE;
            }
        }

        if (ret == PIFS_SUCCESS)
        {
            ret = pifs_write(new_fsbm_ba, new_fsbm_pa, 0, &pifs.page_buf, PIFS_FLASH_PAGE_SIZE_BYTE);
            print_buffer(pifs.page_buf, PIFS_FLASH_PAGE_SIZE_BYTE,
                         new_fsbm_ba * PIFS_FLASH_BLOCK_SIZE_BYTE + new_fsbm_pa * PIFS_FLASH_PAGE_SIZE_BYTE);
        }

        if (fba < PIFS_FLASH_BLOCK_NUM_ALL)
        {
            if (ret == PIFS_SUCCESS)
            {
                ret = pifs_inc_ba_pa(&old_fsbm_ba, &old_fsbm_pa);
            }
            if (ret == PIFS_SUCCESS)
            {
                ret = pifs_inc_ba_pa(&new_fsbm_ba, &new_fsbm_pa);
            }
        }
    } while (ret == PIFS_SUCCESS && fba < PIFS_FLASH_BLOCK_NUM_ALL);

    return ret;
}

/**
 * @brief pifs_copy_map Copy map of a file.
 * @param[in] a_old_entry Entry to be copied.
 * @return PIFS_SUCCESS if map was copied successfully.
 */
static pifs_status_t pifs_copy_map(pifs_entry_t * a_old_entry)
{
    pifs_status_t        ret = PIFS_SUCCESS;

    pifs_size_t          i;
    pifs_size_t          j;
    pifs_block_address_t old_map_ba = a_old_entry->first_map_address.block_address;
    pifs_page_address_t  old_map_pa = a_old_entry->first_map_address.page_address;
    pifs_map_header_t    old_map_header;
    pifs_map_entry_t     map_entry;
    bool_t               end = FALSE;
    pifs_block_address_t delta_ba;
    pifs_page_address_t  delta_pa;
    pifs_address_t       address;

    PIFS_NOTICE_MSG("start\r\n");

    pifs_internal_open(&pifs.internal_file, a_old_entry->name, "w");

    if (pifs.internal_file.status == PIFS_SUCCESS)
    {
        do
        {
            /* Read map's header */
            ret = pifs_read(old_map_ba, old_map_pa, 0, &old_map_header, PIFS_MAP_HEADER_SIZE_BYTE);
            for (i = 0; i < PIFS_MAP_ENTRY_PER_PAGE && !end && ret == PIFS_SUCCESS; i++)
            {
                /* Go through all map entries in the page */
                ret = pifs_read(old_map_ba, old_map_pa, PIFS_MAP_HEADER_SIZE_BYTE + i * PIFS_MAP_ENTRY_SIZE_BYTE,
                                &map_entry, PIFS_MAP_ENTRY_SIZE_BYTE);
                if (ret == PIFS_SUCCESS)
                {
                    if (!pifs_is_buffer_erased(&map_entry, PIFS_MAP_ENTRY_SIZE_BYTE))
                    {
                        /* Map entry is valid */
                        /* Check if original page was overwritten and */
                        /* delta page was used */
                        /* TODO to be optimized: map entry entries are added */
                        /* one by one, if data pages follow each other */
                        /* 2 or more map entries can be added. */
                        address = map_entry.address;
                        for (j = 0; j < map_entry.page_count && ret == PIFS_SUCCESS; j++)
                        {
                            ret = pifs_find_delta_page(address.block_address,
                                                       address.page_address,
                                                       &delta_ba, &delta_pa, NULL);
                            if (ret == PIFS_SUCCESS)
                            {
                                PIFS_DEBUG_MSG("%s ->", pifs_address2str(&address));
                                PIFS_DEBUG_MSG("%s\r\n", pifs_ba_pa2str(delta_ba, delta_pa));
                                ret = pifs_append_map_entry(&pifs.internal_file,
                                                      address.block_address,
                                                      address.page_address, 1); /* <<< one page added here */
                            }
                            if (ret == PIFS_SUCCESS && j < map_entry.page_count)
                            {
                                ret = pifs_inc_address(&address);
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
        /* Close internal file */
        ret = pifs_fclose(&pifs.internal_file);
        PIFS_ASSERT(ret == PIFS_SUCCESS);
    }

    return ret;
}

/**
 * @brief pifs_copy_entry_list copy list of files (entry list) from previous
 * management block.
 *
 * @param[in] a_old_header Pointer to previous file system's header.
 * @return PIFS_SUCCESS if copy was successful.
 */
static pifs_status_t pifs_copy_entry_list(pifs_header_t * a_old_header, pifs_header_t * a_new_header)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_size_t          i;
    pifs_size_t          j;
    pifs_size_t          entry_cntr;
    pifs_block_address_t new_entry_list_ba = a_new_header->entry_list_address.block_address;
    pifs_page_address_t  new_entry_list_pa = a_new_header->entry_list_address.page_address;
    pifs_block_address_t old_entry_list_ba = a_old_header->entry_list_address.block_address;
    pifs_page_address_t  old_entry_list_pa = a_old_header->entry_list_address.page_address;
    pifs_entry_t         entry; /* FIXME this might be too much data for stack */

    PIFS_NOTICE_MSG("start\r\n");
    entry_cntr = 0;
    do
    {
        for (i = 0, j = 0; i < PIFS_ENTRY_PER_PAGE && entry_cntr < PIFS_ENTRY_NUM_MAX; i++)
        {
            ret = pifs_read(old_entry_list_ba, old_entry_list_pa, i * PIFS_ENTRY_SIZE_BYTE, &entry,
                            PIFS_ENTRY_SIZE_BYTE);
            /* Check if entry is valid */
            if (!pifs_is_buffer_erased(&entry, PIFS_ENTRY_SIZE_BYTE))
            {
                PIFS_NOTICE_MSG("name: %s attrib: 0x%02X\r\n",
                                entry.name, entry.attrib);
            }
            if (!pifs_is_buffer_erased(&entry, PIFS_ENTRY_SIZE_BYTE)
                    && (entry.attrib != 0))
            {
                /* Create file in the new management area and copy map */
                ret = pifs_copy_map(&entry);
                j++;
                if ((j % PIFS_ENTRY_PER_PAGE) == 0)
                {
                    PIFS_NOTICE_MSG("%s\r\n", pifs_ba_pa2str(new_entry_list_ba, new_entry_list_pa));
                    print_buffer(pifs.cache_page_buf, sizeof(pifs.cache_page_buf),
                                 new_entry_list_ba * PIFS_FLASH_BLOCK_SIZE_BYTE + new_entry_list_pa * PIFS_FLASH_PAGE_SIZE_BYTE);
                }
            }
            entry_cntr++;
        }
        if (entry_cntr < PIFS_ENTRY_NUM_MAX)
        {
            if (ret == PIFS_SUCCESS)
            {
                ret = pifs_inc_ba_pa(&new_entry_list_ba, &new_entry_list_pa);
            }
            if (ret == PIFS_SUCCESS)
            {
                ret = pifs_inc_ba_pa(&old_entry_list_ba, &old_entry_list_pa);
            }
        }
    } while (entry_cntr < PIFS_ENTRY_NUM_MAX && ret == PIFS_SUCCESS);

    return ret;
}

/**
 * @brief pifs_merge Merge management and data pages. Erase to be released pages.
 *
 * Steps of merging:
 * #0 Close opened files.
 * #1 Erase next management blocks
 * #2 Initialize file system's header, but not write. Next management blocks'
 *    address is not initialized and checksum is not calculated.
 * #3 Copy free space bitmap from old management blocks to new ones and
 *    erase to be released blocks. New free space bitmap will be updated.
 * #4 Write new management blocks. Address of next management blocks is not
 *    initialized.
 * #5 Copy file entries from old to new management blocks. Maps are also copied,
 *    so map blocks are allocated from new management area (FSBM is needed).
 * #6 Erase old management blocks. Erase delta page mirror in RAM.
 * #7 Find free blocks for next management block in the new file system header.
 * #8 Add next management block's address to the new file system header and
 *    calculate checksum.
 * #9 Update page of new file system header.
 * #10 Re-open files and seek to the previous position.
 *     Therefore actual_map_address, map_header, etc. will be updated.
 *
 * @return PIFS_SUCCES when merge was successful.
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

    PIFS_NOTICE_MSG("start\r\n");
    /* #0 */
    for (i = 0; i < PIFS_OPEN_FILE_NUM_MAX; i++)
    {
        file = &pifs.file[i];
        file_is_opened[i] = file->is_opened;
        if (file_is_opened[i])
        {
            file_pos[i] = file->read_pos;
            pifs_fclose(file);
        }
    }
    /* #1 */
    for (i = 0; i < PIFS_MANAGEMENT_BLOCKS && ret == PIFS_SUCCESS; i++)
    {
        ret = pifs_erase(pifs.header.next_management_blocks[i]);
    }
    /* #2 */
    if (ret == PIFS_SUCCESS)
    {
        new_header.counter = old_header.counter;
        new_header_ba = old_header.next_management_blocks[0];
        new_header_pa = 0;
        ret = pifs_header_init(new_header_ba, new_header_pa, PIFS_BLOCK_ADDRESS_ERASED, &new_header);
    }
    /* #3 */
    if (ret == PIFS_SUCCESS)
    {
        /* Copy free space bitmap */
        ret = pifs_copy_fsbm(&old_header, &new_header);
    }
    /* #4 */
    if (ret == PIFS_SUCCESS)
    {
        /* Activate new file system header */
        pifs.header = new_header;
        /* Write new management area's header */
        ret = pifs_header_write(new_header_ba, new_header_pa, &pifs.header, TRUE);
    }
    /* #5 */
    if (ret == PIFS_SUCCESS)
    {
        /* Copy file entry list and process deltas */
        ret = pifs_copy_entry_list(&old_header, &new_header);
    }
    /* #6 */
    if (ret == PIFS_SUCCESS)
    {
        /* Erase old management area */
        for (i = 0; i < PIFS_MANAGEMENT_BLOCKS && ret == PIFS_SUCCESS; i++)
        {
            ret = pifs_erase(old_header.management_blocks[i]);
        }
        /* Reset delta map */
        memset(pifs.delta_map_page_buf, PIFS_FLASH_ERASED_BYTE_VALUE,
               PIFS_DELTA_MAP_PAGE_NUM * PIFS_FLASH_PAGE_SIZE_BYTE);
        pifs.delta_map_page_is_dirty = FALSE;
        pifs.delta_map_page_is_read = FALSE;
    }
    /* #7 */
    if (ret == PIFS_SUCCESS)
    {
        /* Find next management blocks address after actual management block */
        ret = pifs_find_free_block(PIFS_MANAGEMENT_BLOCKS, PIFS_BLOCK_TYPE_ANY,
                                   new_header_ba + PIFS_MANAGEMENT_BLOCKS,
                                   &pifs.header,
                                   &next_mgmt_ba);
        if (ret != PIFS_SUCCESS)
        {
            /* No free blocks found, find next management blocks address */
            /* anywhere */
            ret = pifs_find_free_block(PIFS_MANAGEMENT_BLOCKS, PIFS_BLOCK_TYPE_ANY,
                                       0,
                                       &pifs.header,
                                       &next_mgmt_ba);
        }
    }
    /* #8 */
    if (ret == PIFS_SUCCESS)
    {
        PIFS_DEBUG_MSG("next management block: %i, ret: %i\r\n", next_mgmt_ba, ret);
        /* Add next management block's address to the current header */
        ret = pifs_header_init(new_header_ba, new_header_pa, next_mgmt_ba, &pifs.header);
    }
    /* #9 */
    if (ret == PIFS_SUCCESS)
    {
        /* Write new management area's header with next management block's address */
        ret = pifs_header_write(new_header_ba, new_header_pa, &pifs.header, FALSE);
    }
    /* #10 */
    if (ret == PIFS_SUCCESS)
    {
        for (i = 0; i < PIFS_OPEN_FILE_NUM_MAX; i++)
        {
            file = &pifs.file[i];
            if (file_is_opened[i])
            {
                file->mode_create_new_file = FALSE;
                pifs_internal_open(file, file->entry.name, NULL);
                if (file->status == PIFS_SUCCESS)
                {
                    pifs_fseek(file, file_pos[i], PIFS_SEEK_SET);
                }
            }
        }
    }
//    pifs_flush();
//    exit(1);

    return ret;
}

/**
 * @brief pifs_merge_check Check if data merge is needed and perform it.
 *
 * @param[in] a_file Pointer to actual file or NULL.
 * @return PIFS_SUCCESS if merge was not necessary or merge was successfull.
 */
pifs_status_t pifs_merge_check(pifs_file_t * a_file)
{
    pifs_status_t ret = PIFS_SUCCESS;
    pifs_size_t   free_management_pages = 0;
    pifs_size_t   free_data_pages = 0;
    pifs_size_t   to_be_released_management_pages = 0;
    pifs_size_t   to_be_released_data_pages = 0;
    bool_t        merge = FALSE;
    bool_t        is_free_map_entry = TRUE;

    /* Get number of free management and data pages */
    ret = pifs_get_free_pages(&free_management_pages, &free_data_pages);
    PIFS_NOTICE_MSG("free_data_pages: %lu, free_management_pages: %lu\r\n",
                    free_data_pages, free_management_pages);
    if (ret == PIFS_SUCCESS && (free_data_pages == 0 || free_management_pages == 0))
    {
        /* Get number of erasable pages */
        ret = pifs_get_to_be_released_pages(&to_be_released_management_pages,
                                            &to_be_released_data_pages);
        PIFS_NOTICE_MSG("to_be_released_data_pages: %lu, to_be_released_management_pages: %lu\r\n",
                        to_be_released_data_pages, to_be_released_management_pages);
        if (ret == PIFS_SUCCESS)
        {
            if (free_data_pages == 0 && to_be_released_data_pages > 0)
            {
                merge = TRUE;
            }
            if (free_management_pages == 0 && to_be_released_management_pages > 0)
            {
                if (a_file)
                {
                    /*
                     * If free_management_pages is 0, number of free map entries
                     * are to be checked. If there is free entry in the actual
                     * map, no merge is needed.
                     */
                    ret = pifs_is_free_map_entry(a_file, &is_free_map_entry);
                    if (ret == PIFS_SUCCESS && !is_free_map_entry)
                    {
                        merge = TRUE;
                    }
                }
                else
                {
                    merge = TRUE;
                }
            }
        }
        if (ret == PIFS_SUCCESS && merge)
        {
            /* Some pages could be erased, do data merge */
            ret = pifs_merge();
        }
    }

    return ret;
}
