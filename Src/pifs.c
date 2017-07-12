/**
 * @file        pifs.c
 * @brief       Pi file system
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
#include "pifs_debug.h"
#include "buffer.h" /* DEBUG */

pifs_t pifs =
{
    .header_address = { PIFS_BLOCK_ADDRESS_INVALID, PIFS_PAGE_ADDRESS_INVALID },
    .is_header_found = FALSE,
    .header = { 0 },
    .cache_page_buf_address = { PIFS_BLOCK_ADDRESS_INVALID, PIFS_PAGE_ADDRESS_INVALID },
    .cache_page_buf = { 0 },
    .cache_page_buf_is_dirty = FALSE
};

/**
 * @brief pifs_calc_header_checksum Calculate checksum of the file system header.
 *
 * @param[in] a_pifs_header Pointer to the file system header.
 * @return The calculated checksum.
 */
static pifs_checksum_t pifs_calc_header_checksum(pifs_header_t * a_pifs_header)
{
    uint8_t * ptr = (uint8_t*) a_pifs_header;
    pifs_checksum_t checksum = (pifs_checksum_t) UINT32_MAX;
    uint8_t cntr = sizeof(pifs_header_t) - sizeof(pifs_checksum_t); /* Skip checksum */
    
    while (cntr--)
    {
        checksum += *ptr;
        ptr++;
    }

    return checksum;
}

/**
 * @brief pifs_flush  Flush cache.
 *
 * @return PIFS_SUCCESS if data flush successfully.
 */
pifs_status_t pifs_flush(void)
{
    pifs_status_t ret = PIFS_SUCCESS;

    if (pifs.cache_page_buf_is_dirty)
    {
        ret = pifs_flash_write(pifs.cache_page_buf_address.block_address,
                               pifs.cache_page_buf_address.page_address,
                               0,
                               pifs.cache_page_buf, PIFS_FLASH_PAGE_SIZE_BYTE);
        if (ret == PIFS_SUCCESS)
        {
            pifs.cache_page_buf_is_dirty = FALSE;
        }
        else
        {
            PIFS_ERROR_MSG("Cannot flush buffer %s\r\n",
                           pifs_address2str(&pifs.cache_page_buf_address));
        }
    }

    return ret;
}

/**
 * @brief pifs_read  Cached read.
 *
 * @param[in] a_block_address   Block address of page to read.
 * @param[in] a_page_address    Page address of page to read.
 * @param[in] a_page_offset     Offset in page.
 * @param[out] a_buf            Pointer to buffer to fill or NULL if
 *                              pifs.cache_page_buf is used.
 * @param[in] a_buf_size        Size of buffer. Ignored if a_buf is NULL.
 * @return PIFS_SUCCESS if data read successfully.
 */
pifs_status_t pifs_read(pifs_block_address_t a_block_address,
                        pifs_page_address_t a_page_address,
                        pifs_page_offset_t a_page_offset,
                        void * const a_buf,
                        pifs_size_t a_buf_size)
{
    pifs_status_t ret = PIFS_ERROR;

    if (a_block_address == pifs.cache_page_buf_address.block_address
            && a_page_address == pifs.cache_page_buf_address.page_address)
    {
        /* Cache hit */
        if (a_buf)
        {
            memcpy(a_buf, &pifs.cache_page_buf[a_page_offset], a_buf_size);
        }
        ret = PIFS_SUCCESS;
    }
    else
    {
        /* Cache miss, first flush cache */
        ret = pifs_flush();

        if (ret == PIFS_SUCCESS)
        {
            ret = pifs_flash_read(a_block_address, a_page_address, 0,
                                  pifs.cache_page_buf, PIFS_FLASH_PAGE_SIZE_BYTE);
        }

        if (ret == PIFS_SUCCESS)
        {
            if (a_buf)
            {
                memcpy(a_buf, &pifs.cache_page_buf[a_page_offset], a_buf_size);
            }
            pifs.cache_page_buf_address.block_address = a_block_address;
            pifs.cache_page_buf_address.page_address = a_page_address;
        }
    }

    return ret;
}

/**
 * @brief pifs_write  Cached write.
 *
 * @param[in] a_block_address   Block address of page to write.
 * @param[in] a_page_address    Page address of page to write.
 * @param[in] a_page_offset     Offset in page.
 * @param[in] a_buf             Pointer to buffer to write or NULL if
 *                              pifs.cache_page_buf is directly written.
 * @param[in] a_buf_size        Size of buffer. Ignored if a_buf is NULL.
 * @return PIFS_SUCCESS if data write successfully.
 */
pifs_status_t pifs_write(pifs_block_address_t a_block_address,
                         pifs_page_address_t a_page_address,
                         pifs_page_offset_t a_page_offset,
                         const void * const a_buf,
                         pifs_size_t a_buf_size)
{
    pifs_status_t ret = PIFS_ERROR;

    if (a_block_address == pifs.cache_page_buf_address.block_address
            && a_page_address == pifs.cache_page_buf_address.page_address)
    {
        /* Cache hit */
        if (a_buf)
        {
            memcpy(&pifs.cache_page_buf[a_page_offset], a_buf, a_buf_size);
        }
        pifs.cache_page_buf_is_dirty = TRUE;
        ret = PIFS_SUCCESS;
    }
    else
    {
        /* Cache miss, first flush cache */
        ret = pifs_flush();

        if (ret == PIFS_SUCCESS)
        {
            if (a_page_offset != 0 || a_buf_size != PIFS_FLASH_PAGE_SIZE_BYTE)
            {
                /* Only part of page is written */
                ret = pifs_flash_read(a_block_address, a_page_address, 0,
                                      pifs.cache_page_buf, PIFS_FLASH_PAGE_SIZE_BYTE);
            }

            if (a_buf)
            {
                memcpy(&pifs.cache_page_buf[a_page_offset], a_buf, a_buf_size);
            }
            pifs.cache_page_buf_address.block_address = a_block_address;
            pifs.cache_page_buf_address.page_address = a_page_address;
            pifs.cache_page_buf_is_dirty = TRUE;
        }
    }

    return ret;
}

/**
 * @brief pifs_erase  Cached erase.
 *
 * @param[in] a_block_address   Block address of page to erase.
 * @return PIFS_SUCCESS if data erased successfully.
 */
pifs_status_t pifs_erase(pifs_block_address_t a_block_address)
{
    pifs_status_t ret = PIFS_ERROR;

    ret = pifs_flash_erase(a_block_address);

    if (a_block_address == pifs.cache_page_buf_address.block_address)
    {
        /* If the block was erased which contains the cached page, simply forget it */
        pifs.cache_page_buf_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
        pifs.cache_page_buf_address.page_address = PIFS_PAGE_ADDRESS_INVALID;
        pifs.cache_page_buf_is_dirty = FALSE;
    }

    return ret;
}

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
    pifs_size_t          file_pos[PIFS_OPEN_FILE_NUM_MAX];

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
 * @brief pifs_header_init Initialize file system's header.
 *
 * @param a_block_address[in]   Block address of header.
 * @param a_page_address[in]    Page address of header.
 * @param a_header[out]         Pointer to the header to be initialized.
 */
pifs_status_t pifs_header_init(pifs_block_address_t a_block_address,
                               pifs_page_address_t a_page_address,
                               pifs_block_address_t a_next_mgmt_block_address,
                               pifs_header_t * a_header)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_size_t          i = 0;
    pifs_block_address_t ba = a_block_address;
    /* FIXME use random block? */
    PIFS_DEBUG_MSG("Creating managamenet block %s\r\n", pifs_ba_pa2str(a_block_address, a_page_address));
    a_header->magic = PIFS_MAGIC;
#if ENABLE_PIFS_VERSION
    a_header->majorVersion = PIFS_MAJOR_VERSION;
    a_header->minorVersion = PIFS_MINOR_VERSION;
#endif
    if (a_next_mgmt_block_address == PIFS_BLOCK_ADDRESS_ERASED)
    {
        a_header->counter++;
    }
    a_header->entry_list_address.block_address = ba;
    a_header->entry_list_address.page_address = a_page_address + PIFS_HEADER_SIZE_PAGE;
    if (a_header->entry_list_address.page_address + PIFS_ENTRY_LIST_SIZE_PAGE >= PIFS_FLASH_PAGE_PER_BLOCK)
    {
        ba += (a_header->entry_list_address.page_address + PIFS_ENTRY_LIST_SIZE_PAGE + PIFS_FLASH_PAGE_PER_BLOCK - 1) / PIFS_FLASH_PAGE_PER_BLOCK;
    }
    a_header->free_space_bitmap_address.block_address = ba;
    a_header->free_space_bitmap_address.page_address = a_header->entry_list_address.page_address + PIFS_ENTRY_LIST_SIZE_PAGE;
    if (a_header->free_space_bitmap_address.page_address + PIFS_FREE_SPACE_BITMAP_SIZE_PAGE >= PIFS_FLASH_PAGE_PER_BLOCK)
    {
        ba += (a_header->free_space_bitmap_address.page_address + PIFS_FREE_SPACE_BITMAP_SIZE_PAGE + PIFS_FLASH_PAGE_PER_BLOCK - 1) / PIFS_FLASH_PAGE_PER_BLOCK;
    }
    a_header->delta_map_address.block_address = ba;
    a_header->delta_map_address.page_address = a_header->free_space_bitmap_address.page_address + PIFS_FREE_SPACE_BITMAP_SIZE_PAGE;
    ba = a_block_address;
#if PIFS_MANAGEMENT_BLOCKS > 1
    for (i = 0; i < PIFS_MANAGEMENT_BLOCKS; i++)
#endif
    {
        a_header->management_blocks[i] = ba;
        ba++;
#if PIFS_MANAGEMENT_BLOCKS > 1
        if (ba >= PIFS_FLASH_BLOCK_NUM_ALL)
        {
            ba = PIFS_FLASH_BLOCK_RESERVED_NUM;
        }
#endif
    }
#if PIFS_MANAGEMENT_BLOCKS > 1
    for (i = 0; i < PIFS_MANAGEMENT_BLOCKS; i++)
#endif
    {
        if (a_next_mgmt_block_address != PIFS_BLOCK_ADDRESS_ERASED)
        {
            a_header->next_management_blocks[i] = ba;
#if PIFS_MANAGEMENT_BLOCKS > 1
            ba++;
            if (ba >= PIFS_FLASH_BLOCK_NUM_ALL)
            {
                ba = PIFS_FLASH_BLOCK_RESERVED_NUM;
            }
#endif
        }
        else
        {
            a_header->next_management_blocks[i] = PIFS_BLOCK_ADDRESS_ERASED;
        }
    }
    if (a_next_mgmt_block_address != PIFS_BLOCK_ADDRESS_ERASED)
    {
        a_header->checksum = pifs_calc_header_checksum(a_header);
    }
    else
    {
        a_header->checksum = PIFS_CHECKSUM_ERASED;
    }

    return ret;
}

/**
 * @brief pifs_header_write Write file system header.
 *
 * @param a_block_address[in]   Block address of header.
 * @param a_page_address[in]    Page address of header.
 * @param a_header[in]          Pointer to the header.
 * @return PIFS_SUCCESS if header successfully written.
 */
pifs_status_t pifs_header_write(pifs_block_address_t a_block_address,
                                pifs_page_address_t a_page_address,
                                pifs_header_t * a_header,
                                bool_t a_mark_pages)
{
    pifs_status_t ret = PIFS_ERROR;
    pifs_size_t   free_management_bytes;
    pifs_size_t   free_data_bytes;
    pifs_size_t   free_management_pages;
    pifs_size_t   free_data_pages;

    ret = pifs_write(a_block_address, a_page_address, 0, a_header, sizeof(pifs_header_t));
    if (ret == PIFS_SUCCESS)
    {
        pifs.is_header_found = TRUE;
        pifs.header_address.block_address = a_block_address;
        pifs.header_address.page_address = a_page_address;
    }
    if (a_mark_pages)
    {
        if (ret == PIFS_SUCCESS)
        {
            /* Mark file system header as used */
            ret = pifs_mark_page(a_block_address,
                                 a_page_address,
                                 PIFS_HEADER_SIZE_PAGE, TRUE);
        }
        /* FIXME this may not be the right function to mark entry list and
         * free space bitmap. */
        if (ret == PIFS_SUCCESS)
        {
            /* Mark entry list as used */
            ret = pifs_mark_page(a_header->entry_list_address.block_address,
                                 a_header->entry_list_address.page_address,
                                 PIFS_ENTRY_LIST_SIZE_PAGE, TRUE);
        }
        if (ret == PIFS_SUCCESS)
        {
            /* Mark free space bitmap as used */
            ret = pifs_mark_page(a_header->free_space_bitmap_address.block_address,
                                 a_header->free_space_bitmap_address.page_address,
                                 PIFS_FREE_SPACE_BITMAP_SIZE_PAGE, TRUE);
        }
        if (ret == PIFS_SUCCESS)
        {
            /* Mark delta page map as used */
            ret = pifs_mark_page(a_header->delta_map_address.block_address,
                                 a_header->delta_map_address.page_address,
                                 PIFS_DELTA_MAP_PAGE_NUM, TRUE);
        }
    }
    PIFS_INFO_MSG("Counter: %i\r\n",
                  a_header->counter);
    PIFS_INFO_MSG("Entry list at %s\r\n",
                  pifs_address2str(&a_header->entry_list_address));
    PIFS_INFO_MSG("Free space bitmap at %s\r\n",
                  pifs_address2str(&a_header->free_space_bitmap_address));
    PIFS_INFO_MSG("Delta page map at %s\r\n",
                  pifs_address2str(&a_header->delta_map_address));
    pifs_get_free_space(&free_management_bytes, &free_data_bytes,
                        &free_management_pages, &free_data_pages);
    PIFS_INFO_MSG("Free data area:                     %lu bytes, %lu pages\r\n",
                  free_data_bytes, free_data_pages);
    PIFS_INFO_MSG("Free management area:               %lu bytes, %lu pages\r\n",
                  free_management_bytes, free_management_pages);

    return ret;
}

/**
 * @brief pifs_init Initialize flash driver and file system.
 *
 * @return PIFS_SUCCESS if FS successfully initialized.
 */
pifs_status_t pifs_init(void)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba;
    pifs_page_address_t  pa;
    pifs_header_t        header;
    pifs_header_t        prev_header;
    pifs_checksum_t      checksum;
    pifs_size_t          free_management_bytes;
    pifs_size_t          free_data_bytes;
    pifs_size_t          free_management_pages;
    pifs_size_t          free_data_pages;

    pifs.header_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
    pifs.header_address.page_address = PIFS_PAGE_ADDRESS_INVALID;
    pifs.is_header_found = FALSE;
    pifs.cache_page_buf_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
    pifs.cache_page_buf_address.page_address = PIFS_PAGE_ADDRESS_INVALID;
    pifs.cache_page_buf_is_dirty = FALSE;

    if (PIFS_ENTRY_SIZE_BYTE > PIFS_FLASH_PAGE_SIZE_BYTE)
    {
        PIFS_ERROR_MSG("Entry size (%lu) is larger than flash page (%u)!\r\n"
                       "Change PIFS_FILENAME_LEN_MAX to %lu!\r\n",
                       PIFS_ENTRY_SIZE_BYTE, PIFS_FLASH_PAGE_SIZE_BYTE,
                       PIFS_FILENAME_LEN_MAX - (PIFS_ENTRY_SIZE_BYTE - PIFS_FLASH_PAGE_SIZE_BYTE));
        ret = PIFS_ERROR_CONFIGURATION;
    }

    PIFS_INFO_MSG("Geometry of flash memory\r\n");
    PIFS_INFO_MSG("------------------------\r\n");
    PIFS_INFO_MSG("Size of flash memory (all):         %i bytes, %i KiB\r\n", PIFS_FLASH_SIZE_BYTE_ALL, PIFS_FLASH_SIZE_BYTE_ALL / 1024);
    PIFS_INFO_MSG("Size of flash memory (used by FS):  %i bytes, %i KiB\r\n", PIFS_FLASH_SIZE_BYTE_FS, PIFS_FLASH_SIZE_BYTE_ALL / 1024);
    PIFS_INFO_MSG("Size of block:                      %i bytes\r\n", PIFS_FLASH_BLOCK_SIZE_BYTE);
    PIFS_INFO_MSG("Size of page:                       %i bytes\r\n", PIFS_FLASH_PAGE_SIZE_BYTE);
    PIFS_INFO_MSG("Number of blocks (all):             %i\r\n", PIFS_FLASH_BLOCK_NUM_ALL);
    PIFS_INFO_MSG("Number of blocks (used by FS)):     %i\r\n", PIFS_FLASH_BLOCK_NUM_FS);
    PIFS_INFO_MSG("Number of pages/block:              %i\r\n", PIFS_FLASH_PAGE_PER_BLOCK);
    PIFS_INFO_MSG("Number of pages (all):              %i\r\n", PIFS_FLASH_PAGE_NUM_ALL);
    PIFS_INFO_MSG("Number of pages (used by FS)):      %i\r\n", PIFS_FLASH_PAGE_NUM_FS);
    PIFS_INFO_MSG("\r\n");
    PIFS_INFO_MSG("Geometry of file system\r\n");
    PIFS_INFO_MSG("-----------------------\r\n");
    PIFS_INFO_MSG("Block address size:                 %lu bytes\r\n", sizeof(pifs_block_address_t));
    PIFS_INFO_MSG("Page address size:                  %lu bytes\r\n", sizeof(pifs_page_address_t));
    PIFS_INFO_MSG("Header size:                        %lu bytes, %lu pages\r\n", PIFS_HEADER_SIZE_BYTE, PIFS_HEADER_SIZE_PAGE);
    PIFS_INFO_MSG("Entry size:                         %lu bytes\r\n", PIFS_ENTRY_SIZE_BYTE);
    PIFS_INFO_MSG("Entry size in a page:               %lu bytes\r\n", PIFS_ENTRY_SIZE_BYTE * PIFS_ENTRY_PER_PAGE);
    PIFS_INFO_MSG("Entry list size:                    %lu bytes, %lu pages\r\n", PIFS_ENTRY_LIST_SIZE_BYTE, PIFS_ENTRY_LIST_SIZE_PAGE);
    PIFS_INFO_MSG("Free space bitmap size:             %u bytes, %u pages\r\n", PIFS_FREE_SPACE_BITMAP_SIZE_BYTE, PIFS_FREE_SPACE_BITMAP_SIZE_PAGE);
    PIFS_INFO_MSG("Map header size:                    %lu bytes\r\n", PIFS_MAP_HEADER_SIZE_BYTE);
    PIFS_INFO_MSG("Map entry size:                     %lu bytes\r\n", PIFS_MAP_ENTRY_SIZE_BYTE);
    PIFS_INFO_MSG("Number of map entries/page:         %lu\r\n", PIFS_MAP_ENTRY_PER_PAGE);
//    PIFS_INFO_MSG("Delta header size:                  %lu bytes\r\n", PIFS_DELTA_HEADER_SIZE_BYTE);
    PIFS_INFO_MSG("Delta entry size:                   %lu bytes\r\n", PIFS_DELTA_ENTRY_SIZE_BYTE);
    PIFS_INFO_MSG("Number of delta entries/page:       %lu\r\n", PIFS_DELTA_ENTRY_PER_PAGE);
    PIFS_INFO_MSG("Number of delta entries:            %lu\r\n", PIFS_DELTA_ENTRY_PER_PAGE * PIFS_DELTA_MAP_PAGE_NUM);
    PIFS_INFO_MSG("Delta map size:                     %u bytes, %u pages\r\n", PIFS_DELTA_MAP_PAGE_NUM * PIFS_FLASH_PAGE_SIZE_BYTE, PIFS_DELTA_MAP_PAGE_NUM);
    PIFS_INFO_MSG("Full reserved area for management:  %i bytes, %i pages\r\n",
                   PIFS_MANAGEMENT_BLOCKS * 2 * PIFS_FLASH_BLOCK_SIZE_BYTE,
                   PIFS_MANAGEMENT_BLOCKS * 2 * PIFS_FLASH_PAGE_PER_BLOCK);
    PIFS_INFO_MSG("Size of management area:            %i bytes, %i pages\r\n",
                   PIFS_MANAGEMENT_BLOCKS * PIFS_FLASH_BLOCK_SIZE_BYTE,
                   PIFS_MANAGEMENT_BLOCKS * PIFS_FLASH_PAGE_PER_BLOCK);
    PIFS_INFO_MSG("\r\n");

    if ((PIFS_HEADER_SIZE_PAGE + PIFS_ENTRY_LIST_SIZE_PAGE + PIFS_FREE_SPACE_BITMAP_SIZE_PAGE + PIFS_DELTA_MAP_PAGE_NUM) > PIFS_FLASH_PAGE_PER_BLOCK * PIFS_MANAGEMENT_BLOCKS)
    {
        PIFS_ERROR_MSG("Cannot fit data in management block!\r\n");
        PIFS_ERROR_MSG("Decrease PIFS_ENTRY_NUM_MAX or PIFS_FILENAME_LEN_MAX or PIFS_DELTA_PAGES_NUM!\r\n");
        PIFS_ERROR_MSG("Or increase PIFS_MANAGEMENT_BLOCKS to %lu!\r\n",
                       (PIFS_HEADER_SIZE_PAGE + PIFS_ENTRY_LIST_SIZE_PAGE + PIFS_FREE_SPACE_BITMAP_SIZE_PAGE + PIFS_DELTA_MAP_PAGE_NUM + PIFS_FLASH_PAGE_PER_BLOCK - 1) / PIFS_FLASH_PAGE_PER_BLOCK);
        ret = PIFS_ERROR_CONFIGURATION;
    }

    if (((PIFS_FLASH_PAGE_SIZE_BYTE - (PIFS_ENTRY_PER_PAGE * PIFS_ENTRY_SIZE_BYTE)) / PIFS_ENTRY_PER_PAGE) > 0)
    {
        PIFS_NOTICE_MSG("PIFS_FILENAME_LEN_MAX can be increased by %lu with same entry list size.\r\n",
                        (PIFS_FLASH_PAGE_SIZE_BYTE - (PIFS_ENTRY_PER_PAGE * PIFS_ENTRY_SIZE_BYTE)) / PIFS_ENTRY_PER_PAGE
                        );
    }

    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_flash_init();
    }

    if (ret == PIFS_SUCCESS)
    {
        /* Find latest management block */
        for (ba = PIFS_FLASH_BLOCK_RESERVED_NUM; ba < PIFS_FLASH_BLOCK_NUM_ALL && ret == PIFS_SUCCESS; ba++)
        {
            pa = 0;
            ret = pifs_read(ba, pa, 0, &header, sizeof(header));
            if (ret == PIFS_SUCCESS && header.magic == PIFS_MAGIC
#if ENABLE_PIFS_VERSION
                    && header.majorVersion == PIFS_MAJOR_VERSION
                    && header.minorVersion == PIFS_MINOR_VERSION
#endif
               )
            {
                PIFS_DEBUG_MSG("Management page found: %s\r\n", pifs_ba_pa2str(ba, pa));
                checksum = pifs_calc_header_checksum(&header);
                if (checksum == header.checksum)
                {
                    PIFS_DEBUG_MSG("Checksum is valid\r\n");
                    if (!pifs.is_header_found || prev_header.counter < pifs.header.counter)
                    {
                        pifs.is_header_found = TRUE;
                        pifs.header_address.block_address = ba;
                        pifs.header_address.page_address = pa;
                        memcpy(&prev_header, &header, sizeof(prev_header));
                    }
                }
                else
                {
                    PIFS_WARNING_MSG("Checksum is invalid! Calculated: 0x%02X, read: 0x%02X\r\n",
                            checksum, pifs.header.checksum);
                }
            }
        }

        if (pifs.is_header_found)
        {
            memcpy(&pifs.header, &prev_header, sizeof(pifs.header));
        }
        else
        {
            /* No file system header found, so create brand new one */
            pifs.header.counter = 0;
#if 1
            ba = PIFS_FLASH_BLOCK_RESERVED_NUM;
#else
            ba = PIFS_FLASH_BLOCK_RESERVED_NUM + rand() % (PIFS_FLASH_BLOCK_NUM_FS);
#endif
            pa = 0;
            ret = pifs_header_init(ba, pa, ba + PIFS_MANAGEMENT_BLOCKS, &pifs.header);
            if (ret == PIFS_SUCCESS)
            {
                ret = pifs_erase(ba);
            }
            if (ret == PIFS_SUCCESS)
            {
                ret = pifs_header_write(ba, pa, &pifs.header, TRUE);
            }
        }

        if (pifs.is_header_found)
        {
#if PIFS_DEBUG_LEVEL >= 6
            print_buffer(&pifs.header, sizeof(pifs.header), 0);
#endif
            PIFS_INFO_MSG("Counter: %i\r\n",
                            pifs.header.counter);
            PIFS_INFO_MSG("Entry list at %s\r\n",
                          pifs_address2str(&pifs.header.entry_list_address));
            PIFS_INFO_MSG("Free space bitmap at %s\r\n",
                          pifs_address2str(&pifs.header.free_space_bitmap_address));
            PIFS_INFO_MSG("Delta page map at %s\r\n",
                          pifs_address2str(&pifs.header.delta_map_address));
            pifs_get_free_space(&free_management_bytes, &free_data_bytes,
                                &free_management_pages, &free_data_pages);
            PIFS_INFO_MSG("Free data area:                     %lu bytes, %lu pages\r\n",
                          free_data_bytes, free_data_pages);
            PIFS_INFO_MSG("Free management area:               %lu bytes, %lu pages\r\n",
                          free_management_bytes, free_management_pages);

#if PIFS_DEBUG_LEVEL >= 6
            {
                uint32_t i;
                printf("DATA blocks\r\n");
                for (i = 0; i < PIFS_FLASH_BLOCK_NUM_ALL; i++)
                {
                    printf("%i %i\r\n", i, pifs_is_block_type(i, PIFS_BLOCK_TYPE_DATA, &pifs.header));
                }
                printf("PRIMARY MANAGEMENT blocks\r\n");
                for (i = 0; i < PIFS_FLASH_BLOCK_NUM_ALL; i++)
                {
                    printf("%i %i\r\n", i, pifs_is_block_type(i, PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT, &pifs.header));
                }
                printf("SECONDARY MANAGEMENT blocks\r\n");
                for (i = 0; i < PIFS_FLASH_BLOCK_NUM_ALL; i++)
                {
                    printf("%i %i\r\n", i, pifs_is_block_type(i, PIFS_BLOCK_TYPE_SECONDARY_MANAGEMENT, &pifs.header));
                }
                printf("RESERVED blocks\r\n");
                for (i = 0; i < PIFS_FLASH_BLOCK_NUM_ALL; i++)
                {
                    printf("%i %i\r\n", i, pifs_is_block_type(i, PIFS_BLOCK_TYPE_RESERVED, &pifs.header));
                }
            }
#endif
#if 0
            {
                pifs_block_address_t ba = 2;
                pifs_page_address_t  pa = 0;
                pifs_status_t ret;
                char test_buf[256];
                char test_buf2[256];
                bool_t is_delta;

                fill_buffer(test_buf, sizeof(test_buf), FILL_TYPE_SEQUENCE_WORD, 0x1);
                ret = pifs_write_delta(ba, pa, 0, test_buf, sizeof (test_buf), &is_delta);
                test_buf[0] = 0xff;
                test_buf[1] = 0xff;
                ret = pifs_write_delta(ba, pa, 0, test_buf, sizeof (test_buf), &is_delta);
                ret = pifs_read_delta(ba, pa, 0, test_buf2, sizeof(test_buf2));
                print_buffer(test_buf2, sizeof(test_buf2), 0);
                {
                    pifs_block_address_t ba0;
                    pifs_page_address_t  pa0;
                    pifs_page_count_t    count0;
                    pifs_find_page(1, 1, PIFS_BLOCK_TYPE_DATA, TRUE,
                                   &ba0, &pa0, &count0);
                    PIFS_DEBUG_MSG("Find page %s, count: %i ret: %i\r\n",
                                   pifs_ba_pa2str(ba0, pa0), count0, ret);
                }
                pifs_delete();
                exit(-1);
            }
#endif
        }
    }

    return ret;
}

/**
 * @brief pifs_delete Shut down file system.
 *
 * @return PIFS_SUCCESS if caches successfully flushed and flash I/F deleted.
 */
pifs_status_t pifs_delete(void)
{
    pifs_status_t ret = PIFS_ERROR;

    /* Flush cache */
    ret = pifs_flush();

    ret = pifs_flash_delete();

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
static pifs_status_t pifs_append_entry(pifs_entry_t * a_entry)
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
static pifs_status_t pifs_update_entry(const pifs_char_t * a_name, pifs_entry_t * const a_entry)
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
static pifs_status_t pifs_find_entry(const pifs_char_t * a_name, pifs_entry_t * const a_entry)
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
static pifs_status_t pifs_clear_entry(const pifs_char_t * a_name)
{
    return pifs_find_entry(a_name, NULL);
}

/**
 * @brief pifs_release_file_pages Mark file map and file's pages to be released.
 *
 * @param a_file[in] Pointer of file structure.
 * @return TRUE: if all pages were succesfully marked to be released.
 */
static pifs_status_t pifs_release_file_pages(pifs_file_t * a_file)
{
    pifs_block_address_t    ba = a_file->entry.first_map_address.block_address;
    pifs_page_address_t     pa = a_file->entry.first_map_address.page_address;
    pifs_block_address_t    mba;
    pifs_page_address_t     mpa;
    pifs_page_count_t       page_count;
    pifs_size_t             i;
    bool_t                  erased = FALSE;
    pifs_page_offset_t      po = PIFS_MAP_HEADER_SIZE_BYTE;

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
                    /* Map entry found */
                    mba = a_file->map_entry.address.block_address;
                    mpa = a_file->map_entry.address.page_address;
                    page_count = a_file->map_entry.page_count;
                    if (page_count && page_count < PIFS_PAGE_COUNT_INVALID)
                    {
                        PIFS_DEBUG_MSG("Release map entry %i pages %s\r\n", page_count,
                                       pifs_ba_pa2str(mba, mpa));
                        /* Mark pages to be released */
                        a_file->status = pifs_mark_page(mba, mpa, page_count, FALSE);
                    }
                }
                po += PIFS_MAP_ENTRY_SIZE_BYTE;
            }
        }
        if (a_file->status == PIFS_SUCCESS)
        {
            /* Mark map page to be released */
            /* FIXME If PIFS_MAP_PAGE_NUM > 1, this can cause error?! */
            a_file->status = pifs_mark_page(ba, pa, PIFS_MAP_PAGE_NUM, FALSE);
            /* Jump to the next map page */
            ba = a_file->map_header.next_map_address.block_address;
            pa = a_file->map_header.next_map_address.page_address;
            po = PIFS_MAP_HEADER_SIZE_BYTE;
        }
    } while (ba < PIFS_BLOCK_ADDRESS_INVALID && pa < PIFS_PAGE_ADDRESS_INVALID && a_file->status == PIFS_SUCCESS);

    return a_file->status;
}

/**
 * @brief pifs_read_first_map_entry Read first map's first map entry.
 *
 * @param a_file[in] Pointer to opened file.
 * @return PIFS_SUCCESS if entry is read and valid.
 */
static pifs_status_t pifs_read_first_map_entry(pifs_file_t * a_file)
{
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
 * @param a_file[in] Pointer to opened file.
 * @return PIFS_SUCCESS if entry is read and valid.
 * PIFS_ERROR_END_OF_FILE if end of file reached.
 */
static pifs_status_t pifs_read_next_map_entry(pifs_file_t * a_file)
{
    a_file->map_entry_idx++;
    if (a_file->map_entry_idx >= PIFS_MAP_ENTRY_PER_PAGE)
    {
        a_file->map_entry_idx = 0;
        if (a_file->map_header.next_map_address.block_address < PIFS_BLOCK_ADDRESS_INVALID
                && a_file->map_header.next_map_address.page_address < PIFS_PAGE_ADDRESS_INVALID)
        {
            a_file->actual_map_address = a_file->map_header.next_map_address;
            PIFS_DEBUG_MSG("Next map address %s\r\n",
                           pifs_address2str(&a_file->actual_map_address));
            a_file->status = pifs_read(a_file->actual_map_address.block_address,
                                       a_file->actual_map_address.page_address,
                                       0, &a_file->map_header, PIFS_MAP_HEADER_SIZE_BYTE);
        }
        else
        {
            a_file->status = PIFS_ERROR_END_OF_FILE;
        }
    }
    if (a_file->status == PIFS_SUCCESS)
    {
        a_file->status = pifs_read(a_file->actual_map_address.block_address,
                                   a_file->actual_map_address.page_address,
                                   PIFS_MAP_HEADER_SIZE_BYTE + a_file->map_entry_idx * PIFS_MAP_ENTRY_SIZE_BYTE,
                                   &a_file->map_entry,
                                   PIFS_MAP_ENTRY_SIZE_BYTE);
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
 * @param a_file[in]            Pointer to file to use.
 * @return PIFS_SUCCESS if entry was successfully written.
 */
static pifs_status_t pifs_is_free_map_entry(pifs_file_t * a_file,
                                            bool_t * a_is_free_map_entry)
{
    pifs_block_address_t    ba = a_file->actual_map_address.block_address;
    pifs_page_address_t     pa = a_file->actual_map_address.page_address;
    bool_t                  empty_entry_found = FALSE;
    pifs_map_entry_t      * map_entry = (pifs_map_entry_t*) &pifs.page_buf[PIFS_MAP_HEADER_SIZE_BYTE];
    pifs_size_t             i;

    PIFS_DEBUG_MSG("Actual map address %s\r\n",
                   pifs_address2str(&a_file->actual_map_address));
    a_file->status = pifs_read(ba, pa, 0, pifs.page_buf, PIFS_FLASH_PAGE_SIZE_BYTE);
    if (a_file->status == PIFS_SUCCESS)
    {
        for (i = 0; i < PIFS_MAP_ENTRY_PER_PAGE && !empty_entry_found; i++)
        {
            if (pifs_is_buffer_erased(&map_entry[i], PIFS_MAP_ENTRY_SIZE_BYTE))
            {
                empty_entry_found = TRUE;
            }
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
 * @param a_file[in]            Pointer to file to use.
 * @param a_block_address[in]   Block address of new file pages to added.
 * @param a_page_address[in]    Page address of new file pages to added.
 * @param a_page_count[in]      Number of new file pages.
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

    PIFS_DEBUG_MSG("Actual map address %s\r\n",
                   pifs_address2str(&a_file->actual_map_address));
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
    if (a_file->status == PIFS_ERROR_END_OF_FILE)
    {
        PIFS_DEBUG_MSG("End of map, new map will be created\r\n");
        a_file->status = pifs_find_free_page(PIFS_MAP_PAGE_NUM,
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
            a_file->status = pifs_mark_page(ba, pa, PIFS_MAP_PAGE_NUM, TRUE);
            empty_entry_found = TRUE;
        }
    }
    if (empty_entry_found)
    {
        /* Empty map entry found */
        a_file->map_entry.address.block_address = a_block_address;
        a_file->map_entry.address.page_address = a_page_address;
        a_file->map_entry.page_count = a_page_count;
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

    return a_file->status;
}

/**
 * @brief pifs_internal_open Internally used function to open a file.
 *
 * @param[in] a_file        Pointer to internal file structure.
 * @param[in] a_filename    Pointer to file name.
 * @param[in] a_modes       Pointer to open mode. NULL: open with existing modes.
 */
void pifs_internal_open(pifs_file_t * a_file,
                        const pifs_char_t * a_filename,
                        const pifs_char_t * a_modes)
{
    pifs_entry_t       * entry = &a_file->entry;
    pifs_block_address_t ba = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_page_address_t  pa = PIFS_PAGE_ADDRESS_INVALID;
    pifs_page_count_t    page_count_found = 0;
    pifs_size_t          free_management_pages = 0;
    pifs_size_t          free_data_pages = 0;

    a_file->status = PIFS_SUCCESS;
    a_file->write_pos = 0;
    a_file->write_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
    a_file->write_address.page_address = PIFS_PAGE_ADDRESS_INVALID;
    a_file->read_pos = 0;
    a_file->actual_map_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
    a_file->actual_map_address.page_address = PIFS_PAGE_ADDRESS_INVALID;
    a_file->is_size_changed = FALSE;
    if (a_modes)
    {
        pifs_parse_open_mode(a_file, a_modes);
    }
    if (a_file->status == PIFS_SUCCESS)
    {
        a_file->status = pifs_find_entry(a_filename, entry);
        if (a_file->mode_file_shall_exist && a_file->status == PIFS_SUCCESS)
        {
            PIFS_DEBUG_MSG("Entry of %s found\r\n", a_filename);
#if PIFS_DEBUG_LEVEL >= 6
            print_buffer(entry, sizeof(pifs_entry_t), 0);
#endif
            a_file->is_opened = TRUE;
        }
        if (a_file->mode_create_new_file)
        {
            /* Deliberately avoiding return code */
            (void)pifs_get_free_pages(&free_management_pages, &free_data_pages);
            if (a_file->status == PIFS_SUCCESS && free_data_pages > 0)
            {
                /* File already exist */
                a_file->status = pifs_clear_entry(a_filename); /* FIXME use delta pages! */
                if (a_file->status == PIFS_SUCCESS)
                {
                    /* Mark allocated pages to be released */
                    a_file->status = pifs_release_file_pages(a_file);
                }
                a_file->is_size_changed = TRUE; /* FIXME when delta pages used! */
            }
            else
            {
                /* File does not exists, no problem, we'll create it */
                a_file->status = PIFS_SUCCESS;
            }
            /* Order of steps to create a a_file: */
            /* #1 Find a free page for map of a_file */
            /* #2 Create entry of a_file, which contains the map's address */
            /* #3 Mark map page */
            if (a_file->status == PIFS_SUCCESS)
            {
                a_file->status = pifs_find_free_page(PIFS_MAP_PAGE_NUM,
                                                   PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT,
                                                   &ba, &pa, &page_count_found);
            }
            if (a_file->status == PIFS_SUCCESS)
            {
                PIFS_DEBUG_MSG("Map page: %u free page found %s\r\n", page_count_found, pifs_ba_pa2str(ba, pa));
                memset(entry, PIFS_FLASH_ERASED_BYTE_VALUE, PIFS_ENTRY_SIZE_BYTE);
                strncpy((char*)entry->name, a_filename, PIFS_FILENAME_LEN_MAX);
                entry->attrib = PIFS_ATTRIB_ARCHIVE;
                entry->first_map_address.block_address = ba;
                entry->first_map_address.page_address = pa;
                a_file->status = pifs_append_entry(entry);
                if (a_file->status == PIFS_SUCCESS)
                {
                    PIFS_DEBUG_MSG("Entry created\r\n");
                    a_file->status = pifs_mark_page(ba, pa, PIFS_MAP_PAGE_NUM, TRUE);
                    if (a_file->status == PIFS_SUCCESS)
                    {
                        a_file->is_opened = TRUE;
                    }
                }
                else
                {
                    PIFS_DEBUG_MSG("Cannot create entry!\r\n");
                }
            }
            else
            {
                PIFS_DEBUG_MSG("No free page found!\r\n");
            }
        }
        if (a_file->is_opened)
        {
            a_file->status = pifs_read_first_map_entry(a_file);
            PIFS_ASSERT(a_file->status == PIFS_SUCCESS);
            a_file->read_address = a_file->map_entry.address;
            a_file->read_page_count = a_file->map_entry.page_count;
        }
    }
}

/**
 * @brief pifs_get_file Find an unused file structure (file handle).
 *
 * @param[out] a_file Pointer to pointer of file structure to fill.
 * @return PIFS_SUCCESS if unused file structure found.
 */
pifs_status_t pifs_get_file(pifs_file_t **a_file)
{
    pifs_status_t ret = PIFS_ERROR_NO_MORE_FILE_HANDLE;
    pifs_size_t   i;
    pifs_file_t * file = NULL;

    for (i = 0; i < PIFS_OPEN_FILE_NUM_MAX && ret != PIFS_SUCCESS; i++)
    {
        if (!pifs.file[i].is_used)
        {
            file = &pifs.file[i];
            file->is_used = TRUE;
            *a_file = file;
            ret = PIFS_SUCCESS;
        }
    }

    return ret;
}

/**
 * @brief pifs_fopen Open file, works like fopen().
 * Note: "a", "a+" not handled correctly.
 *
 * @param[in] a_filename    File name to open.
 * @param[in] a_modes       Open mode: "r", "r+", "w", "w+", "a" or "a+".
 * @return Pointer to file if file opened successfully.
 */
P_FILE * pifs_fopen(const pifs_char_t * a_filename, const pifs_char_t * a_modes)
{
    /* TODO search first free element of pifs.file[] */
    pifs_file_t  * file = NULL;
    pifs_status_t  ret;

    /* TODO put return value to 'errno' */
    ret = pifs_get_file(&file);
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_check_filename(a_filename);
    }
    if (ret == PIFS_SUCCESS)
    {
        pifs_internal_open(file, a_filename, a_modes);
    }

    return (P_FILE*) file;
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

/**
 * @brief pifs_fwrite Write to file. Works like fwrite().
 *
 * @param[in] a_data    Pointer to buffer to write.
 * @param[in] a_size    Size of data element to write.
 * @param[in] a_count   Number of data elements to write.
 * @param[in] a_file    Pointer to file.
 * @return Number of data elements written.
 */
pifs_size_t pifs_fwrite(const void * a_data, pifs_size_t a_size, pifs_size_t a_count, P_FILE * a_file)
{
    pifs_file_t        * file = (pifs_file_t*) a_file;
    uint8_t            * data = (uint8_t*) a_data;
    pifs_size_t          data_size = a_size * a_count;
    pifs_size_t          written_size = 0;
    pifs_page_count_t    page_count_needed = 0;
    pifs_page_count_t    page_count_needed_limited = 0;
    pifs_page_count_t    page_count_found;
    pifs_page_count_t    page_cound_found_start;
    pifs_size_t          chunk_size;
    pifs_block_address_t ba = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_block_address_t ba_start = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_page_address_t  pa = PIFS_PAGE_ADDRESS_INVALID;
    pifs_page_address_t  pa_start = PIFS_PAGE_ADDRESS_INVALID;
    pifs_page_offset_t   po = PIFS_PAGE_OFFSET_INVALID;
    bool_t               is_delta = FALSE;

    if (pifs.is_header_found && file && file->is_opened && file->mode_write)
    {
        /* TODO if opened in "a" mode always jump to end of file! */
        po = file->write_pos % PIFS_FLASH_PAGE_SIZE_BYTE;
        /* Check if last page was not fully used up */
        if (po)
        {
            /* There is some space in the last page */
            PIFS_ASSERT(pifs_is_address_valid(&file->write_address));
            chunk_size = PIFS_MIN(data_size, PIFS_FLASH_PAGE_SIZE_BYTE - po);
//            PIFS_DEBUG_MSG("--------> pos: %i po: %i data_size: %i chunk_size: %i\r\n",
//                           file->write_pos, po, data_size, chunk_size);
            file->status = pifs_write(file->write_address.block_address,
                                      file->write_address.page_address,
                                      po, data, chunk_size);
//            pifs_print_cache();
            if (file->status == PIFS_SUCCESS)
            {
                data += chunk_size;
                data_size -= chunk_size;
                written_size += chunk_size;
            }
        }

        if (data_size > 0)
        {
            if (file->status == PIFS_SUCCESS)
            {
                /* Check if merge needed and do it if necessary */
                file->status = pifs_merge_check(a_file);
            }
            if (file->status == PIFS_SUCCESS)
            {
                page_count_needed = (data_size + PIFS_FLASH_PAGE_SIZE_BYTE - 1) / PIFS_FLASH_PAGE_SIZE_BYTE;
                do
                {
                    page_count_needed_limited = page_count_needed;
                    if (page_count_needed_limited >= PIFS_PAGE_COUNT_INVALID)
                    {
                        page_count_needed_limited = PIFS_PAGE_COUNT_INVALID - 1;
                    }
                    file->status = pifs_find_free_page(page_count_needed_limited,
                                                  PIFS_BLOCK_TYPE_DATA,
                                                  &ba, &pa, &page_count_found);
                    PIFS_DEBUG_MSG("%u pages found. %s, status: %i\r\n",
                                   page_count_found, pifs_ba_pa2str(ba, pa), file->status);
                    if (file->status == PIFS_SUCCESS)
                    {
                        ba_start = ba;
                        pa_start = pa;
                        page_cound_found_start = page_count_found;
                        do
                        {
                            if (data_size > PIFS_FLASH_PAGE_SIZE_BYTE)
                            {
                                chunk_size = PIFS_FLASH_PAGE_SIZE_BYTE;
                            }
                            else
                            {
                                chunk_size = data_size;
                            }
                            file->status = pifs_write_delta(ba, pa, 0, data, chunk_size, &is_delta);
                            PIFS_DEBUG_MSG("%s is_delta: %i status: %i\r\n", pifs_ba_pa2str(ba, pa),
                                           is_delta, file->status);
                            /* Save last page's address for future use */
                            file->write_address.block_address = ba;
                            file->write_address.page_address = pa;
                            data += chunk_size;
                            data_size -= chunk_size;
                            written_size += chunk_size;
                            page_count_found--;
                            page_count_needed--;
                            if (page_count_found)
                            {
                                file->status = pifs_inc_ba_pa(&ba, &pa);
                            }
                        } while (page_count_found && file->status == PIFS_SUCCESS);

                        if (file->status == PIFS_SUCCESS && !is_delta)
                        {
                            /* Write pages to file map entry after successfully write */
                            file->status = pifs_append_map_entry(file, ba_start, pa_start, page_cound_found_start);
                            PIFS_ASSERT(file->status == PIFS_SUCCESS);
                        }
                    }
                } while (page_count_needed && file->status == PIFS_SUCCESS);
            }
        }
        file->write_pos += written_size;
        if (file->write_pos > file->entry.file_size
                || file->entry.file_size == PIFS_FILE_SIZE_ERASED)
        {
            file->is_size_changed = TRUE;
            file->entry.file_size = file->write_pos;
            PIFS_DEBUG_MSG("New file size: %i bytes\r\n", file->entry.file_size);
        }
        if (!file->mode_append)
        {
            file->read_pos = file->write_pos;
            file->read_address = file->write_address;
        }
    }

    return written_size;
}

/**
 * @brief pifs_inc_read_address Increment read address for file reading.
 *
 * @param[in] a_file Pointer to the internal file structure.
 */
void pifs_inc_read_address(pifs_file_t * a_file)
{
    //PIFS_DEBUG_MSG("started %s\r\n", pifs_address2str(&a_file->read_address));
    a_file->read_page_count--;
    if (a_file->read_page_count)
    {
        a_file->status = pifs_inc_address(&a_file->read_address);
    }
    else
    {
        a_file->status = pifs_read_next_map_entry(a_file);
        if (a_file->status == PIFS_SUCCESS)
        {
            //      print_buffer(&a_file->map_entry, PIFS_MAP_ENTRY_SIZE_BYTE, 0);
            if (pifs_is_buffer_erased(&a_file->map_entry, PIFS_MAP_ENTRY_SIZE_BYTE))
            {
                a_file->status = PIFS_ERROR_END_OF_FILE;
            }
            else
            {
                a_file->read_address = a_file->map_entry.address;
                a_file->read_page_count = a_file->map_entry.page_count;
            }
        }
        else
        {
            //      PIFS_DEBUG_MSG("status: %i\r\n", a_file->status);
        }
    }
    //  PIFS_DEBUG_MSG("exited %s\r\n", pifs_address2str(&a_file->read_address));
}

/**
 * @brief pifs_fread File read. Works like fread().
 *
 * @param[out] a_data   Pointer to buffer to read.
 * @param[in] a_size    Size of data element to read.
 * @param[in] a_count   Number of data elements to read.
 * @param[in] a_file    Pointer to file.
 * @return Number of data elements read.
 */
pifs_size_t pifs_fread(void * a_data, pifs_size_t a_size, pifs_size_t a_count, P_FILE * a_file)
{
    pifs_file_t        * file = (pifs_file_t*) a_file;
    uint8_t *            data = (uint8_t*) a_data;
    pifs_size_t          read_size = 0;
    pifs_size_t          chunk_size = 0;
    pifs_size_t          data_size = a_size * a_count;
    pifs_size_t          page_count = 0;
    pifs_page_offset_t   po;

    if (pifs.is_header_found && file && file->is_opened && file->mode_read)
    {
        po = file->read_pos % PIFS_FLASH_PAGE_SIZE_BYTE;
        /* Check if last page was not fully read */
        if (po)
        {
            /* There is some data in the last page */
            //          PIFS_DEBUG_MSG("po != 0  %s\r\n", pifs_address2str(&file->read_address));
            PIFS_ASSERT(pifs_is_address_valid(&file->read_address));
            chunk_size = PIFS_MIN(data_size, PIFS_FLASH_PAGE_SIZE_BYTE - po);
            //          PIFS_DEBUG_MSG("--------> pos: %i po: %i data_size: %i chunk_size: %i\r\n",
            //                         file->read_pos, po, data_size, chunk_size);
            file->status = pifs_read_delta(file->read_address.block_address,
                                           file->read_address.page_address,
                                           po, data, chunk_size);
            //pifs_print_cache();
            //          print_buffer(data, chunk_size, 0);
            if (file->status == PIFS_SUCCESS)
            {
                data += chunk_size;
                data_size -= chunk_size;
                read_size += chunk_size;
                if (po + chunk_size >= PIFS_FLASH_PAGE_SIZE_BYTE)
                {
                    pifs_inc_read_address(file);
                }
            }
        }
        if (file->status == PIFS_SUCCESS && data_size > 0)
        {
            page_count = (data_size + PIFS_FLASH_PAGE_SIZE_BYTE - 1) / PIFS_FLASH_PAGE_SIZE_BYTE;
            while (page_count && file->status == PIFS_SUCCESS)
            {
                PIFS_ASSERT(pifs_is_address_valid(&file->read_address));
                chunk_size = PIFS_MIN(data_size, PIFS_FLASH_PAGE_SIZE_BYTE);
                //PIFS_DEBUG_MSG("read %s\r\n", pifs_address2str(&file->read_address));
                file->status = pifs_read_delta(file->read_address.block_address,
                                               file->read_address.page_address,
                                               0, data, chunk_size);
                if (file->status == PIFS_SUCCESS && chunk_size == PIFS_FLASH_PAGE_SIZE_BYTE)
                {
                    pifs_inc_read_address(file);
                }
                data += chunk_size;
                data_size -= chunk_size;
                read_size += chunk_size;
                page_count--;
            }
        }
        file->read_pos += read_size;
        if (!file->mode_append)
        {
            file->write_pos = file->read_pos;
            file->write_address = file->read_address;
        }
    }

    return read_size;
}

/**
 * @brief pifs_fclose Close file. Works like fclose().
 * @param[in] a_file File to close.
 * @return 0 if file close, PIFS_EOF if error occurred.
 */
int pifs_fclose(P_FILE * a_file)
{
    int ret = PIFS_EOF;
    pifs_file_t * file = (pifs_file_t*) a_file;

    if (pifs.is_header_found && file && file->is_opened)
    {
        if (file->mode_write && file->is_size_changed)
        {
            file->status = pifs_update_entry(file->entry.name, &file->entry);
        }
        pifs_flush();
        file->is_opened = FALSE;
        file->is_used = FALSE;
        ret = 0;
    }

    return ret;
}

/**
 * @brief pifs_fseek Seek in opened file.
 *
 * @param[in] a_file    File to seek.
 * @param[in] a_offset  Offset to seek.
 * @param[in] a_origin  Origin of offset. Can be
 *                      PIFS_SEEK_SET, PIFS_SEEK_CUR, PIFS_SEEK_END.
 *                      @see pifs_fseek_origin_t
 * @return 0 if seek was successful. Non-zero if error occured.
 */
int pifs_fseek(P_FILE * a_file, long int a_offset, int a_origin)
{
    int                 ret = PIFS_ERROR;
    pifs_file_t       * file = (pifs_file_t*) a_file;
    pifs_size_t         seek_size = 0;
    pifs_size_t         chunk_size = 0;
    pifs_size_t         data_size = 0;
    pifs_size_t         page_count = 0;
    pifs_page_offset_t  po;
    pifs_size_t         target_pos = 0;

    if (pifs.is_header_found && file && file->is_opened)
    {
        switch (a_origin)
        {
            case PIFS_SEEK_CUR:
                if (a_offset > 0)
                {
                    data_size = a_offset;
                }
                else
                {
                    /* TODO implement better method: */
                    /* if read_pos + a_offset > read_pos / 2 */
                    /* (if position is close to current position) */
                    /* go backward using map header's previous address entry */
                    data_size = file->read_pos + a_offset;
                    pifs_rewind(file);
                }
                target_pos = file->read_pos + a_offset;
                break;
            case PIFS_SEEK_SET:
                if (file->read_pos < a_offset)
                {
                    data_size = a_offset - file->read_pos;
                }
                else
                {
                    data_size = a_offset;
                    pifs_rewind(file);
                }
                target_pos = a_offset;
                break;
            case PIFS_SEEK_END:
                data_size = file->entry.file_size + a_offset;
                if (data_size >= file->read_pos)
                {
                    data_size -= file->read_pos;
                }
                else
                {
                    pifs_rewind(file);
                }
                target_pos = data_size;
                break;
            default:
                break;
        }

        if (data_size > file->entry.file_size)
        {
            if (file->mode_write)
            {
                /* TODO write non-existing data */
            }
            else if (file->mode_read)
            {

            }
        }

        po = file->read_pos % PIFS_FLASH_PAGE_SIZE_BYTE;
        /* Check if last page was not fully read */
        if (po)
        {
            /* There is some data in the last page */
            PIFS_ASSERT(pifs_is_address_valid(&file->read_address));
            chunk_size = PIFS_MIN(data_size, PIFS_FLASH_PAGE_SIZE_BYTE - po);
            if (file->status == PIFS_SUCCESS)
            {
                data_size -= chunk_size;
                seek_size += chunk_size;
                if (po + chunk_size >= PIFS_FLASH_PAGE_SIZE_BYTE)
                {
                    pifs_inc_read_address(file);
                }
            }
        }
        if (file->status == PIFS_SUCCESS && data_size > 0)
        {
            page_count = (data_size + PIFS_FLASH_PAGE_SIZE_BYTE - 1) / PIFS_FLASH_PAGE_SIZE_BYTE;
            while (page_count && file->status == PIFS_SUCCESS)
            {
                PIFS_ASSERT(pifs_is_address_valid(&file->read_address));
                chunk_size = PIFS_MIN(data_size, PIFS_FLASH_PAGE_SIZE_BYTE);
                //PIFS_DEBUG_MSG("read %s\r\n", pifs_address2str(&file->read_address));
                if (file->status == PIFS_SUCCESS && chunk_size == PIFS_FLASH_PAGE_SIZE_BYTE)
                {
                    pifs_inc_read_address(file);
                }
                data_size -= chunk_size;
                seek_size += chunk_size;
                page_count--;
            }
        }
        file->read_pos += seek_size;
        if (!file->mode_append)
        {
            file->write_pos = file->read_pos;
            file->write_address = file->read_address;
        }
        ret = file->status;
    }

    return ret;
}

/**
 * @brief pifs_rewind Set file positions to zero.
 *
 * @param[in] a_file Pointer to file.
 */
void pifs_rewind(P_FILE * a_file)
{
    pifs_file_t * file = (pifs_file_t*) a_file;

    if (pifs.is_header_found && file && file->is_opened)
    {
        file->write_pos = 0;
        file->write_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
        file->write_address.page_address = PIFS_PAGE_ADDRESS_INVALID;
        file->read_pos = 0;
        file->actual_map_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
        file->actual_map_address.page_address = PIFS_PAGE_ADDRESS_INVALID;
        file->status = pifs_read_first_map_entry(file);
        PIFS_ASSERT(file->status == PIFS_SUCCESS);
        file->read_address = file->map_entry.address;
        file->read_page_count = file->map_entry.page_count;
    }
}

/**
 * @brief pifs_remove Remove file.
 *
 * @param[in] a_filename Pointer to filename to be removed.
 * @return 0 if file removed. Non-zero if file not found or file name is not valid.
 */
int pifs_remove(const pifs_char_t * a_filename)
{
    int ret;

    ret = pifs_check_filename(a_filename);
    if (ret == PIFS_SUCCESS)
    {
        ret = PIFS_ERROR_FILE_NOT_FOUND;
        pifs_internal_open(&pifs.internal_file, a_filename, "r");
        if (pifs.internal_file.is_opened)
        {
            /* File already exist */
            pifs.internal_file.status = pifs_clear_entry(a_filename);
            if (pifs.internal_file.status == PIFS_SUCCESS)
            {
                /* Mark allocated pages to be released */
                pifs.internal_file.status = pifs_release_file_pages(&pifs.internal_file);
            }
            ret = pifs_fclose(&pifs.internal_file);
        }
    }

    return ret;
}

/**
 * @brief pifs_ferror Return last error of file.
 *
 * @param[in] a_file Pointer to file.
 * @return 0 (PIFS_SUCCESS): if no error occurred.
 */
int pifs_ferror(P_FILE * a_file)
{
    pifs_file_t * file = (pifs_file_t*) a_file;
    int ret = file->status;

    return ret;
}
