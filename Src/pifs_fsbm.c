/**
 * @file        pifs_fsbm.c
 * @brief       Pi file system: free space bitmap functions
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

#define PIFS_DEBUG_LEVEL 3
#include "pifs_debug.h"

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
pifs_status_t pifs_calc_free_space_pos(const pifs_address_t * a_free_space_bitmap_address,
                                       pifs_block_address_t a_block_address,
                                       pifs_page_address_t a_page_address,
                                       pifs_block_address_t * a_free_space_block_address,
                                       pifs_page_address_t * a_free_space_page_address,
                                       pifs_bit_pos_t * a_free_space_bit_pos)
{
    pifs_status_t  status = PIFS_ERROR_INTERNAL_RANGE;
    pifs_bit_pos_t bit_pos;

    if (a_block_address < PIFS_BLOCK_ADDRESS_INVALID
            && a_page_address < PIFS_PAGE_ADDRESS_INVALID)
    {
        /* Shift left by one (<< 1) due to two bits are stored in free space bitmap */
        bit_pos = ((a_block_address - PIFS_FLASH_BLOCK_RESERVED_NUM) * PIFS_FLASH_PAGE_PER_BLOCK + a_page_address) << PIFS_FSBM_BITS_PER_PAGE_SHIFT;
        //PIFS_DEBUG_MSG("BA%i/PA%i bit_pos: %i\r\n", a_block_address, a_page_address, bit_pos);
        *a_free_space_block_address = a_free_space_bitmap_address->block_address
                + (bit_pos / PIFS_BYTE_BITS / PIFS_FLASH_BLOCK_SIZE_BYTE);
        bit_pos %= PIFS_FLASH_BLOCK_SIZE_BYTE * PIFS_BYTE_BITS;
        *a_free_space_page_address = a_free_space_bitmap_address->page_address
                + (bit_pos / PIFS_BYTE_BITS / PIFS_FLASH_PAGE_SIZE_BYTE);
        bit_pos %= PIFS_FLASH_PAGE_SIZE_BYTE * PIFS_BYTE_BITS;
        *a_free_space_bit_pos = bit_pos;

        status = PIFS_SUCCESS;
    }
    else
    {
        PIFS_FATAL_ERROR_MSG("Invalid address %s\r\n",
                             pifs_ba_pa2str(a_block_address, a_page_address));
    }

    return status;
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
    a_bit_pos >>= PIFS_FSBM_BITS_PER_PAGE_SHIFT;
    *a_block_address = (a_bit_pos / PIFS_FLASH_PAGE_PER_BLOCK) + PIFS_FLASH_BLOCK_RESERVED_NUM;
    a_bit_pos %= PIFS_FLASH_PAGE_PER_BLOCK;
    *a_page_address = a_bit_pos;
}

/**
 * @brief pifs_is_page_free Check if page is used.
 *
 * @param[in] a_block_address   Block address of page(s).
 * @param[in] a_page_address    Page address of page(s).
 * @return TRUE: page is free. FALSE: page is used.
 */
bool_t pifs_is_page_free(pifs_block_address_t a_block_address,
                         pifs_page_address_t a_page_address)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_bit_pos_t       bit_pos;
    pifs_block_address_t ba = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_page_address_t  pa = PIFS_PAGE_ADDRESS_INVALID;
    bool_t               is_free_space = FALSE;

    PIFS_ASSERT(pifs.is_header_found);

    ret = pifs_calc_free_space_pos(&pifs.header.free_space_bitmap_address,
                                   a_block_address, a_page_address, &ba, &pa, &bit_pos);
    if (ret == PIFS_SUCCESS)
    {
        /* Read actual status of free space memory bitmap (or cache) */
        ret = pifs_read(ba, pa, 0, NULL, 0);
    }
    if (ret == PIFS_SUCCESS)
    {
        PIFS_ASSERT((bit_pos / PIFS_BYTE_BITS) < PIFS_FLASH_PAGE_SIZE_BYTE);
        is_free_space = pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS] & (1u << (bit_pos % PIFS_BYTE_BITS));
    }

    return is_free_space;
}

/**
 * @brief pifs_is_page_to_be_released Mark page(s) as used (or to be released) in free space
 * memory bitmap.
 *
 * @param[in] a_block_address   Block address of page(s).
 * @param[in] a_page_address    Page address of page(s).
 * @return PIFS_SUCCESS: if page was successfully marked.
 */
bool_t pifs_is_page_to_be_released(pifs_block_address_t a_block_address,
                                   pifs_page_address_t a_page_address)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_bit_pos_t       bit_pos;
    pifs_block_address_t ba = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_page_address_t  pa = PIFS_PAGE_ADDRESS_INVALID;
    bool_t               is_not_to_be_released = FALSE;

    PIFS_ASSERT(pifs.is_header_found);

    ret = pifs_calc_free_space_pos(&pifs.header.free_space_bitmap_address,
                                   a_block_address, a_page_address, &ba, &pa, &bit_pos);
    if (ret == PIFS_SUCCESS)
    {
        /* Read actual status of free space memory bitmap (or cache) */
        ret = pifs_read(ba, pa, 0, NULL, 0);
    }
    if (ret == PIFS_SUCCESS)
    {
        PIFS_ASSERT((bit_pos / PIFS_BYTE_BITS) < PIFS_FLASH_PAGE_SIZE_BYTE);
        is_not_to_be_released = pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS] & (1u << ((bit_pos % PIFS_BYTE_BITS) + 1));
    }

    return !is_not_to_be_released;
}

/**
 * @brief pifs_mark_page Mark page(s) as used (or to be released) in free space
 * memory bitmap.
 *
 * @param[in] a_block_address   Block address of page(s).
 * @param[in] a_page_address    Page address of page(s).
 * @param[in] a_page_count      Number of pages.
 * @param[in] mark_used         TRUE: Mark page used, FALSE: mark page to be released.
 * @return PIFS_SUCCESS: if page was successfully marked.
 */
pifs_status_t pifs_mark_page(pifs_block_address_t a_block_address,
                             pifs_page_address_t a_page_address,
                             pifs_page_count_t a_page_count, bool_t a_mark_used)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_bit_pos_t       bit_pos;
    pifs_block_address_t ba = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_page_address_t  pa = PIFS_PAGE_ADDRESS_INVALID;
    bool_t               is_free_space;
    bool_t               is_not_to_be_released;

    PIFS_ASSERT(pifs.is_header_found);

    while (a_page_count > 0 && ret == PIFS_SUCCESS)
    {
        PIFS_DEBUG_MSG("Mark page %s as %s\r\n",
                        pifs_ba_pa2str(a_block_address, a_page_address),
                        a_mark_used ? "used" : "to be released");
        ret = pifs_calc_free_space_pos(&pifs.header.free_space_bitmap_address,
                                       a_block_address, a_page_address, &ba, &pa, &bit_pos);
        //        PIFS_DEBUG_MSG("BA%i/PA%i/BITPOS%i\r\n", ba, pa, bit_pos);
        if (ret == PIFS_SUCCESS)
        {
            /* Read actual status of free space memory bitmap (or cache) */
            ret = pifs_read(ba, pa, 0, NULL, 0);
        }
        if (ret == PIFS_SUCCESS)
        {
            PIFS_ASSERT((bit_pos / PIFS_BYTE_BITS) < PIFS_FLASH_PAGE_SIZE_BYTE);
            //print_buffer(pifs.cache_page_buf, sizeof(pifs.cache_page_buf), 0);
            //PIFS_DEBUG_MSG("-Free space byte:    0x%02X\r\n", pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS]);
            is_free_space = pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS] & (1u << (bit_pos % PIFS_BYTE_BITS));
            is_not_to_be_released = pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS] & (1u << ((bit_pos % PIFS_BYTE_BITS) + 1));
            //PIFS_DEBUG_MSG("-Free space bit:     %i\r\n", is_free_space);
            //PIFS_DEBUG_MSG("-Release space bit:  %i\r\n", is_not_to_be_released);
            //PIFS_DEBUG_MSG("-Free space bit:     %i\r\n", (pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS] >> (bit_pos % PIFS_BYTE_BITS)) & 1);
            //PIFS_DEBUG_MSG("-Release space bit:  %i\r\n", (pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS] >> ((bit_pos % PIFS_BYTE_BITS) + 1)) & 1);
            if (a_mark_used)
            {
                /* Mark page used */
                if (is_free_space)
                {
                    //PIFS_NOTICE_MSG("MARK %s\r\n", pifs_ba_pa2str(a_block_address, a_page_address));
                    /* Clear free bit */
                    pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS] &= ~(1u << (bit_pos % PIFS_BYTE_BITS));
                }
                else
                {
                    /* The space has already allocated */
                    pifs_print_cache();
                    PIFS_FATAL_ERROR_MSG("Page has already allocated! %s\r\n", pifs_ba_pa2str(a_block_address, a_page_address));
                    ret = PIFS_ERROR_INTERNAL_ALLOCATION;
                }
            }
            else
            {
                /* Mark page to be released */
                if (!is_free_space)
                {
                    if (is_not_to_be_released)
                    {
                        /* Clear release bit */
                        pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS] &= ~(1u << ((bit_pos % PIFS_BYTE_BITS) + 1));
                    }
                    else
                    {
                        /* The space has already marked to be released */
                        pifs_print_cache();
                        PIFS_FATAL_ERROR_MSG("Page has already marked to be released %s\r\n", pifs_ba_pa2str(a_block_address, a_page_address));
                        ret = PIFS_ERROR_INTERNAL_ALLOCATION;
                    }
                }
                else
                {
                    /* The space has not yet allocated */
                    pifs_print_cache();
                    PIFS_FATAL_ERROR_MSG("Page has not yet allocated! %s\r\n", pifs_ba_pa2str(a_block_address, a_page_address));
                    ret = PIFS_ERROR_INTERNAL_ALLOCATION;
                }
            }
            //PIFS_DEBUG_MSG("+Free space byte:    0x%02X\r\n", pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS]);
            //PIFS_DEBUG_MSG("+Free space bit:     %i\r\n", (pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS] >> (bit_pos % PIFS_BYTE_BITS)) & 1);
            //PIFS_DEBUG_MSG("+Release space bit:  %i\r\n", (pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS] >> ((bit_pos % PIFS_BYTE_BITS) + 1)) & 1);
            /* Write new status to cache */
            ret = pifs_write(ba, pa, 0, NULL, 0);
        }
        a_page_count--;
        if (a_page_count > 0)
        {
            ret = pifs_inc_ba_pa(&a_block_address, &a_page_address);
        }
        else
        {
            ret = pifs_flush();
        }
    }

    return ret;
}

/**
 * @brief pifs_find_free_page Find free page(s) in free space
 * memory bitmap.
 * It tries to find 'a_page_count_desired' pages, but at least
 * one page.
 * NOTE there should be an option for maximum search time.
 *
 * @param[in] a_page_count_desired Number of pages needed.
 * @param[in] a_block_type         Block type to find.
 * @param[out] a_block_address     Block address of page(s).
 * @param[out] a_page_address      Page address of page(s).
 * @param[out] a_page_count_found  Number of free pages found.
 * @return PIFS_SUCCESS: if free pages found. PIFS_ERROR_GENERAL: if no free pages found.
 */
pifs_status_t pifs_find_free_page(pifs_page_count_t a_page_count_desired,
                                  pifs_block_type_t a_block_type,
                                  pifs_block_address_t * a_block_address,
                                  pifs_page_address_t * a_page_address,
                                  pifs_page_count_t * a_page_count_found)
{
    return pifs_find_page(1, a_page_count_desired, a_block_type, TRUE, FALSE,
                          PIFS_FLASH_BLOCK_RESERVED_NUM,
                          a_block_address, a_page_address, a_page_count_found);
}

/**
 * @brief pifs_find_page Find free or to be released page(s) in free space
 * memory bitmap.
 * It tries to find 'a_page_count_desired' pages, but at least
 * 'a_page_count_minimum'.
 * NOTE there should be an option for maximum search time.
 *
 * @param[in] a_page_count_minimum Number of pages needed at least.
 * @param[in] a_page_count_desired Number of pages needed.
 * @param[in] a_block_type         Block type to find.
 * @param[in] a_is_free            TRUE: find free page,
 *                                 FALSE: find to be released page.
 * @param[in] a_is_same_block      TRUE: find pages in same block,
 *                                 FALSE: pages can be in different block.
 * @param[out] a_block_address     Block address of page(s).
 * @param[out] a_page_address      Page address of page(s).
 * @param[out] a_page_count_found  Number of free pages found.
 * @return PIFS_SUCCESS: if free pages found. PIFS_ERROR_GENERAL: if no free pages found.
 */
pifs_status_t pifs_find_page(pifs_page_count_t a_page_count_minimum,
                             pifs_page_count_t a_page_count_desired,
                             pifs_block_type_t a_block_type,
                             bool_t a_is_free,
                             bool_t a_is_same_block,
                             pifs_block_address_t a_start_block_address,
                             pifs_block_address_t * a_block_address,
                             pifs_page_address_t * a_page_address,
                             pifs_page_count_t * a_page_count_found)
{
    pifs_find_t find;

    find.page_count_minimum = a_page_count_minimum;
    find.page_count_desired = a_page_count_desired;
    find.block_type = a_block_type;
    find.is_free = a_is_free;
    find.is_same_block = a_is_same_block;
    find.start_block_address = a_start_block_address;
    find.header = &pifs.header;

    return pifs_find_page_adv(&find, a_block_address, a_page_address, a_page_count_found);
}

/**
 * @brief pifs_find_page_adv Find free or to be released page(s) in free space
 * memory bitmap. Advanced version.
 * It tries to find 'page_count_desired' pages, but at least
 * 'page_count_minimum'.
 * NOTE there should be an option for maximum search time.
 *
 * @param[in] a_find               Find parameters.
 * @param[out] a_block_address     Block address of page(s).
 * @param[out] a_page_address      Page address of page(s).
 * @param[out] a_page_count_found  Number of free pages found.
 * @return PIFS_SUCCESS: if free pages found. PIFS_ERROR_GENERAL: if no free pages found.
 */
pifs_status_t pifs_find_page_adv(pifs_find_t * a_find,
                                 pifs_block_address_t * a_block_address,
                                 pifs_page_address_t * a_page_address,
                                 pifs_page_count_t * a_page_count_found)
{
    pifs_status_t           ret = PIFS_ERROR_GENERAL;
    pifs_block_address_t    fba = a_find->start_block_address;
    pifs_page_address_t     fpa = 0;
    pifs_block_address_t    fba_start = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_page_address_t     fpa_start = PIFS_PAGE_ADDRESS_INVALID;
    pifs_block_address_t    fsbm_ba = a_find->header->free_space_bitmap_address.block_address;
    pifs_page_address_t     fsbm_pa = a_find->header->free_space_bitmap_address.page_address;
    pifs_page_offset_t      po = 0;
    pifs_size_t             i;
    pifs_page_count_t       page_count_found = 0;
    uint8_t                 free_space_bitmap = 0;
    bool_t                  found = FALSE;
    pifs_size_t             byte_cntr = PIFS_FREE_SPACE_BITMAP_SIZE_BYTE;
    pifs_bit_pos_t          bit_pos = 0;
    uint8_t                 mask = 1;  /**< Mask for finding free page */
    uint8_t                 value = 1; /**< Value for finding free page */

    PIFS_ASSERT(pifs.is_header_found);

#if PIFS_FLASH_BLOCK_RESERVED_NUM
    fba = PIFS_MAX(PIFS_FLASH_BLOCK_RESERVED_NUM, a_find->start_block_address);
#endif
    /* Check if start block address is valid */
    if (fba >= PIFS_FLASH_BLOCK_NUM_ALL)
    {
        PIFS_WARNING_MSG("Start block address corrected from %i to %i\r\n",
                         fba, PIFS_FLASH_BLOCK_RESERVED_NUM);
        fba = PIFS_FLASH_BLOCK_RESERVED_NUM;
    }

    if (!a_find->is_free)
    {
        /* Find to be released page */
        mask = 2;
        value = 0;
    }

    ret = pifs_calc_free_space_pos(&a_find->header->free_space_bitmap_address,
                                   fba, fpa, &fsbm_ba, &fsbm_pa, &bit_pos);
    if (ret == PIFS_SUCCESS)
    {
        po = bit_pos / PIFS_BYTE_BITS;

        do
        {
            ret = pifs_read(fsbm_ba, fsbm_pa, po, &free_space_bitmap, sizeof(free_space_bitmap));
            if (ret == PIFS_SUCCESS)
            {
                //PIFS_DEBUG_MSG("%s %i 0x%X\r\n", pifs_ba_pa2str(ba, pa), po, free_space_bitmap);
                for (i = 0; i < (PIFS_BYTE_BITS / PIFS_FSBM_BITS_PER_PAGE) && !found; i++)
                {
                    /* TODO use free pages and to be released pages as well when */
                    /* looking for erasable blocks! */
                    if ((free_space_bitmap & mask) == value
                            && pifs_is_block_type(fba, a_find->block_type, &pifs.header))
                    {
#if PIFS_CHECK_IF_PAGE_IS_ERASED
                        if (!a_find->is_free || pifs_is_page_erased(fba, fpa))
#endif
                        {
                            if (page_count_found == 0)
                            {
                                fba_start = fba;
                                fpa_start = fpa;
                            }
                            page_count_found++;
                            if (page_count_found >= a_find->page_count_minimum)
                            {
                                *a_block_address = fba_start;
                                *a_page_address = fpa_start;
                                *a_page_count_found = page_count_found;
                            }
                            if (page_count_found == a_find->page_count_desired)
                            {
                                found = TRUE;
                            }
                        }
#if PIFS_CHECK_IF_PAGE_IS_ERASED
                        else
                        {
                            PIFS_ERROR_MSG("Flash page should be erased, but it is not! %s\r\n", pifs_ba_pa2str(fba, fpa));
                        }
#endif
                    }
                    else
                    {
                        page_count_found = 0;
                    }
                    if (!found)
                    {
                        free_space_bitmap >>= PIFS_FSBM_BITS_PER_PAGE;
                        fpa++;
                        if (fpa == PIFS_FLASH_PAGE_PER_BLOCK)
                        {
                            fpa = 0;
                            fba++;
                            if (a_find->is_same_block
                                    && (a_find->page_count_minimum < PIFS_FLASH_PAGE_PER_BLOCK
                                        || (fpa_start > 0 && a_find->page_count_desired >= PIFS_FLASH_PAGE_PER_BLOCK)))
                            {
                                page_count_found = 0;
                            }
                            if (fba >= PIFS_FLASH_BLOCK_NUM_ALL)
                            {
                                ret = PIFS_ERROR_NO_MORE_SPACE;
                            }
                        }
                    }
                }
                po++;
                if (po == PIFS_FLASH_PAGE_SIZE_BYTE)
                {
                    po = 0;
                    fsbm_pa++;
                    if (fsbm_pa == PIFS_FLASH_PAGE_PER_BLOCK)
                    {
                        fsbm_pa = 0;
                        fsbm_ba++;
                        if (fsbm_ba >= PIFS_FLASH_BLOCK_NUM_ALL)
                        {
                            PIFS_FATAL_ERROR_MSG("End of flash! byte_cntr: %lu\r\n", byte_cntr);
                        }
                    }
                }
            }
        } while (byte_cntr-- > 0 && ret == PIFS_SUCCESS && !found);
    }

    if (ret == PIFS_SUCCESS && !found)
    {
        ret = PIFS_ERROR_NO_MORE_SPACE;
    }

    return ret;
}

pifs_status_t pifs_find_free_block(pifs_size_t a_block_count,
                                   pifs_block_type_t a_block_type,
                                   pifs_block_address_t a_start_block_address,
                                   pifs_header_t *a_header,
                                   pifs_block_address_t * a_block_address)
{
    pifs_status_t        ret;
    pifs_block_address_t ba;
    pifs_page_address_t  pa;
    pifs_page_count_t    page_count;
    pifs_find_t          find;

    find.page_count_minimum = a_block_count * PIFS_FLASH_PAGE_PER_BLOCK;
    find.page_count_desired = a_block_count * PIFS_FLASH_PAGE_PER_BLOCK;
    find.block_type = a_block_type;
    find.is_free = TRUE;
    find.is_same_block = TRUE;
    find.start_block_address = a_start_block_address;
    find.header = a_header;

    ret = pifs_find_page_adv(&find, &ba, &pa, &page_count);

    *a_block_address = ba;

    return ret;
}

pifs_status_t pifs_find_to_be_released_block(pifs_size_t a_block_count,
                                             pifs_block_type_t a_block_type,
                                             pifs_block_address_t a_start_block_address,
                                             pifs_header_t * a_header,
                                             pifs_block_address_t * a_block_address)
{
    pifs_status_t        ret;
    pifs_block_address_t ba = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_page_address_t  pa;
    pifs_page_count_t    page_count;
    pifs_find_t          find;

    find.page_count_minimum = a_block_count * PIFS_FLASH_PAGE_PER_BLOCK;
    find.page_count_desired = a_block_count * PIFS_FLASH_PAGE_PER_BLOCK;
    find.block_type = a_block_type;
    find.is_free = FALSE;
    find.is_same_block = TRUE;
    find.start_block_address = a_start_block_address;
    find.header = a_header;

    ret = pifs_find_page_adv(&find, &ba, &pa, &page_count);

    *a_block_address = ba;

    return ret;
}

/**
 * @brief pifs_get_pages Find free/to be released page(s) in free space memory bitmap.
 *
 * @param[in] a_is_free                 TRUE: find free page,
 *                                      FALSE: find to be released page.
 * @param[out] a_management_page_count  Number of management pages found.
 * @param[out] a_data_page_count        Number of data pages found.
 *
 * @return PIFS_SUCCESS: if free pages found. PIFS_ERROR_GENERAL: if no free pages found.
 */
pifs_status_t pifs_get_pages(bool_t a_is_free,
                             pifs_size_t * a_management_page_count,
                             pifs_size_t * a_data_page_count)
{
    pifs_status_t           ret = PIFS_ERROR_GENERAL;
    pifs_block_address_t    fba = PIFS_FLASH_BLOCK_RESERVED_NUM;
    pifs_page_address_t     fpa = 0;
    pifs_block_address_t    ba = pifs.header.free_space_bitmap_address.block_address;
    pifs_page_address_t     pa = pifs.header.free_space_bitmap_address.page_address;
    pifs_page_offset_t      po = 0;
    pifs_size_t             i;
    uint8_t                 free_space_bitmap = 0;
    bool_t                  found = FALSE;
    bool_t                  end = FALSE;
    pifs_size_t             byte_cntr = PIFS_FREE_SPACE_BITMAP_SIZE_BYTE;
    pifs_bit_pos_t          bit_pos = 0;
    uint8_t                 mask = 1; /**< Mask for finding free page */
    uint8_t                 value = 1; /**< Value for finding free page */

    PIFS_ASSERT(pifs.is_header_found);

    if (!a_is_free)
    {
        /* Find to be released page */
        mask = 2;
        value = 0;
    }

    *a_management_page_count = 0;
    *a_data_page_count = 0;

    do
    {
        ret = pifs_read(ba, pa, po, &free_space_bitmap, sizeof(free_space_bitmap));
        if (ret == PIFS_SUCCESS)
        {
#if PIFS_DEBUG_LEVEL >= 6
            printf("%s ", pifs_byte2bin_str(free_space_bitmap));
            if (((po + 1) % 8) == 0)
            {
                printf("\r\n");
            }
#endif
            for (i = 0; i < (PIFS_BYTE_BITS / PIFS_FSBM_BITS_PER_PAGE); i++)
            {
                if ((free_space_bitmap & mask) == value)
                {
#if PIFS_CHECK_IF_PAGE_IS_ERASED
                    if (!a_is_free || pifs_is_page_erased(fba, fpa))
#endif
                    {
                        found = TRUE;
                        if (pifs_is_block_type(fba, PIFS_BLOCK_TYPE_DATA, &pifs.header))
                        {
                            (*a_data_page_count)++;
                        }
                        else if (pifs_is_block_type(fba, PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT, &pifs.header))
                        {
                            /* Only count primary management, because secondary
                             * management pages will be used when this management
                             * area is full.
                             */
                            (*a_management_page_count)++;
                        }
                    }
#if PIFS_CHECK_IF_PAGE_IS_ERASED
                    else
                    {
                        PIFS_ERROR_MSG("Flash page should be erased, but it is not! %s\r\n", pifs_ba_pa2str(fba, fpa));
                    }
#endif
                }
                free_space_bitmap >>= PIFS_FSBM_BITS_PER_PAGE;
                bit_pos += PIFS_FSBM_BITS_PER_PAGE;
                fpa++;
                if (fpa == PIFS_FLASH_PAGE_PER_BLOCK)
                {
                    fpa = 0;
                    fba++;
                    if (fba >= PIFS_FLASH_BLOCK_NUM_ALL)
                    {
                        end = TRUE;
                    }
                }
            }
            po++;
            if (po == PIFS_FLASH_PAGE_SIZE_BYTE)
            {
                po = 0;
                pa++;
                if (pa == PIFS_FLASH_PAGE_PER_BLOCK)
                {
                    pa = 0;
                    ba++;
                    if (ba >= PIFS_FLASH_BLOCK_NUM_ALL)
                    {
                        PIFS_FATAL_ERROR_MSG("End of flash! byte_cntr: %lu\r\n", byte_cntr);
                    }
                }
            }
        }
    } while (byte_cntr-- > 0 && ret == PIFS_SUCCESS && !end);

    if (ret == PIFS_SUCCESS && !found)
    {
        ret = PIFS_ERROR_NO_MORE_SPACE;
    }

    return ret;
}

/**
 * @brief pifs_get_pages Find to be released page(s) in free space memory bitmap.
 *
 * @param[out] a_management_page_count  Number of management pages found.
 * @param[out] a_data_page_count        Number of data pages found.
 *
 * @return PIFS_SUCCESS: if free pages found. PIFS_ERROR_GENERAL: if no free pages found.
 */
pifs_status_t pifs_get_to_be_released_pages(pifs_size_t * a_free_management_page_count,
                                            pifs_size_t * a_free_data_page_count)
{
    return pifs_get_pages(FALSE, a_free_management_page_count, a_free_data_page_count);
}

/**
 * @brief pifs_get_pages Find free page(s) in free space memory bitmap.
 *
 * @param[out] a_management_page_count  Number of management pages found.
 * @param[out] a_data_page_count        Number of data pages found.
 *
 * @return PIFS_SUCCESS: if free pages found. PIFS_ERROR_GENERAL: if no free pages found.
 */
pifs_status_t pifs_get_free_pages(pifs_size_t * a_free_management_page_count,
                                  pifs_size_t * a_free_data_page_count)
{
    return pifs_get_pages(TRUE, a_free_management_page_count, a_free_data_page_count);
}

/**
 * @brief pifs_get_free_space Find free page(s) in free space memory bitmap.
 *
 * @return PIFS_SUCCESS: if free pages found. PIFS_ERROR_GENERAL: if no free pages found.
 */
pifs_status_t pifs_get_free_space(pifs_size_t * a_free_management_bytes,
                                  pifs_size_t * a_free_data_bytes,
                                  pifs_size_t * a_free_management_page_count,
                                  pifs_size_t * a_free_data_page_count)
{
    pifs_status_t ret = PIFS_ERROR_GENERAL;

    ret = pifs_get_free_pages(a_free_management_page_count, a_free_data_page_count);

    if (ret == PIFS_SUCCESS)
    {
        *a_free_management_bytes = *a_free_management_page_count * PIFS_FLASH_PAGE_SIZE_BYTE;
        *a_free_data_bytes = *a_free_data_page_count * PIFS_FLASH_PAGE_SIZE_BYTE;
    }

    return ret;
}

/**
 * @brief pifs_check_free_space Debugging routine to check free space bitmap.
 *
 * @return
 */
pifs_status_t pifs_check_free_space(void)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba;
    pifs_page_address_t  pa;

    for (ba = PIFS_FLASH_BLOCK_RESERVED_NUM; ba < PIFS_FLASH_BLOCK_NUM_ALL; ba++)
    {
        for (pa = 0; pa < PIFS_FLASH_PAGE_PER_BLOCK; pa++)
        {
            if (pifs_is_page_free(ba, pa) && !pifs_is_page_erased(ba, pa))
            {
                PIFS_WARNING_MSG("%s is marked free, but it is programmed!\r\n",
                                 pifs_ba_pa2str(ba, pa));
                ret = PIFS_ERROR_GENERAL;
            }
            if (pifs_is_page_to_be_released(ba, pa) && pifs_is_page_erased(ba, pa))
            {
                PIFS_WARNING_MSG("%s is marked to be released, but it is erased!\r\n",
                                 pifs_ba_pa2str(ba, pa));
                ret = PIFS_ERROR_GENERAL;
            }
        }
    }

    return ret;
}
