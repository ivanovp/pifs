/**
 * @file        pifs.c
 * @brief       Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-16 13:10:56 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api_pifs.h"
#include "flash.h"
#include "flash_config.h"
#include "pifs.h"
#include "pifs_debug.h"
#include "buffer.h" /* DEBUG */

static pifs_t pifs =
{
    .header_address = { PIFS_BLOCK_ADDRESS_INVALID, PIFS_PAGE_ADDRESS_INVALID },
    .is_header_found = FALSE,
    .header = { 0 },
    .latest_object_id = 1,
    .page_buf_address = { PIFS_BLOCK_ADDRESS_INVALID, PIFS_PAGE_ADDRESS_INVALID },
    .page_buf = { 0 },
    .page_buf_is_dirty = FALSE
};

pifs_checksum_t pifs_calc_header_checksum(pifs_header_t * a_pifs_header)
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
 * @brief pifs_calc_free_space_pos Calculate position of a page in free space
 * memory bitmap.
 *
 * @param[in] a_free_space_bitmap_address Start address of free space bitmap area.
 * @param[in] a_block_address             Block address of page to be calculated.
 * @param[in] a_page_address              Page address of page to be calculated.
 * @param[out] a_free_space_block_address Block address of free space memory map.
 * @param[out] a_free_space_page_address  Page address of free space memory map.
 * @param[out] a_free_space_bit_pos       Bit position in free space memory map.
 */
void pifs_calc_free_space_pos(const pifs_address_t * a_free_space_bitmap_address,
                              pifs_block_address_t a_block_address,
                              pifs_page_address_t a_page_address,
                              pifs_block_address_t * a_free_space_block_address,
                              pifs_page_address_t * a_free_space_page_address,
                              pifs_bit_pos_t * a_free_space_bit_pos)
{
    pifs_bit_pos_t bit_pos;

    /* Shift left by one (<< 1) due to two bits are stored in free space bitmap */
    bit_pos = ((a_block_address - PIFS_FLASH_BLOCK_RESERVED_NUM) * PIFS_FLASH_PAGE_PER_BLOCK + a_page_address) << 1;
    PIFS_DEBUG_MSG("BA%i/PA%i bit_pos: %i\r\n", a_block_address, a_page_address, bit_pos);
    *a_free_space_block_address = a_free_space_bitmap_address->block_address
            + (bit_pos / BYTE_BITS / PIFS_FLASH_BLOCK_SIZE_BYTE);
    bit_pos %= PIFS_FLASH_BLOCK_SIZE_BYTE * BYTE_BITS;
    *a_free_space_page_address = a_free_space_bitmap_address->page_address
            + (bit_pos / BYTE_BITS / PIFS_FLASH_PAGE_SIZE_BYTE);
    bit_pos %= PIFS_FLASH_PAGE_SIZE_BYTE * BYTE_BITS;
    *a_free_space_bit_pos = bit_pos;
}

/**
 * @brief pifs_calc_address Calculate address from bit position of free space
 * memory bitmap.
 *
 * @param[in] a_bit_pos         Bit position in free space memory map.
 * @param[out] a_block_address  Block address of page to be calculated.
 * @param[out] a_page_address   Page address of page to be calculated.
 */
void pifs_calc_address(pifs_bit_pos_t a_bit_pos,
                       pifs_block_address_t * a_block_address,
                       pifs_page_address_t * a_page_address)
{
    /* Shift right by one (>> 1) due to two bits are stored in free space bitmap */
    a_bit_pos >>= 1;
    *a_block_address = (a_bit_pos / PIFS_FLASH_PAGE_PER_BLOCK) + PIFS_FLASH_BLOCK_RESERVED_NUM;
    a_bit_pos %= PIFS_FLASH_PAGE_PER_BLOCK;
    *a_page_address = a_bit_pos;
}

/**
 * @brief pifs_flush  Flush cache.
 *
 * @return PIFS_SUCCESS if data flush successfully.
 */
pifs_status_t pifs_flush(void)
{
    pifs_status_t ret = PIFS_SUCCESS;

    if (pifs.page_buf_is_dirty)
    {
        ret = pifs_flash_write(pifs.page_buf_address.block_address,
                               pifs.page_buf_address.page_address,
                               0,
                               pifs.page_buf, PIFS_FLASH_PAGE_SIZE_BYTE);
        if (ret == PIFS_SUCCESS)
        {
            pifs.page_buf_is_dirty = FALSE;
        }
        else
        {
            PIFS_ERROR_MSG("Cannot flush buffer BA%i/PA%i\r\n",
                           pifs.page_buf_address.block_address,
                           pifs.page_buf_address.page_address);
        }
    }

    return ret;
}

/**
 * @brief pifs_read  Cached read.
 *
 * @param a_block_address[in]   Block address of page to read.
 * @param a_page_address[in]    Page address of page to read.
 * @param a_page_offset[in]     Offset in page.
 * @param a_buf[out]            Pointer to buffer to fill or NULL if
 *                              pifs.page_buf is used.
 * @param a_buf_size[in]        Size of buffer. Ignored if a_buf is NULL.
 * @return PIFS_SUCCESS if data read successfully.
 */
pifs_status_t pifs_read(pifs_block_address_t a_block_address, pifs_page_address_t a_page_address, pifs_page_offset_t a_page_offset, void * const a_buf, size_t a_buf_size)
{
    pifs_status_t ret = PIFS_ERROR;

    if (a_block_address == pifs.page_buf_address.block_address
            && a_page_address == pifs.page_buf_address.page_address)
    {
        if (a_buf)
        {
            memcpy(a_buf, &pifs.page_buf[a_page_offset], a_buf_size);
        }
        ret = PIFS_SUCCESS;
    }
    else
    {
        ret = pifs_flush();

        if (ret == PIFS_SUCCESS)
        {
            ret = pifs_flash_read(a_block_address, a_page_address, 0,
                                  pifs.page_buf, PIFS_FLASH_PAGE_SIZE_BYTE);
        }

        if (ret == PIFS_SUCCESS)
        {
            if (a_buf)
            {
                memcpy(a_buf, &pifs.page_buf[a_page_offset], a_buf_size);
            }
            pifs.page_buf_address.block_address = a_block_address;
            pifs.page_buf_address.page_address = a_page_address;
        }
    }

    return ret;
}

/**
 * @brief pifs_write  Cached write.
 *
 * @param a_block_address[in]   Block address of page to write.
 * @param a_page_address[in]    Page address of page to write.
 * @param a_page_offset[in]     Offset in page.
 * @param a_buf[in]             Pointer to buffer to write or NULL if
 *                              pifs.page_buf is directly written.
 * @param a_buf_size[in]        Size of buffer. Ignored if a_buf is NULL.
 * @return PIFS_SUCCESS if data write successfully.
 */
pifs_status_t pifs_write(pifs_block_address_t a_block_address, pifs_page_address_t a_page_address, pifs_page_offset_t a_page_offset, const void * const a_buf, size_t a_buf_size)
{
    pifs_status_t ret = PIFS_ERROR;

    if (a_block_address == pifs.page_buf_address.block_address
            && a_page_address == pifs.page_buf_address.page_address)
    {
        if (a_buf)
        {
            memcpy(&pifs.page_buf[a_page_offset], a_buf, a_buf_size);
        }
        pifs.page_buf_is_dirty = TRUE;
        ret = PIFS_SUCCESS;
    }
    else
    {
        ret = pifs_flush();

        if (ret == PIFS_SUCCESS)
        {
            if (a_page_offset != 0 || a_buf_size != PIFS_FLASH_PAGE_SIZE_BYTE)
            {
                /* Only part of page is written */
                ret = pifs_flash_read(a_block_address, a_page_address, 0,
                                      pifs.page_buf, PIFS_FLASH_PAGE_SIZE_BYTE);
            }

            if (a_buf)
            {
                memcpy(&pifs.page_buf[a_page_offset], a_buf, a_buf_size);
            }
            pifs.page_buf_address.block_address = a_block_address;
            pifs.page_buf_address.page_address = a_page_address;
            pifs.page_buf_is_dirty = TRUE;
        }
    }

    return ret;
}

/**
 * @brief pifs_erase  Cached erase.
 *
 * @param a_block_address[in]   Block address of page to erase.
 * @return PIFS_SUCCESS if data erased successfully.
 */
pifs_status_t pifs_erase(pifs_block_address_t a_block_address)
{
    pifs_status_t ret = PIFS_ERROR;

    ret = pifs_flash_erase(a_block_address);

    if (a_block_address == pifs.page_buf_address.block_address)
    {
        pifs.page_buf_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
        pifs.page_buf_address.page_address = PIFS_PAGE_ADDRESS_INVALID;
        pifs.page_buf_is_dirty = FALSE;
    }

    return ret;
}

/**
 * @brief pifs_mark_page Mark page(s) as used (or to be released) in free space
 * memory bitmap.
 *
 * @param a_block_address Block address of page(s).
 * @param a_page_address Page address of page(s).
 * @param a_page_count Number of pages.
 * @param mark_used TRUE: Mark page used, FALSE: mark page to be released.
 * @return PIFS_SUCCESS: if page was successfully marked.
 */
pifs_status_t pifs_mark_page(pifs_block_address_t a_block_address,
                             pifs_page_address_t a_page_address,
                             size_t a_page_count, bool_t a_mark_used)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_bit_pos_t       bit_pos;
    pifs_block_address_t ba = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_page_address_t  pa = PIFS_PAGE_ADDRESS_INVALID;
    bool_t               is_free_space;
    bool_t               is_to_be_released;

    PIFS_ASSERT(pifs.is_header_found);

    while (a_page_count > 0 && ret == PIFS_SUCCESS)
    {
        pifs_calc_free_space_pos(&pifs.header.free_space_bitmap_address,
                                 a_block_address, a_page_address, &ba, &pa, &bit_pos);
        PIFS_DEBUG_MSG("BA%i/PA%i/BITPOS%i\r\n", ba, pa, bit_pos);
        /* Read actual status of free space memory bitmap (or cache) */
        ret = pifs_read(ba, pa, 0, NULL, 0);
        if (ret == PIFS_SUCCESS)
        {
            PIFS_ASSERT((bit_pos / BYTE_BITS) < PIFS_FLASH_PAGE_SIZE_BYTE);
//            print_buffer(pifs.page_buf, sizeof(pifs.page_buf), 0);
//            PIFS_DEBUG_MSG("-Free space byte:    0x%02X\r\n", pifs.page_buf[bit_pos / BYTE_BITS]);
            is_free_space = pifs.page_buf[bit_pos / BYTE_BITS] & (1u << (bit_pos % BYTE_BITS));
            is_to_be_released = pifs.page_buf[bit_pos / BYTE_BITS] & (1u << ((bit_pos % BYTE_BITS) + 1));
//            PIFS_DEBUG_MSG("-Free space bit:     %i\r\n", is_free_space);
//            PIFS_DEBUG_MSG("-Release space bit:  %i\r\n", is_to_be_released);
//            PIFS_DEBUG_MSG("-Free space bit:     %i\r\n", (pifs.page_buf[bit_pos / BYTE_BITS] >> (bit_pos % BYTE_BITS)) & 1);
//            PIFS_DEBUG_MSG("-Release space bit:  %i\r\n", (pifs.page_buf[bit_pos / BYTE_BITS] >> ((bit_pos % BYTE_BITS) + 1)) & 1);
            if (a_mark_used)
            {
                /* Mark page used */
                if (is_free_space)
                {
                    /* Clear free bit */
                    pifs.page_buf[bit_pos / BYTE_BITS] &= ~(1u << (bit_pos % BYTE_BITS));
                }
                else
                {
                    /* The space has already allocated */
                    PIFS_ERROR_MSG("Page has already allocated! BA%i/PA%i\r\n", a_block_address, a_page_address);
                    ret = PIFS_ERROR_INTERNAL_ALLOCATION;
                }
            }
            else
            {
                /* Mark page to be released */
                if (!is_free_space)
                {
                    if (!is_to_be_released)
                    {
                        /* Clear release bit */
                        pifs.page_buf[bit_pos / BYTE_BITS] &= ~(1u << ((bit_pos % BYTE_BITS) + 1));
                    }
                    else
                    {
                        /* The space has already marked to be released */
                        PIFS_ERROR_MSG("Page has already marked to be released BA%i/PA%i\r\n", a_block_address, a_page_address);
                        ret = PIFS_ERROR_INTERNAL_ALLOCATION;
                    }
                }
                else
                {
                    /* The space has not yet allocated */
                    PIFS_ERROR_MSG("Page has not yet allocated! BA%i/PA%i\r\n", a_block_address, a_page_address);
                    ret = PIFS_ERROR_INTERNAL_ALLOCATION;
                }
            }
//            PIFS_DEBUG_MSG("+Free space byte:    0x%02X\r\n", pifs.page_buf[bit_pos / BYTE_BITS]);
//            PIFS_DEBUG_MSG("+Free space bit:     %i\r\n", (pifs.page_buf[bit_pos / BYTE_BITS] >> (bit_pos % BYTE_BITS)) & 1);
//            PIFS_DEBUG_MSG("+Release space bit:  %i\r\n", (pifs.page_buf[bit_pos / BYTE_BITS] >> ((bit_pos % BYTE_BITS) + 1)) & 1);
            /* Write new status to cache */
            ret = pifs_write(ba, pa, 0, NULL, 0);
        }
        a_page_count--;
        if (a_page_count > 0)
        {
            a_page_address++;
            if (a_page_address == PIFS_FLASH_PAGE_PER_BLOCK)
            {
                a_page_address = 0;
                a_block_address++;
                if (a_block_address == PIFS_FLASH_BLOCK_NUM_ALL)
                {
                    PIFS_ERROR_MSG("Trying to mark invalid address! BA%i/PA%i\r\n", a_block_address, a_page_address);
                    ret = PIFS_ERROR_INTERNAL_RANGE;
                }
            }
        }
        else
        {
            ret = pifs_flush();
        }
    }

    return ret;
}

/**
 * @brief pifs_find_page Find free page(s) in free space memory bitmap.
 *
 * @param[in] a_page_count_needed Number of pages needed.
 * @param[in] a_page_type         Page type to find.
 * @param[in] a_is_free           TRUE: find free page, FALSE: find to be released page.
 * @param[out] a_block_address    Block address of page(s).
 * @param[out] a_page_address     Page address of page(s).
 * @param[out] a_page_count_found Number of free pages found.
 * @return PIFS_SUCCESS: if free pages found. PIFS_ERROR: if no free pages found.
 */
pifs_status_t pifs_find_page(size_t a_page_count_needed,
                             pifs_page_type_t a_page_type,
                             bool_t a_is_free,
                             pifs_block_address_t * a_block_address,
                             pifs_page_address_t * a_page_address,
                             size_t * a_page_count_found)
{
    pifs_status_t           ret = PIFS_ERROR;
    pifs_block_address_t    ba = pifs.header.free_space_bitmap_address.block_address;
    pifs_page_address_t     pa = pifs.header.free_space_bitmap_address.page_address;
    pifs_page_offset_t      po = 0;
    size_t                  i;
    size_t                  page_count_found = 0;
    uint8_t                 free_space_bitmap = 0;
    bool_t                  found = FALSE;
    size_t                  byte_cntr = PIFS_FREE_SPACE_BITMAP_SIZE_BYTE;
    pifs_bit_pos_t          bit_pos = 0;
    pifs_bit_pos_t          bit_pos_start = 0;
    uint8_t                 mask = 1; /**< Mask for finding free page */

    PIFS_ASSERT(pifs.is_header_found);

    if (!a_is_free)
    {
        /* Find to be released page */
        mask = 2;
    }

    do
    {
        printf("BA%i/PA%i ", ba, pa);
        ret = pifs_read(ba, pa, po, &free_space_bitmap, sizeof(free_space_bitmap));
        printf("%02X\r\n", free_space_bitmap);
        if (ret == PIFS_SUCCESS)
        {
            for (i = 0; i < (BYTE_BITS / 2) && !found; i++)
            {
                if (free_space_bitmap & mask)
                {
                    printf("bit_pos: %i free!\r\n", bit_pos);
                    page_count_found++;
                    if (page_count_found == a_page_count_needed)
                    {
                        PIFS_NOTICE_MSG("Page count found, bit_pos_start: %i, bit_pos: %i!\r\n", bit_pos_start, bit_pos);
                        pifs_calc_address(bit_pos_start,
                                          a_block_address, a_page_address);
                        *a_page_count_found = page_count_found;
                        found = TRUE;
                    }
                }
                else
                {
                    page_count_found = 0;
                    bit_pos_start = bit_pos + 2;
                }
                free_space_bitmap >>= 2;
                bit_pos += 2;
            }
            po++;
            if (po == PIFS_FLASH_PAGE_SIZE_BYTE)
            {
                po = 0;
                pa++;
                if (pa == PIFS_FLASH_PAGE_PER_BLOCK)
                {
                    PIFS_ERROR_MSG("End of block. byte_cntr: %lu\r\n", byte_cntr);
                }
            }
        }
    } while (byte_cntr-- > 0 && ret == PIFS_SUCCESS && !found);

    if (ret == PIFS_SUCCESS && !found)
    {
        ret = PIFS_ERROR_NO_MORE_SPACE;
    }

    return ret;
}

/**
 * @brief pifs_header_init Initialize file system's header.
 *
 * @param a_block_address[in]   Block address of header.
 * @param a_page_address[in]    Page address of header.
 * @param a_header[out]         Pointer to the header to be initialized.
 */
void pifs_header_init(pifs_block_address_t a_block_address,
                      pifs_page_address_t a_page_address,
                      pifs_header_t * a_header)
{
    /* FIXME use random block? */
    PIFS_DEBUG_MSG("Creating managamenet block BA%i/PA%i\r\n", a_block_address, a_page_address);
    a_header->magic = PIFS_MAGIC;
#if ENABLE_PIFS_VERSION
    a_header->majorVersion = PIFS_MAJOR_VERSION;
    a_header->minorVersion = PIFS_MINOR_VERSION;
#endif
    a_header->counter = 1;
    a_header->block_type = PIFS_BLOCK_TYPE_MANAGEMENT;
    a_header->entry_list_address.block_address = a_block_address;
    a_header->entry_list_address.page_address = a_page_address + PIFS_HEADER_SIZE_PAGE;
    a_header->free_space_bitmap_address.block_address = a_block_address;
    a_header->free_space_bitmap_address.page_address = a_page_address + PIFS_HEADER_SIZE_PAGE + PIFS_ENTRY_LIST_SIZE_PAGE;
    a_header->checksum = pifs_calc_header_checksum(a_header);
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
                                pifs_header_t * a_header)
{
    pifs_status_t ret = PIFS_ERROR;

    ret = pifs_erase(a_block_address);
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_write(a_block_address, a_page_address, 0, a_header, sizeof(pifs_header_t));
    }
    if (ret == PIFS_SUCCESS)
    {
        pifs.is_header_found = TRUE;
        pifs.header_address.block_address = a_block_address;
        pifs.header_address.page_address = a_page_address;

        /* Mark file system header as used */
        ret = pifs_mark_page(a_block_address,
                             a_page_address,
                             PIFS_HEADER_SIZE_PAGE, TRUE);
    }
    /* FIXME this may not be the right function to mark entry list and
     * free space bitmap.
     */
    if ( ret == PIFS_SUCCESS )
    {
        /* Mark entry list as used */
        ret = pifs_mark_page(a_header->entry_list_address.block_address,
                             a_header->entry_list_address.page_address,
                             PIFS_ENTRY_LIST_SIZE_PAGE, TRUE);
    }
    if ( ret == PIFS_SUCCESS )
    {
        /* Mark free space bitmap as used */
        ret = pifs_mark_page(a_header->free_space_bitmap_address.block_address,
                             a_header->free_space_bitmap_address.page_address,
                             PIFS_FREE_SPACE_BITMAP_SIZE_PAGE, TRUE);
    }

    return ret;
}

pifs_status_t pifs_init(void)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba;
    pifs_page_address_t  pa;
    pifs_header_t        header;
    pifs_header_t        prev_header;
    pifs_checksum_t      checksum;

    pifs.header_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
    pifs.header_address.page_address = PIFS_PAGE_ADDRESS_INVALID;
    pifs.is_header_found = FALSE;
    pifs.page_buf_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
    pifs.page_buf_address.page_address = PIFS_PAGE_ADDRESS_INVALID;
    pifs.page_buf_is_dirty = FALSE;

    PIFS_INFO_MSG("Geometry of flash memory\r\n");
    PIFS_INFO_MSG("------------------------\r\n");
    PIFS_INFO_MSG("Size of flash memory (all):         %i bytes\r\n", PIFS_FLASH_SIZE_BYTE_ALL);
    PIFS_INFO_MSG("Size of flash memory (used by FS):  %i bytes\r\n", PIFS_FLASH_SIZE_BYTE_FS);
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
    PIFS_INFO_MSG("Object ID size:                     %lu bytes\r\n", sizeof(pifs_object_id_t));
    PIFS_INFO_MSG("Header size:                        %lu bytes, %lu pages\r\n", PIFS_HEADER_SIZE_BYTE, PIFS_HEADER_SIZE_PAGE);
    PIFS_INFO_MSG("Entry size:                         %lu bytes\r\n", PIFS_ENTRY_SIZE_BYTE);
    PIFS_INFO_MSG("Entry size in a page:               %lu bytes\r\n", PIFS_ENTRY_SIZE_BYTE * PIFS_ENTRY_PER_PAGE);
    PIFS_INFO_MSG("Entry list size:                    %lu bytes, %lu pages\r\n", PIFS_ENTRY_LIST_SIZE_BYTE, PIFS_ENTRY_LIST_SIZE_PAGE);
    PIFS_INFO_MSG("Free space bitmap size:             %u bytes, %u pages\r\n", PIFS_FREE_SPACE_BITMAP_SIZE_BYTE, PIFS_FREE_SPACE_BITMAP_SIZE_PAGE);
    PIFS_INFO_MSG("Maximum number of management pages: %i\r\n",
                   PIFS_FLASH_PAGE_PER_BLOCK
                   + (PIFS_FLASH_BLOCK_NUM_FS - 1 ) * PIFS_MANAGEMENT_PAGE_PER_BLOCK_MAX);
    PIFS_INFO_MSG("\r\n");

    if (PIFS_ENTRY_SIZE_BYTE > PIFS_FLASH_PAGE_SIZE_BYTE)
    {
        PIFS_ERROR_MSG("Entry size is larger than flash page! Decrease PIFS_FILENAME_LEN_MAX!\r\n");
        ret = PIFS_ERROR_CONFIGURATION;
    }

    if ((PIFS_HEADER_SIZE_PAGE + PIFS_ENTRY_LIST_SIZE_PAGE + PIFS_FREE_SPACE_BITMAP_SIZE_PAGE ) > PIFS_FLASH_PAGE_PER_BLOCK)
    {
        PIFS_ERROR_MSG("Entry and free space bitmap is larger than a block! Decrease PIFS_FILENAME_LEN_MAX or PIFS_ENTRY_NUM_MAX!\r\n");
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
        for (ba = PIFS_FLASH_BLOCK_RESERVED_NUM; ba < PIFS_FLASH_BLOCK_NUM_ALL; ba++)
        {
            for (pa = 0; pa < PIFS_MANAGEMENT_PAGE_PER_BLOCK_MAX; pa++)
            {
                pifs_read(ba, pa, 0, &header, sizeof(header));
                if (header.magic == PIFS_MAGIC
        #if ENABLE_PIFS_VERSION
                        && header.majorVersion == PIFS_MAJOR_VERSION
                        && header.minorVersion == PIFS_MINOR_VERSION
        #endif
                        )
                {
                    PIFS_DEBUG_MSG("Management page found: BA%i/PA%i\r\n", ba, pa);
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
        }

        if (pifs.is_header_found)
        {
            memcpy(&pifs.header, &prev_header, sizeof(pifs.header));
        }
        else
        {
            /* No file system header found, so create brand new one */
#if 1
            ba = PIFS_FLASH_BLOCK_RESERVED_NUM;
#else
            ba = PIFS_FLASH_BLOCK_RESERVED_NUM + rand() % (PIFS_FLASH_BLOCK_NUM_FS);
#endif
            pa = 0;
            pifs_header_init(ba, pa, &pifs.header);
            ret = pifs_header_write(ba, pa, &pifs.header);
        }

        if (pifs.is_header_found)
        {
#if PIFS_DEBUG_LEVEL >= 5
            print_buffer(&pifs.header, sizeof(pifs.header), 0);
#endif
            PIFS_INFO_MSG("Counter: %i\r\n",
                            pifs.header.counter);
            PIFS_INFO_MSG("Entry list at BA%i/PA%i\r\n",
                            pifs.header.entry_list_address.block_address,
                            pifs.header.entry_list_address.page_address);
            PIFS_INFO_MSG("Free space bitmap at BA%i/PA%i\r\n",
                            pifs.header.free_space_bitmap_address.block_address,
                            pifs.header.free_space_bitmap_address.page_address);
        }
    }

    return ret;
}

pifs_status_t pifs_delete(void)
{
    pifs_status_t ret = PIFS_ERROR;

    ret = pifs_flash_delete();

    return ret;
}

bool_t pifs_is_buffer_erased(const void * a_buf, size_t a_buf_size)
{
    uint8_t * buf = (uint8_t*) a_buf;
    size_t i;
    bool_t ret = TRUE;

    for (i = 0; i < a_buf_size && ret; i++)
    {
        if (buf[i] != PIFS_ERASED_VALUE)
        {
            ret = FALSE;
        }
    }

    return ret;
}

pifs_status_t pifs_create_entry(const pifs_entry_t * a_entry)
{
    pifs_status_t        ret = PIFS_ERROR;
    pifs_block_address_t ba = pifs.header.entry_list_address.block_address;
    pifs_page_address_t  pa = pifs.header.entry_list_address.page_address;
    bool_t               created = FALSE;
    pifs_entry_t       * entry = (pifs_entry_t*) pifs.page_buf;
    size_t               i;

    while (pa < PIFS_FLASH_PAGE_PER_BLOCK && !created)
    {
        ret = pifs_read(ba, pa, 0, pifs.page_buf, PIFS_ENTRY_PER_PAGE * PIFS_ENTRY_SIZE_BYTE);
        for (i = 0; i < PIFS_ENTRY_PER_PAGE && !created; i++)
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

    return ret;
}

pifs_status_t pifs_find_entry(const char * a_name, pifs_entry_t * const a_entry)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba = pifs.header.entry_list_address.block_address;
    pifs_page_address_t  pa = pifs.header.entry_list_address.page_address;
    bool_t               found = FALSE;
    pifs_entry_t       * entry = (pifs_entry_t*) pifs.page_buf;
    size_t               i;

    while (pa < PIFS_FLASH_PAGE_PER_BLOCK && !found && ret == PIFS_SUCCESS)
    {
        ret = pifs_read(ba, pa, 0, pifs.page_buf, PIFS_ENTRY_PER_PAGE * PIFS_ENTRY_SIZE_BYTE);
        for (i = 0; i < PIFS_ENTRY_PER_PAGE && !found; i++)
        {
            /* Check if name matches */
            if (strncmp((char*)entry->name, a_name, sizeof(entry->name)) == 0)
            {
                /* Entry found */
                memcpy(a_entry, entry, sizeof(pifs_entry_t));
                found = TRUE;
            }
        }
        pa++;
    }

    if (ret == PIFS_SUCCESS && !found)
    {
        ret = PIFS_ERROR_ENTRY_NOT_FOUND;
    }

    return ret;
}

P_FILE * pifs_fopen(const char * a_filename, const char *a_modes)
{
    pifs_file_t        * file = &pifs.file[0];
    pifs_entry_t       * entry = &pifs.file[0].entry;
    pifs_block_address_t ba = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_page_address_t  pa = PIFS_PAGE_ADDRESS_INVALID;
    size_t               page_count_found = 0;

    /* TODO check validity of filename */
    /* TODO check a_modes */
    if (pifs.is_header_found && strlen(a_filename))
    {
        file->status = pifs_find_entry(a_filename, entry);
        if (file->status == PIFS_SUCCESS)
        {
            printf("Entry of %s found:\r\n", a_filename);
            print_buffer(entry, sizeof(pifs_entry_t), 0);
            file->is_opened = TRUE;
        }
        else
        {
            file->status = pifs_find_page(1, PIFS_PAGE_TYPE_DATA, TRUE,
                                          &ba, &pa, &page_count_found);
            if (file->status == PIFS_SUCCESS)
            {
                printf("%lu free page found BA%i/PA%i\r\n", page_count_found, ba, pa);
            }
            else
            {
                printf("No free page found!\r\n");
            }
            strncpy((char*)entry->name, a_filename, PIFS_FILENAME_LEN_MAX);
            entry->object_id = 1;
            entry->attrib = PIFS_ATTRIB_ARCHIVE;
            entry->address.block_address = ba;
            entry->address.page_address = pa;
            file->status = pifs_create_entry(entry);
            if (file->status == PIFS_SUCCESS)
            {
                PIFS_DEBUG_MSG("Entry created\r\n");
                file->status = pifs_mark_page(ba, pa, 1, TRUE);
                if (file->status == PIFS_SUCCESS)
                {
                    file->is_opened = TRUE;
                }
            }
            else
            {
                printf("Cannot create entry!\r\n");
            }
        }
    }

    return file->is_opened ? (P_FILE*) file : (P_FILE*) NULL;
}

size_t pifs_fwrite(const void * a_data, size_t a_size, size_t a_count, P_FILE * a_file)
{
    pifs_file_t        * file = (pifs_file_t*) a_file;
    size_t               data_size = a_size * a_count;
    size_t               written_size = 0;
    size_t               page_needed = 0;
    size_t               page_found;
    size_t               page_found_start;
    size_t               chunk_size;
    pifs_block_address_t ba = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_block_address_t ba_start = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_page_address_t  pa = PIFS_PAGE_ADDRESS_INVALID;
    pifs_page_address_t  pa_start = PIFS_PAGE_ADDRESS_INVALID;

    if (pifs.is_header_found && file && file->is_opened)
    {
        page_needed = (a_size * a_count + PIFS_FLASH_PAGE_SIZE_BYTE - 1) / PIFS_FLASH_PAGE_SIZE_BYTE;
        do
        {
            file->status = pifs_find_page(page_needed, PIFS_PAGE_TYPE_DATA, TRUE,
                                          &ba, &pa, &page_found);
            PIFS_DEBUG_MSG("%lu pages found. BA%i/PA%i\r\n", page_found, ba, pa);
            if (file->status == PIFS_SUCCESS)
            {
                ba_start = ba;
                pa_start = pa;
                page_found_start = page_found;
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
                    file->status = pifs_write(ba, pa, 0, a_data, chunk_size);
                    pa++;
                    if (pa == PIFS_FLASH_PAGE_PER_BLOCK)
                    {
                        pa = 0;
                        ba++;
                        if (ba == PIFS_FLASH_BLOCK_NUM_ALL)
                        {
                            PIFS_ERROR_MSG("Trying to write to invalid flash address! BA%i/PA%i\r\n",
                                           ba, pa);
                        }
                    }
                    data_size -= chunk_size;
                    written_size += chunk_size;
                    page_found--;
                    page_needed--;
                } while (page_found && file->status == PIFS_SUCCESS);

                if (file->status == PIFS_SUCCESS)
                {
                    file->status = pifs_mark_page(ba_start, pa_start, page_found_start, TRUE);
                    /* TODO write pages to management entry after successfully
                     * write
                     */
                }
            }
        } while (page_needed && file->status == PIFS_SUCCESS);
    }

    return written_size;
}

size_t pifs_fread(void * a_data, size_t a_size, size_t a_count, P_FILE * a_file)
{
    size_t read_size = 0;

    if (pifs.is_header_found && a_file)
    {
    }

    return read_size;
}

int pifs_fclose(P_FILE * a_file)
{
    if (pifs.is_header_found && a_file)
    {
    }

    return 0;
}

