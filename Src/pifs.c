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
#include "pifs_entry.h"
#include "pifs_map.h"
#include "pifs_merge.h"
#include "pifs_wear.h"
#include "buffer.h" /* DEBUG */

#define PIFS_DEBUG_LEVEL 2
#include "pifs_debug.h"

pifs_t pifs =
{
    .header_address = { PIFS_BLOCK_ADDRESS_INVALID, PIFS_PAGE_ADDRESS_INVALID },
    .is_header_found = FALSE,
    .is_merging = FALSE,
    .header = { 0 },
    .cache_page_buf_address = { PIFS_BLOCK_ADDRESS_INVALID, PIFS_PAGE_ADDRESS_INVALID },
    .cache_page_buf = { 0 },
    .cache_page_buf_is_dirty = FALSE,
    .file = { { 0 } },
    .internal_file = { 0 },
    .dir = { { 0 } },
    .delta_map_page_buf = { { 0 } },
    .delta_map_page_is_read = FALSE,
    .delta_map_page_is_dirty = FALSE,
    .dmw_page_buf = { 0 },
    .sc_page_buf = { 0 },
};

int pifs_errno = PIFS_SUCCESS;

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
 * @return PIFS_SUCCESS if data written successfully.
 */
pifs_status_t pifs_flush(void)
{
    pifs_status_t ret = PIFS_SUCCESS;
#if PIFS_LOGICAL_PAGE_ENABLED
    pifs_size_t   i;
#endif

    if (pifs.cache_page_buf_is_dirty)
    {
#if PIFS_LOGICAL_PAGE_ENABLED
        for (i = 0; i < PIFS_FLASH_PAGE_PER_LOGICAL_PAGE && ret == PIFS_SUCCESS; i++)
#endif
        {
            ret = pifs_flash_write(pifs.cache_page_buf_address.block_address,
                                   PIFS_LP2FP(pifs.cache_page_buf_address.page_address) + PIFS_LOGICAL_PAGE_IDX(i),
                                   0,
                                   pifs.cache_page_buf + PIFS_LOGICAL_PAGE_IDX(i * PIFS_FLASH_PAGE_SIZE_BYTE),
                                   PIFS_FLASH_PAGE_SIZE_BYTE);
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
    pifs_status_t ret = PIFS_ERROR_GENERAL;
#if PIFS_LOGICAL_PAGE_ENABLED
    pifs_size_t   i;
#endif

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
#if PIFS_LOGICAL_PAGE_ENABLED
            for (i = 0; i < PIFS_FLASH_PAGE_PER_LOGICAL_PAGE && ret == PIFS_SUCCESS; i++)
#endif
            {
                ret = pifs_flash_read(a_block_address,
                                      PIFS_LP2FP(a_page_address) + PIFS_LOGICAL_PAGE_IDX(i),
                                      0,
                                      pifs.cache_page_buf + PIFS_LOGICAL_PAGE_IDX(i * PIFS_FLASH_PAGE_SIZE_BYTE),
                                      PIFS_FLASH_PAGE_SIZE_BYTE);
            }
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
    pifs_status_t ret = PIFS_ERROR_GENERAL;
#if PIFS_LOGICAL_PAGE_ENABLED
    pifs_size_t   i;
#endif

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
            if (a_page_offset != 0 || a_buf_size != PIFS_LOGICAL_PAGE_SIZE_BYTE)
            {
#if PIFS_LOGICAL_PAGE_ENABLED
                for (i = 0; i < PIFS_FLASH_PAGE_PER_LOGICAL_PAGE && ret == PIFS_SUCCESS; i++)
#endif
                {
                    /* Only part of page is written */
                    ret = pifs_flash_read(a_block_address,
                                          PIFS_LP2FP(a_page_address) + PIFS_LOGICAL_PAGE_IDX(i),
                                          0,
                                          pifs.cache_page_buf + PIFS_LOGICAL_PAGE_IDX(i * PIFS_FLASH_PAGE_SIZE_BYTE),
                                          PIFS_FLASH_PAGE_SIZE_BYTE);
                }
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
pifs_status_t pifs_erase(pifs_block_address_t a_block_address, pifs_header_t * a_old_header, pifs_header_t * a_new_header)
{
    pifs_status_t           ret = PIFS_ERROR_GENERAL;

    (void) a_old_header;

    PIFS_DEBUG_MSG("Erasing block %i\r\n", a_block_address)
    ret = pifs_flash_erase(a_block_address);

    if (a_block_address == pifs.cache_page_buf_address.block_address)
    {
        /* If the block was erased which contains the cached page, simply forget it */
        pifs.cache_page_buf_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
        pifs.cache_page_buf_address.page_address = PIFS_PAGE_ADDRESS_INVALID;
        pifs.cache_page_buf_is_dirty = FALSE;
    }

    if (ret == PIFS_SUCCESS && a_new_header)
    {
        /* Increase wear level */
        ret = pifs_inc_wear_level(a_block_address, a_new_header);
    }

    return ret;
}

/**
 * @brief pifs_header_init Initialize file system's header.
 *
 * @param[in] a_block_address   Block address of header.
 * @param[in] a_page_address    Page address of header.
 * @param[out] a_header         Pointer to the header to be initialized.
 */
pifs_status_t pifs_header_init(pifs_block_address_t a_block_address,
                               pifs_page_address_t a_page_address,
                               pifs_block_address_t a_next_mgmt_block_address,
                               pifs_header_t * a_header)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba = a_block_address;
    pifs_address_t       address;
    pifs_size_t          i;

    PIFS_DEBUG_MSG("Creating managamenet block %s\r\n", pifs_ba_pa2str(a_block_address, a_page_address));
    a_header->magic = PIFS_MAGIC;
#if ENABLE_PIFS_VERSION
    a_header->majorVersion = PIFS_MAJOR_VERSION;
    a_header->minorVersion = PIFS_MINOR_VERSION;
#endif
    if (a_next_mgmt_block_address == PIFS_BLOCK_ADDRESS_ERASED)
    {
        a_header->counter++;
        for (i = 0; i < PIFS_LEAST_WEARED_BLOCK_NUM; i++)
        {
            a_header->least_weared_blocks[i].block_address = PIFS_BLOCK_ADDRESS_ERASED;
            a_header->least_weared_blocks[i].wear_level_cntr = PIFS_WEAR_LEVEL_CNTR_MAX;
        }
        a_header->wear_level_cntr_max = PIFS_WEAR_LEVEL_CNTR_MAX;
    }
    address.block_address = a_block_address;
    address.page_address = a_page_address;
    pifs_add_address(&address, PIFS_HEADER_SIZE_PAGE);
    a_header->entry_list_address = address;
    pifs_add_address(&address, PIFS_ENTRY_LIST_SIZE_PAGE);
    a_header->free_space_bitmap_address = address;
    pifs_add_address(&address, PIFS_FREE_SPACE_BITMAP_SIZE_PAGE);
    a_header->delta_map_address = address;
    pifs_add_address(&address, PIFS_DELTA_MAP_PAGE_NUM);
    a_header->wear_level_list_address = address;
    if ((address.block_address - a_block_address) > (pifs_block_address_t)PIFS_MANAGEMENT_BLOCKS)
    {
        /* Not enough space for management pages */
        ret = PIFS_ERROR_CONFIGURATION;
    }
    a_header->management_block_address = ba;
    a_header->next_management_block_address = a_next_mgmt_block_address;
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
 * @param[in] a_block_address   Block address of header.
 * @param[in] a_page_address    Page address of header.
 * @param[in] a_header          Pointer to the header.
 * @param[in] a_mark_pages      TRUE: mark management pages as used ones.
 * @return PIFS_SUCCESS if header successfully written.
 */
pifs_status_t pifs_header_write(pifs_block_address_t a_block_address,
                                pifs_page_address_t a_page_address,
                                pifs_header_t * a_header,
                                bool_t a_mark_pages)
{
    pifs_status_t ret = PIFS_ERROR_GENERAL;
    pifs_size_t   free_management_bytes = 0;
    pifs_size_t   free_data_bytes = 0;
    pifs_size_t   free_management_pages = 0;
    pifs_size_t   free_data_pages = 0;
    pifs_size_t   free_entries = 0;
    pifs_size_t   to_be_released_entries = 0;

    ret = pifs_write(a_block_address, a_page_address, 0, a_header, sizeof(pifs_header_t));
    if (ret == PIFS_SUCCESS)
    {
        pifs.is_header_found = TRUE;
        pifs.header_address.block_address = a_block_address;
        pifs.header_address.page_address = a_page_address;
        if (a_header->counter == 0)
        {
            /* Initialize wear level list for the very first header */
            ret = pifs_wear_level_list_init();
        }
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
        if (ret == PIFS_SUCCESS)
        {
            PIFS_DEBUG_MSG("Marking entry list %s, %i pages\r\n",
                           pifs_address2str(&a_header->entry_list_address),
                           PIFS_ENTRY_LIST_SIZE_PAGE);
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
        if (ret == PIFS_SUCCESS)
        {
            /* Mark wear level list as used */
            ret = pifs_mark_page(a_header->wear_level_list_address.block_address,
                                 a_header->wear_level_list_address.page_address,
                                 PIFS_WEAR_LEVEL_LIST_SIZE_PAGE, TRUE);
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
    PIFS_INFO_MSG("Wear level list at %s\r\n",
                  pifs_address2str(&a_header->wear_level_list_address));
    if (ret == PIFS_SUCCESS)
    {
        /* Deliberately avoiding return code, it is not an error */
        /* if there is no more space */
        (void)pifs_get_free_space(&free_management_bytes, &free_data_bytes,
                                  &free_management_pages, &free_data_pages);
        PIFS_INFO_MSG("Free data area:                     %lu bytes, %lu pages\r\n",
                      free_data_bytes, free_data_pages);
        PIFS_INFO_MSG("Free management area:               %lu bytes, %lu pages\r\n",
                      free_management_bytes, free_management_pages);
        /* Deliberately avoiding return code, it is not an error */
        /* if there is no more space */
        (void)pifs_count_entries(&free_entries, &to_be_released_entries,
                                 pifs.header.entry_list_address.block_address,
                                 pifs.header.entry_list_address.page_address);
        PIFS_NOTICE_MSG("free_entries: %lu, to_be_released_entries: %lu\r\n",
                        free_entries, to_be_released_entries);
    }

    return ret;
}

/**
 * @brief pifs_fs_info Print information about flash memory and filesystem.
 */
void pifs_print_fs_info(void)
{
    printf("Geometry of flash memory\r\n");
    printf("------------------------\r\n");
    printf("Size of flash memory (all):         %i bytes, %i KiB\r\n", PIFS_FLASH_SIZE_BYTE_ALL, PIFS_FLASH_SIZE_BYTE_ALL / 1024);
    printf("Size of flash memory (used by FS):  %i bytes, %i KiB\r\n", PIFS_FLASH_SIZE_BYTE_FS, PIFS_FLASH_SIZE_BYTE_ALL / 1024);
    printf("Size of block:                      %i bytes\r\n", PIFS_FLASH_BLOCK_SIZE_BYTE);
    printf("Size of page:                       %i bytes\r\n", PIFS_FLASH_PAGE_SIZE_BYTE);
    printf("Number of blocks (all):             %i\r\n", PIFS_FLASH_BLOCK_NUM_ALL);
    printf("Number of blocks (used by FS)):     %i\r\n", PIFS_FLASH_BLOCK_NUM_FS);
    printf("Number of pages/block:              %i\r\n", PIFS_FLASH_PAGE_PER_BLOCK);
    printf("Number of pages (all):              %i\r\n", PIFS_FLASH_PAGE_NUM_ALL);
    printf("Number of pages (used by FS)):      %i\r\n", PIFS_FLASH_PAGE_NUM_FS);
    printf("\r\n");
    printf("Geometry of file system\r\n");
    printf("-----------------------\r\n");
    printf("Size of logical page:               %i bytes\r\n", PIFS_LOGICAL_PAGE_SIZE_BYTE);
    printf("Block address size:                 %lu bytes\r\n", sizeof(pifs_block_address_t));
    printf("Page address size:                  %lu bytes\r\n", sizeof(pifs_page_address_t));
    printf("Header size:                        %lu bytes, %lu logical pages\r\n", PIFS_HEADER_SIZE_BYTE, PIFS_HEADER_SIZE_PAGE);
    printf("Entry size:                         %lu bytes\r\n", PIFS_ENTRY_SIZE_BYTE);
    printf("Entry size in a page:               %lu bytes\r\n", PIFS_ENTRY_SIZE_BYTE * PIFS_ENTRY_PER_PAGE);
    printf("Entry list size:                    %lu bytes, %lu logical pages\r\n", PIFS_ENTRY_LIST_SIZE_BYTE, PIFS_ENTRY_LIST_SIZE_PAGE);
    printf("Free space bitmap size:             %u bytes, %u logical pages\r\n", PIFS_FREE_SPACE_BITMAP_SIZE_BYTE, PIFS_FREE_SPACE_BITMAP_SIZE_PAGE);
    printf("Map header size:                    %lu bytes\r\n", PIFS_MAP_HEADER_SIZE_BYTE);
    printf("Map entry size:                     %lu bytes\r\n", PIFS_MAP_ENTRY_SIZE_BYTE);
    printf("Number of map entries/page:         %lu\r\n", PIFS_MAP_ENTRY_PER_PAGE);
    printf("Delta entry size:                   %lu bytes\r\n", PIFS_DELTA_ENTRY_SIZE_BYTE);
    printf("Number of delta entries/page:       %lu\r\n", PIFS_DELTA_ENTRY_PER_PAGE);
    printf("Number of delta entries:            %lu\r\n", PIFS_DELTA_ENTRY_PER_PAGE * PIFS_DELTA_MAP_PAGE_NUM);
    printf("Delta map size:                     %u bytes, %u logical pages\r\n", PIFS_DELTA_MAP_PAGE_NUM * PIFS_LOGICAL_PAGE_SIZE_BYTE, PIFS_DELTA_MAP_PAGE_NUM);
    printf("Wear level entry size:              %lu bytes\r\n", PIFS_WEAR_LEVEL_ENTRY_SIZE_BYTE);
    printf("Number of wear level entries/page:  %lu\r\n", PIFS_WEAR_LEVEL_ENTRY_PER_PAGE);
    printf("Number of wear level entries:       %lu\r\n", PIFS_FLASH_BLOCK_NUM_FS);
    printf("Wear level map size:                %u bytes, %u logical pages\r\n", PIFS_WEAR_LEVEL_LIST_SIZE_BYTE, PIFS_WEAR_LEVEL_LIST_SIZE_PAGE);
    printf("Minimum management area:            %lu logical pages, %lu blocks\r\n", PIFS_MANAGEMENT_PAGES_MIN,
           PIFS_MANAGEMENT_BLOCKS_MIN);
    printf("Recommended management area:        %lu logical pages, %lu blocks\r\n", PIFS_MANAGEMENT_PAGES_RECOMMENDED,
           PIFS_MANAGEMENT_BLOCKS_RECOMMENDED);
    printf("Full reserved area for management:  %i bytes, %i logical pages\r\n",
           PIFS_MANAGEMENT_BLOCKS * 2 * PIFS_FLASH_BLOCK_SIZE_BYTE,
           PIFS_MANAGEMENT_BLOCKS * 2 * PIFS_LOGICAL_PAGE_PER_BLOCK);
    printf("Size of management area:            %i bytes, %i logical pages\r\n",
           PIFS_MANAGEMENT_BLOCKS * PIFS_FLASH_BLOCK_SIZE_BYTE,
           PIFS_MANAGEMENT_BLOCKS * PIFS_LOGICAL_PAGE_PER_BLOCK);
    printf("\r\n");
    printf("File system in RAM:                 %lu bytes\r\n", sizeof(pifs_t));
}

void pifs_print_header_info(void)
{
    printf("Counter: %i\r\n",
           pifs.header.counter);
    printf("Entry list at %s\r\n",
           pifs_address2str(&pifs.header.entry_list_address));
    printf("Free space bitmap at %s\r\n",
           pifs_address2str(&pifs.header.free_space_bitmap_address));
    printf("Delta page map at %s\r\n",
           pifs_address2str(&pifs.header.delta_map_address));
    printf("Wear level list at %s\r\n",
           pifs_address2str(&pifs.header.wear_level_list_address));
}

void pifs_print_free_space_info(void)
{
    size_t        free_management_bytes = 0;
    size_t        free_data_bytes = 0;
    size_t        free_management_pages = 0;
    size_t        free_data_pages = 0;
    size_t        to_be_released_management_bytes = 0;
    size_t        to_be_released_data_bytes = 0;
    size_t        to_be_released_management_pages = 0;
    size_t        to_be_released_data_pages = 0;
    pifs_size_t   free_entries = 0;
    pifs_size_t   to_be_released_entries = 0;
    pifs_status_t ret;

    ret = pifs_get_free_space(&free_management_bytes, &free_data_bytes,
                              &free_management_pages, &free_data_pages);
    if (ret == PIFS_SUCCESS || ret == PIFS_ERROR_NO_MORE_SPACE)
    {
        printf("Free data area:                     %lu bytes, %lu pages\r\n",
               free_data_bytes, free_data_pages);
        printf("Free management area:               %lu bytes, %lu pages\r\n",
               free_management_bytes, free_management_pages);
    }
    ret = pifs_get_to_be_released_space(&to_be_released_management_bytes, &to_be_released_data_bytes,
                                        &to_be_released_management_pages, &to_be_released_data_pages);
    if (ret == PIFS_SUCCESS || ret == PIFS_ERROR_NO_MORE_SPACE)
    {
        printf("To be released data area:           %lu bytes, %lu pages\r\n",
               to_be_released_data_bytes, to_be_released_data_pages);
        printf("To be released management area:     %lu bytes, %lu pages\r\n",
               to_be_released_management_bytes, to_be_released_management_pages);
    }
    if (ret == PIFS_SUCCESS || ret == PIFS_ERROR_NO_MORE_SPACE)
    {
        ret = pifs_count_entries(&free_entries, &to_be_released_entries,
                                 pifs.header.entry_list_address.block_address,
                                 pifs.header.entry_list_address.page_address);
    }
    if (ret == PIFS_SUCCESS || ret == PIFS_ERROR_NO_MORE_SPACE)
    {
        printf("Free entries:                       %lu\r\n", free_entries);
        printf("To be released entries:             %lu\r\n", to_be_released_entries);
    }
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
    pifs_size_t          i;

    pifs.header_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
    pifs.header_address.page_address = PIFS_PAGE_ADDRESS_INVALID;
    pifs.is_header_found = FALSE;
    pifs.cache_page_buf_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
    pifs.cache_page_buf_address.page_address = PIFS_PAGE_ADDRESS_INVALID;
    pifs.cache_page_buf_is_dirty = FALSE;

#if PIFS_DEBUG_LEVEL >= 5
    pifs_print_fs_info();
#endif

    if (PIFS_ENTRY_SIZE_BYTE > PIFS_LOGICAL_PAGE_SIZE_BYTE)
    {
        PIFS_ERROR_MSG("Entry size (%lu) is larger than flash page (%u)!\r\n"
                       "Change PIFS_FILENAME_LEN_MAX to %lu!\r\n",
                       PIFS_ENTRY_SIZE_BYTE, PIFS_LOGICAL_PAGE_SIZE_BYTE,
                       PIFS_FILENAME_LEN_MAX - (PIFS_ENTRY_SIZE_BYTE - PIFS_LOGICAL_PAGE_SIZE_BYTE));
        ret = PIFS_ERROR_CONFIGURATION;
    }

    if (PIFS_MANAGEMENT_BLOCKS_MIN > PIFS_MANAGEMENT_BLOCKS)
    {
        PIFS_ERROR_MSG("Cannot fit data in management block!\r\n");
        PIFS_ERROR_MSG("Decrease PIFS_ENTRY_NUM_MAX or PIFS_FILENAME_LEN_MAX or PIFS_DELTA_PAGES_NUM!\r\n");
        PIFS_ERROR_MSG("Or increase PIFS_MANAGEMENT_BLOCKS to %lu!\r\n",
                       PIFS_MANAGEMENT_BLOCKS_MIN);
        ret = PIFS_ERROR_CONFIGURATION;
    }

    if (PIFS_MANAGEMENT_BLOCKS_RECOMMENDED > PIFS_MANAGEMENT_BLOCKS)
    {
        PIFS_WARNING_MSG("Recommended PIFS_MANAGEMENT_BLOCKS is %lu!\r\n", PIFS_MANAGEMENT_BLOCKS_RECOMMENDED);
    }

    if (((PIFS_LOGICAL_PAGE_SIZE_BYTE - (PIFS_ENTRY_PER_PAGE * PIFS_ENTRY_SIZE_BYTE)) / PIFS_ENTRY_PER_PAGE) > 0)
    {
        PIFS_NOTICE_MSG("PIFS_FILENAME_LEN_MAX can be increased by %lu with same entry list size.\r\n",
                        (PIFS_LOGICAL_PAGE_SIZE_BYTE - (PIFS_ENTRY_PER_PAGE * PIFS_ENTRY_SIZE_BYTE)) / PIFS_ENTRY_PER_PAGE
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
                    if (pifs.is_header_found)
                    {
                        PIFS_WARNING_MSG("Previous management page was not erased! Erasing...\r\n");
                        /* This can happen when pifs_merge() was interrupted before step #11 */
                        /* Erase old management area */
                        for (i = 0; i < PIFS_MANAGEMENT_BLOCKS && ret == PIFS_SUCCESS; i++)
                        {
                            ret = pifs_erase(prev_header.management_block_address + i, &prev_header, &header);
                        }
                        if (ret == PIFS_SUCCESS)
                        {
                            PIFS_WARNING_MSG("Done.\r\n");
                        }
                    }
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
            PIFS_WARNING_MSG("No file system header found, creating...\r\n");
            pifs.header.counter = 0;
            /* FIXME use random block? */
#if 1
            ba = PIFS_FLASH_BLOCK_RESERVED_NUM;
#else
            ba = PIFS_FLASH_BLOCK_RESERVED_NUM + rand() % (PIFS_FLASH_BLOCK_NUM_FS);
#endif
            pa = 0;
            ret = pifs_header_init(ba, pa, ba + PIFS_MANAGEMENT_BLOCKS, &pifs.header);
            if (ret == PIFS_SUCCESS)
            {
                PIFS_WARNING_MSG("Erasing all blocks...\r\n");
                for (i = PIFS_FLASH_BLOCK_RESERVED_NUM; i < PIFS_FLASH_BLOCK_NUM_ALL; i++)
                {
                    ret = pifs_flash_erase(i);
                    /* TODO mark bad blocks */
                }
                PIFS_WARNING_MSG("Done.\r\n");
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
#if PIFS_DEBUG_LEVEL >= 5
            pifs_print_header_info();
            pifs_print_free_space_info();
#endif

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
    pifs_status_t ret = PIFS_ERROR_GENERAL;

    /* Flush cache */
    ret = pifs_flush();

    ret = pifs_flash_delete();

    return ret;
}

/**
 * @brief pifs_internal_open Internally used function to open a file.
 *
 * @param[in] a_file                Pointer to internal file structure.
 * @param[in] a_filename            Pointer to file name.
 * @param[in] a_modes               Pointer to open mode. NULL: open with existing modes.
 * @param[in] a_is_merge_allowed    TRUE: merge can be started.
 */
pifs_status_t pifs_internal_open(pifs_file_t * a_file,
                                 const pifs_char_t * a_filename,
                                 const pifs_char_t * a_modes,
                                 bool_t a_is_merge_allowed)
{
    pifs_entry_t       * entry = &a_file->entry;
    pifs_block_address_t ba = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_page_address_t  pa = PIFS_PAGE_ADDRESS_INVALID;
    pifs_page_count_t    page_count_found = 0;

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
        a_file->status = pifs_find_entry(a_filename, entry,
                                         pifs.header.entry_list_address.block_address,
                                         pifs.header.entry_list_address.page_address);
        if ((a_file->mode_file_shall_exist || a_file->mode_append) && a_file->status == PIFS_SUCCESS)
        {
            PIFS_DEBUG_MSG("Entry of %s found\r\n", a_filename);
#if PIFS_DEBUG_LEVEL >= 6
            print_buffer(entry, sizeof(pifs_entry_t), 0);
#endif
#if PIFS_ENABLE_DIRECTORIES
            if (!(a_file->entry.attrib & PIFS_ATTRIB_DIR))
#endif
            {
                /* Check if file size is valid or file is opened during merge */
                if ((a_file->entry.file_size < PIFS_FILE_SIZE_ERASED && a_is_merge_allowed)
                        || !a_is_merge_allowed)
                {
                    a_file->is_opened = TRUE;
                }
                else
                {
                    PIFS_DEBUG_MSG("File size of %s is invalid!\r\n", a_filename);
                    a_file->status = PIFS_ERROR_FILE_NOT_FOUND;
                }
            }
#if PIFS_ENABLE_DIRECTORIES
            else
            {
                a_file->status = PIFS_ERROR_IS_A_DIRECTORY;
            }
#endif
        }
        if (a_file->mode_create_new_file || (a_file->mode_append && !a_file->is_opened))
        {
            if (a_file->status == PIFS_SUCCESS)
            {
                /* File already exist */
#if PIFS_USE_DELTA_FOR_ENTRIES == 0
                a_file->status = pifs_clear_entry(a_filename,
                                                  pifs.header.entry_list_address.block_address,
                                                  pifs.header.entry_list_address.page_address);
                if (a_file->status == PIFS_SUCCESS)
#endif
                {
                    /* Mark allocated pages to be released */
                    a_file->status = pifs_release_file_pages(a_file);
                    a_file->is_size_changed = TRUE;
                }
            }
            else
            {
                /* File does not exists, no problem, we'll create it */
                a_file->status = PIFS_SUCCESS;
            }
            if (a_file->status == PIFS_SUCCESS && a_is_merge_allowed)
            {
                a_file->status = pifs_merge_check(NULL, 1);
            }
            /* Order of steps to create a file: */
            /* #1 Find a free page for map of file */
            /* #2 Create entry of a_file, which contains the map's address */
            /* #3 Mark map page */
            if (a_file->status == PIFS_SUCCESS)
            {
                a_file->status = pifs_find_free_page_wl(PIFS_MAP_PAGE_NUM, PIFS_MAP_PAGE_NUM,
                                                        PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT,
                                                        &ba, &pa, &page_count_found);
            }
            if (a_file->status == PIFS_SUCCESS)
            {
                PIFS_DEBUG_MSG("Map page: %u free page found %s\r\n", page_count_found, pifs_ba_pa2str(ba, pa));
                memset(entry, PIFS_FLASH_ERASED_BYTE_VALUE, PIFS_ENTRY_SIZE_BYTE);
                strncpy((char*)entry->name, a_filename, PIFS_FILENAME_LEN_MAX);
#if PIFS_ENABLE_ATTRIBUTES
                entry->attrib = PIFS_ATTRIB_ARCHIVE;
#endif
                entry->first_map_address.block_address = ba;
                entry->first_map_address.page_address = pa;
                a_file->status = pifs_append_entry(entry,
                                                   pifs.header.entry_list_address.block_address,
                                                   pifs.header.entry_list_address.page_address);
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
                    PIFS_SET_ERRNO(PIFS_ERROR_NO_MORE_ENTRY);
                }
            }
            else
            {
                PIFS_DEBUG_MSG("No free page found!\r\n");
                PIFS_SET_ERRNO(PIFS_ERROR_NO_MORE_SPACE);
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

    return a_file->status;
}

/**
 * @brief pifs_fopen Open file, works like fopen().
 *
 * @param[in] a_filename    File name to open.
 * @param[in] a_modes       Open mode: "r", "r+", "w", "w+", "a" or "a+".
 * @return Pointer to file if file opened successfully.
 */
P_FILE * pifs_fopen(const pifs_char_t * a_filename, const pifs_char_t * a_modes)
{
    pifs_file_t  * file = NULL;
    pifs_status_t  ret;

    PIFS_NOTICE_MSG("filename: '%s' modes: %s\r\n", a_filename, a_modes);
    ret = pifs_get_file(&file);
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_check_filename(a_filename);
    }
    if (ret == PIFS_SUCCESS)
    {
        (void)pifs_internal_open(file, a_filename, a_modes, TRUE);
        PIFS_NOTICE_MSG("status: %i is_opened: %i\r\n", file->status, file->is_opened);
        if (file->status == PIFS_SUCCESS && file->is_opened)
        {
            PIFS_NOTICE_MSG("file size: %i\r\n", file->entry.file_size);
        }
        else
        {
            file = NULL;
        }
    }

    PIFS_SET_ERRNO(ret);
    return (P_FILE*) file;
}

/**
 * @brief pifs_inc_write_address Increment write address for file writing.
 *
 * @param[in] a_file Pointer to the internal file structure.
 */
pifs_status_t pifs_inc_write_address(pifs_file_t * a_file)
{
    //PIFS_DEBUG_MSG("started %s\r\n", pifs_address2str(&a_file->write_address));
    a_file->write_page_count--;
    if (a_file->write_page_count)
    {
        a_file->status = pifs_inc_address(&a_file->write_address);
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
                a_file->write_address = a_file->map_entry.address;
                a_file->write_page_count = a_file->map_entry.page_count;
            }
        }
        else
        {
            //      PIFS_DEBUG_MSG("status: %i\r\n", a_file->status);
        }
    }
    //  PIFS_DEBUG_MSG("exited %s\r\n", pifs_address2str(&a_file->write_address));
    return a_file->status;
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
size_t pifs_fwrite(const void * a_data, size_t a_size, size_t a_count, P_FILE * a_file)
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
    bool_t               is_free_map_entry;
    pifs_size_t          free_management_page_count = 0;
    pifs_size_t          free_data_page_count = 0;

    PIFS_NOTICE_MSG("filename: '%s', size: %i, count: %i\r\n", file->entry.name, a_size, a_count);
    if (pifs.is_header_found && file && file->is_opened && file->mode_write)
    {
        file->status = PIFS_SUCCESS;
        /* If opened in "a" mode always jump to end of file */
        if (file->mode_append && file->write_pos != file->entry.file_size)
        {
            pifs_fseek(file, file->entry.file_size, PIFS_SEEK_SET);
        }
        if (file->status == PIFS_SUCCESS)
        {
            po = file->write_pos % PIFS_LOGICAL_PAGE_SIZE_BYTE;
            /* Check if last page was not fully used up */
            if (po && file->write_pos == file->entry.file_size)
            {
                /* There is some space in the last page */
                PIFS_ASSERT(pifs_is_address_valid(&file->write_address));
                chunk_size = PIFS_MIN(data_size, PIFS_LOGICAL_PAGE_SIZE_BYTE - po);
                //PIFS_DEBUG_MSG("--------> pos: %i po: %i data_size: %i chunk_size: %i\r\n",
                //               file->write_pos, po, data_size, chunk_size);
                file->status = pifs_write(file->write_address.block_address,
                                          file->write_address.page_address,
                                          po, data, chunk_size);
                //pifs_print_cache();
                if (file->status == PIFS_SUCCESS)
                {
                    data += chunk_size;
                    data_size -= chunk_size;
                    written_size += chunk_size;
                }
            }
        }

        if (data_size > 0)
        {
            if (file->status == PIFS_SUCCESS)
            {
                page_count_needed = (data_size + PIFS_LOGICAL_PAGE_SIZE_BYTE - 1) / PIFS_LOGICAL_PAGE_SIZE_BYTE;
                /* Check if merge needed and do it if necessary */
                file->status = pifs_merge_check(a_file, page_count_needed);
            }
            if (file->status == PIFS_SUCCESS && file->entry.file_size != PIFS_FILE_SIZE_ERASED
                    && file->write_pos < file->entry.file_size)
            {
                /* Overwriting existing pages in the file */
                PIFS_WARNING_MSG("Overwriting pages\r\n");
                //page_count_needed = (data_size + PIFS_LOGICAL_PAGE_SIZE_BYTE - 1) / PIFS_LOGICAL_PAGE_SIZE_BYTE;
                while (page_count_needed && file->status == PIFS_SUCCESS)
                {
                    chunk_size = PIFS_MIN(data_size, PIFS_LOGICAL_PAGE_SIZE_BYTE);
                    if (file->write_pos + chunk_size > file->entry.file_size)
                    {
                        chunk_size = file->entry.file_size - file->write_pos;
                    }
                    PIFS_NOTICE_MSG("write %s, page_count_needed: %i, chunk size: %i\r\n",
                                    pifs_address2str(&file->write_address), page_count_needed, chunk_size);
                    PIFS_ASSERT(pifs_is_address_valid(&file->write_address));
                    file->status = pifs_write_delta(file->write_address.block_address,
                                                   file->write_address.page_address,
                                                   0, data, chunk_size, &is_delta);
                    if (file->status == PIFS_SUCCESS && chunk_size == PIFS_LOGICAL_PAGE_SIZE_BYTE)
                    {
                        pifs_inc_write_address(file);
                    }
                    data += chunk_size;
                    data_size -= chunk_size;
                    written_size += chunk_size;
                    page_count_needed--;
                }
            }
        }
        if (data_size > 0)
        {
            /* Map entry is needed when write position is at file's end */
            if (file->status == PIFS_SUCCESS && file->write_pos == file->entry.file_size)
            {
                /* Check if at least one map can be added after writing */
                file->status = pifs_is_free_map_entry(a_file, &is_free_map_entry);
                if (!is_free_map_entry)
                {
                    /* No free entry in actual map page, so check if there are */
                    /* at least one free management page and a new map page */
                    /* can be allocated. */
                    file->status = pifs_get_free_pages(&free_management_page_count,
                                                       &free_data_page_count);
                    if (file->status == PIFS_SUCCESS && !free_management_page_count)
                    {
                        file->status = PIFS_ERROR_NO_MORE_SPACE;
                    }
                }
            }
            if (file->status == PIFS_SUCCESS)
            {
                /* Appending new pages to the file */
                PIFS_WARNING_MSG("Appending pages\r\n");
                do
                {
                    page_count_needed_limited = page_count_needed;
                    if (page_count_needed_limited >= PIFS_MAP_PAGE_COUNT_INVALID)
                    {
                        page_count_needed_limited = PIFS_MAP_PAGE_COUNT_INVALID - 1;
                    }
                    file->status = pifs_find_free_page_wl(1, page_count_needed_limited,
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
                            if (data_size > PIFS_LOGICAL_PAGE_SIZE_BYTE)
                            {
                                chunk_size = PIFS_LOGICAL_PAGE_SIZE_BYTE;
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
                            //PIFS_ASSERT(file->status == PIFS_SUCCESS);
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
            PIFS_DEBUG_MSG("File size increased to %i bytes\r\n", file->entry.file_size);
        }
        if (!file->mode_append)
        {
            file->read_pos = file->write_pos;
            file->read_address = file->write_address;
        }
    }

    PIFS_SET_ERRNO(file->status);
    return written_size / a_size;
}

/**
 * @brief pifs_inc_read_address Increment read address for file reading.
 *
 * @param[in] a_file Pointer to the internal file structure.
 */
pifs_status_t pifs_inc_read_address(pifs_file_t * a_file)
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
    return a_file->status;
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
size_t pifs_fread(void * a_data, size_t a_size, size_t a_count, P_FILE * a_file)
{
    pifs_file_t        * file = (pifs_file_t*) a_file;
    uint8_t *            data = (uint8_t*) a_data;
    pifs_size_t          read_size = 0;
    pifs_size_t          chunk_size = 0;
    pifs_size_t          data_size = a_size * a_count;
    pifs_size_t          page_count = 0;
    pifs_page_offset_t   po;

    PIFS_NOTICE_MSG("filename: '%s', size: %i, count: %i\r\n", file->entry.name, a_size, a_count);
    if (pifs.is_header_found && file && file->is_opened && file->mode_read)
    {
        if (file->read_pos + data_size > file->entry.file_size)
        {
            PIFS_WARNING_MSG("Trying to read more than file size: %i, file size: %i\r\n",
                             file->read_pos + data_size,
                             file->entry.file_size);
            data_size = file->entry.file_size - file->read_pos;
        }
        po = file->read_pos % PIFS_LOGICAL_PAGE_SIZE_BYTE;
        /* Check if last page was not fully read */
        if (po)
        {
            /* There is some data in the last page */
            //          PIFS_DEBUG_MSG("po != 0  %s\r\n", pifs_address2str(&file->read_address));
            PIFS_ASSERT(pifs_is_address_valid(&file->read_address));
            chunk_size = PIFS_MIN(data_size, PIFS_LOGICAL_PAGE_SIZE_BYTE - po);
            PIFS_DEBUG_MSG("--------> pos: %i po: %i data_size: %i chunk_size: %i\r\n",
                           file->read_pos, po, data_size, chunk_size);
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
                if (po + chunk_size >= PIFS_LOGICAL_PAGE_SIZE_BYTE)
                {
                    pifs_inc_read_address(file);
                }
            }
        }
        if (file->status == PIFS_SUCCESS && data_size > 0 && file->read_pos < file->entry.file_size)
        {
            page_count = (data_size + PIFS_LOGICAL_PAGE_SIZE_BYTE - 1) / PIFS_LOGICAL_PAGE_SIZE_BYTE;
            while (page_count && file->status == PIFS_SUCCESS)
            {
                chunk_size = PIFS_MIN(data_size, PIFS_LOGICAL_PAGE_SIZE_BYTE);
                if (file->read_pos + chunk_size > file->entry.file_size)
                {
                    chunk_size = file->entry.file_size - file->read_pos;
                }
                PIFS_NOTICE_MSG("read %s, page_count: %i, chunk size: %i\r\n",
                               pifs_address2str(&file->read_address), page_count, chunk_size);
                PIFS_ASSERT(pifs_is_address_valid(&file->read_address));
                file->status = pifs_read_delta(file->read_address.block_address,
                                               file->read_address.page_address,
                                               0, data, chunk_size);
                if (file->status == PIFS_SUCCESS && chunk_size == PIFS_LOGICAL_PAGE_SIZE_BYTE)
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

    PIFS_SET_ERRNO(file->status);
    return read_size / a_size;
}

/**
 * @brief pifs_fflush Flush cache, update file size.
 * @param[in] a_file File to flush.
 * @return 0 if file close, PIFS_EOF if error occurred.
 */
int pifs_fflush(P_FILE * a_file)
{
    int ret = PIFS_EOF;
    pifs_file_t * file = (pifs_file_t*) a_file;

    PIFS_NOTICE_MSG("filename: '%s'\r\n", file->entry.name);
    if (pifs.is_header_found && file && file->is_opened)
    {
        PIFS_DEBUG_MSG("mode_write: %i, is_size_changed: %i, file_size: %i\r\n",
                       file->mode_write, file->is_size_changed,
                       file->entry.file_size);
        if (file->mode_write && (file->is_size_changed || !file->entry.file_size))
        {
            file->status = pifs_update_entry(file->entry.name, &file->entry,
                                             pifs.header.entry_list_address.block_address,
                                             pifs.header.entry_list_address.page_address);
        }
        pifs_flush();
        ret = 0;
    }

    PIFS_SET_ERRNO(file->status);
    return ret;
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

    PIFS_NOTICE_MSG("filename: '%s'\r\n", file->entry.name);
    ret = pifs_fflush(a_file);
    if (ret == 0)
    {
        file->is_opened = FALSE;
        file->is_used = FALSE;
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
    int                 ret = PIFS_ERROR_GENERAL;
    pifs_file_t       * file = (pifs_file_t*) a_file;
    pifs_size_t         seek_size = 0;
    pifs_size_t         chunk_size = 0;
    pifs_size_t         data_size = 0;
    pifs_size_t         page_count = 0;
    pifs_page_offset_t  po;
    pifs_size_t         target_pos = 0;
    pifs_size_t         file_size = 0;

    PIFS_NOTICE_MSG("filename: '%s', offset: %i, origin: %i, read_pos: %i\r\n",
                    file->entry.name, a_offset, a_origin, file->read_pos);
    if (pifs.is_header_found && file && file->is_opened)
    {
        switch (a_origin)
        {
            case PIFS_SEEK_CUR:
                target_pos = file->read_pos + a_offset;
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
                    pifs_rewind(file); /* Zeroing file->read_pos! */
                }
                break;
            case PIFS_SEEK_SET:
                if ((long int)file->read_pos < a_offset)
                {
                    data_size = a_offset - file->read_pos;
                }
                else
                {
                    data_size = a_offset;
                    pifs_rewind(file); /* Zeroing file->read_pos! */
                }
                target_pos = a_offset;
                break;
            case PIFS_SEEK_END:
                if (file->entry.file_size == PIFS_FILE_SIZE_ERASED)
                {
                    file->status = PIFS_ERROR_SEEK_NOT_POSSIBLE;
                }
                else
                {
                    data_size = file->entry.file_size + a_offset;
                    if (data_size >= file->read_pos)
                    {
                        data_size -= file->read_pos;
                    }
                    else
                    {
                        pifs_rewind(file); /* Zeroing file->read_pos! */
                    }
                    target_pos = data_size;
                }
                break;
            default:
                break;
        }

        if (file->status == PIFS_SUCCESS)
        {
            if (file->entry.file_size != PIFS_FILE_SIZE_ERASED)
            {
                file_size = file->entry.file_size;
            }
            if (target_pos > file_size)
            {
                PIFS_WARNING_MSG("Trying to seek position %i while file size is %i bytes\r\n",
                                 target_pos, file_size);
                data_size -= target_pos - file_size;
                PIFS_WARNING_MSG("data_size: %i bytes\r\n", data_size);
            }

            po = file->read_pos % PIFS_LOGICAL_PAGE_SIZE_BYTE;
            /* Check if last page was not fully read */
            if (po)
            {
                /* There is some data in the last page */
                PIFS_ASSERT(pifs_is_address_valid(&file->read_address));
                chunk_size = PIFS_MIN(data_size, PIFS_LOGICAL_PAGE_SIZE_BYTE - po);
                if (file->status == PIFS_SUCCESS)
                {
                    data_size -= chunk_size;
                    seek_size += chunk_size;
                    if (po + chunk_size >= PIFS_LOGICAL_PAGE_SIZE_BYTE)
                    {
                        pifs_inc_read_address(file);
                    }
                }
            }
            if (file->status == PIFS_SUCCESS && data_size > 0)
            {
                page_count = (data_size + PIFS_LOGICAL_PAGE_SIZE_BYTE - 1) / PIFS_LOGICAL_PAGE_SIZE_BYTE;
                while (page_count && file->status == PIFS_SUCCESS)
                {
                    PIFS_ASSERT(pifs_is_address_valid(&file->read_address));
                    chunk_size = PIFS_MIN(data_size, PIFS_LOGICAL_PAGE_SIZE_BYTE);
                    //PIFS_DEBUG_MSG("read %s\r\n", pifs_address2str(&file->read_address));
                    if (file->status == PIFS_SUCCESS && chunk_size == PIFS_LOGICAL_PAGE_SIZE_BYTE)
                    {
                        pifs_inc_read_address(file);
                    }
                    data_size -= chunk_size;
                    seek_size += chunk_size;
                    page_count--;
                }
            }
            file->read_pos += seek_size;
            file->write_pos = file->read_pos;
            file->write_address = file->read_address;
#if PIFS_ENABLE_FSEEK_BEYOND_FILE
            /* Check if we are at end of file and data needs to be written to reach */
            /* target position */
            if (target_pos > file->read_pos)
            {
#if PIFS_ENABLE_FSEEK_ERASED_VALUE
                memset(pifs.sc_page_buf, PIFS_FLASH_ERASED_BYTE_VALUE, PIFS_LOGICAL_PAGE_SIZE_BYTE);
#else
                memset(pifs.sc_page_buf, PIFS_FLASH_PROGRAMMED_BYTE_VALUE, PIFS_LOGICAL_PAGE_SIZE_BYTE);
#endif
                data_size = target_pos - file->read_pos;
                page_count = (data_size + PIFS_LOGICAL_PAGE_SIZE_BYTE - 1) / PIFS_LOGICAL_PAGE_SIZE_BYTE;
                while (page_count && file->status == PIFS_SUCCESS)
                {
                    chunk_size = PIFS_MIN(data_size, PIFS_LOGICAL_PAGE_SIZE_BYTE);
                    file->status = pifs_fwrite(pifs.sc_page_buf, 1, chunk_size, file);
                    data_size -= chunk_size;
                    page_count--;
                }
                if (data_size && file->status == PIFS_SUCCESS)
                {
                    file->status = pifs_fwrite(pifs.sc_page_buf, 1, data_size, file);
                }
            }
#endif
        }
        ret = file->status;
    }

    PIFS_SET_ERRNO(file->status);
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

    PIFS_NOTICE_MSG("filename: '%s'\r\n", file->entry.name);
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
    PIFS_SET_ERRNO(file->status);
}

/**
 * @brief pifs_ftell Get current position in file.
 *
 * @param[in] a_file Pointer to file.
 * @return -1 if error or position in file.
 */
long int pifs_ftell(P_FILE * a_file)
{
    pifs_file_t * file = (pifs_file_t*) a_file;
    long int pos = -1;

    PIFS_NOTICE_MSG("filename: '%s'\r\n", file->entry.name);
    if (pifs.is_header_found && file && file->is_opened)
    {
        pos = file->read_pos;
    }

    return pos;
}

#if PIFS_ENABLE_USER_DATA
/**
 * @brief pifs_fgetuserdata Non-standard function to get user defined data of
 * file.
 *
 * @param[in] a_file        Pointer to file.
 * @param[out] a_user_data  Pointer to user data structure to fill.
 * @return 0 if success.
 */
int pifs_fgetuserdata(P_FILE * a_file, pifs_user_data_t * a_user_data)
{
    pifs_status_t ret = PIFS_ERROR_GENERAL;
    pifs_file_t * file = (pifs_file_t*) a_file;

    if (file->is_opened)
    {
        memcpy(a_user_data, &file->entry.user_data, sizeof(pifs_user_data_t));
        ret = PIFS_SUCCESS;
    }

    return ret;
}

/**
 * @brief pifs_fsetuserdata Non-standard function to set user defined data of
 * file.
 *
 * @param[in] a_file        Pointer to file.
 * @param[out] a_user_data  Pointer to user data structure to set.
 * @return 0 if success.
 */
int pifs_fsetuserdata(P_FILE * a_file, const pifs_user_data_t * a_user_data)
{
    pifs_file_t * file = (pifs_file_t*) a_file;

    if (file->is_opened)
    {
        memcpy(&file->entry.user_data, a_user_data, sizeof(pifs_user_data_t));

        file->status = pifs_update_entry(file->entry.name, &file->entry,
                                         pifs.header.entry_list_address.block_address,
                                         pifs.header.entry_list_address.page_address);
    }
    return file->status;
}
#endif

/**
 * @brief pifs_remove Remove file.
 *
 * @param[in] a_filename Pointer to filename to be removed.
 * @return 0 if file removed. Non-zero if file not found or file name is not valid.
 */
int pifs_remove(const pifs_char_t * a_filename)
{
    pifs_status_t ret;

    PIFS_NOTICE_MSG("filename: '%s'\r\n", a_filename);
    ret = pifs_check_filename(a_filename);
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_internal_open(&pifs.internal_file, a_filename, "r", FALSE);
        if (ret == PIFS_SUCCESS)
        {
            /* File already exist */
            ret = pifs_clear_entry(a_filename,
                                   pifs.header.entry_list_address.block_address,
                                   pifs.header.entry_list_address.page_address);
            if (ret == PIFS_SUCCESS)
            {
                /* Mark allocated pages to be released */
                ret = pifs_release_file_pages(&pifs.internal_file);
            }
            ret = pifs_fclose(&pifs.internal_file);
        }
    }

    PIFS_SET_ERRNO(ret);
    return ret;
}

/**
 * @brief pifs_rename Change name of file.
 *
 * @param[in] a_oldname Existing file's name.
 * @param[in] a_newname New file name.
 * @return 0: if rename was successfuly. Other: error occurred.
 */
int pifs_rename(const pifs_char_t * a_oldname, const pifs_char_t * a_newname)
{
    pifs_status_t  ret;
    pifs_entry_t * entry = &pifs.entry;

    PIFS_NOTICE_MSG("filename: '%s'\r\n", a_oldname);
    ret = pifs_check_filename(a_oldname);
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_find_entry(a_newname, entry,
                              pifs.header.entry_list_address.block_address,
                              pifs.header.entry_list_address.page_address);
        if (ret == PIFS_SUCCESS)
        {
            /* File already exist, remove! */
            ret = pifs_remove(a_newname);
        }
        ret = pifs_find_entry(a_oldname, entry,
                              pifs.header.entry_list_address.block_address,
                              pifs.header.entry_list_address.page_address);
        if (ret == PIFS_SUCCESS)
        {
            /* File already exist */
            ret = pifs_clear_entry(a_oldname,
                              pifs.header.entry_list_address.block_address,
                              pifs.header.entry_list_address.page_address);
        }
        if (ret == PIFS_SUCCESS)
        {
            /* Change name in entry and append new entry */
            strncpy(entry->name, a_newname, PIFS_FILENAME_LEN_MAX);
            ret = pifs_append_entry(entry,
                                    pifs.header.entry_list_address.block_address,
                                    pifs.header.entry_list_address.page_address);
        }
    }

    PIFS_SET_ERRNO(ret);
    return ret;
}

/**
 * @brief pifs_copy Copy file. This is a non-standard file operation.
 * Note: this function shall not use pifs.internal_file as merge could be
 * needed which uses the pifs.internal_file!
 *
 * @param[in] a_oldname Existing file's name.
 * @param[in] a_newname New file name.
 * @return 0: if copy was successfuly. Other: error occurred.
 */
int pifs_copy(const pifs_char_t * a_oldname, const pifs_char_t * a_newname)
{
    pifs_status_t    ret = PIFS_ERROR_NO_MORE_RESOURCE;
    pifs_file_t    * file = NULL;
    pifs_file_t    * file2 = NULL;
    pifs_size_t      read_bytes;
    pifs_size_t      written_bytes;

    PIFS_NOTICE_MSG("oldname: '%s', newname: '%s'\r\n", a_oldname, a_newname);

    file = pifs_fopen(a_oldname, "r");
    if (file)
    {
        file2 = pifs_fopen(a_newname, "w");
        if (file2)
        {
            do
            {
                read_bytes = pifs_fread(pifs.sc_page_buf, 1,
                                        PIFS_LOGICAL_PAGE_SIZE_BYTE, file);
                if (read_bytes > 0)
                {
                    written_bytes = pifs_fwrite(pifs.sc_page_buf, 1,
                                                read_bytes, file2);
                    if (read_bytes != written_bytes)
                    {
                        PIFS_ERROR_MSG("Cannot write: %i! Read bytes: %i, written bytes: %i\r\n",
                                       file2->status, read_bytes, written_bytes);
                        ret = file2->status;
                    }
                }
            } while (read_bytes > 0 && read_bytes == written_bytes);
            ret = pifs_fclose(file2);
        }
        else
        {
            PIFS_ERROR_MSG("Cannot open file '%s'\r\n", a_newname);
        }
        ret = pifs_fclose(file);
    }
    else
    {
        PIFS_ERROR_MSG("Cannot open file '%s'\r\n", a_oldname);
    }

    PIFS_SET_ERRNO(ret);
    return ret;
}

/**
 * @brief pifs_is_file_exist Check whether a file exist.
 * This is a non-standard file operation.
 *
 * @param[in] a_filename File name to be checked.
 * @return TRUE: if file exist. FALSE: file does not exist.
 */
bool_t pifs_is_file_exist(const pifs_char_t * a_filename)
{
    pifs_status_t    ret = PIFS_ERROR_NO_MORE_RESOURCE;
    bool_t           is_file_exist = FALSE;
    pifs_entry_t    * entry = &pifs.entry;

    PIFS_NOTICE_MSG("filename: '%s'\r\n", a_filename);

    ret = pifs_check_filename(a_filename);
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_find_entry(a_filename, entry,
                              pifs.header.entry_list_address.block_address,
                              pifs.header.entry_list_address.page_address);
    }
    if (ret == PIFS_SUCCESS)
    {
        is_file_exist = TRUE;
    }

    return is_file_exist;
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

/**
 * @brief pifs_feof Check end-of-file indicator.
 * @param[in] a_file Pointer to opened file.
 * @return 0: end-of-file reached, other: not end-of-file.
 */
int pifs_feof(P_FILE * a_file)
{
    pifs_file_t * file = (pifs_file_t*) a_file;
    int ret = file->read_pos == file->entry.file_size;

    return ret;
}

/**
 * @brief pifs_filesize Get size of a file.
 * @param[in] a_filename Pointer to the file name.
 * @return File size in bytes or -1 if file not found.
 */
long int pifs_filesize(const pifs_char_t * a_filename)
{
    long int filesize = -1;
    pifs_status_t status;
    pifs_entry_t  entry;

    status = pifs_find_entry(a_filename, &entry,
                              pifs.header.entry_list_address.block_address,
                              pifs.header.entry_list_address.page_address);
    if (status == PIFS_SUCCESS)
    {
        filesize = entry.file_size;
    }

    return filesize;
}
