/**
 * @file        pifs.c
 * @brief       Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-27 19:40:40 ivanovp {Time-stamp}
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

#if PIFS_MANAGEMENT_BLOCKS >= PIFS_FLASH_BLOCK_NUM_FS
#error PIFS_MANAGEMENT_BLOCKS is greater than PIFS_FLASH_BLOCK_NUM_FS!
#endif
#if PIFS_MANAGEMENT_BLOCKS > PIFS_FLASH_BLOCK_NUM_FS / 4
#warning PIFS_MANAGEMENT_BLOCKS is larger than PIFS_FLASH_BLOCK_NUM_FS / 4!
#endif
#if PIFS_MANAGEMENT_BLOCKS < 1
#error PIFS_MANAGEMENT_BLOCKS shall be 1 at minimum!
#endif

#define PIFS_MAP_PAGE_NUM               1   /**< Default number of map pages. Minimum: 1 */

static pifs_t pifs =
{
    .header_address = { PIFS_BLOCK_ADDRESS_INVALID, PIFS_PAGE_ADDRESS_INVALID },
    .is_header_found = FALSE,
    .header = { 0 },
//    .latest_object_id = 1,
    .cache_page_buf_address = { PIFS_BLOCK_ADDRESS_INVALID, PIFS_PAGE_ADDRESS_INVALID },
    .cache_page_buf = { 0 },
    .cache_page_buf_is_dirty = FALSE
};

static pifs_status_t pifs_find_page(pifs_page_count_t a_page_count_minimum,
                                    pifs_page_count_t a_page_count_desired,
                                    pifs_block_type_t a_block_type,
                                    bool_t a_is_free,
                                    pifs_block_address_t * a_block_address,
                                    pifs_page_address_t * a_page_address,
                                    pifs_page_count_t * a_page_count_found);
static bool_t pifs_is_buffer_erased(const void * a_buf, size_t a_buf_size);
static bool_t pifs_is_page_free(pifs_block_address_t a_block_address,
                                pifs_page_address_t a_page_address);
static bool_t pifs_is_page_to_be_released(pifs_block_address_t a_block_address,
                                          pifs_page_address_t a_page_address);
static pifs_status_t pifs_mark_page(pifs_block_address_t a_block_address,
                             pifs_page_address_t a_page_address,
                             pifs_page_count_t a_page_count, bool_t a_mark_used);
static pifs_status_t pifs_header_init(pifs_block_address_t a_block_address,
                                      pifs_page_address_t a_page_address,
                                      pifs_header_t * a_header);
static pifs_status_t pifs_header_write(pifs_block_address_t a_block_address,
                                       pifs_page_address_t a_page_address,
                                       pifs_header_t * a_header);

#if PIFS_DEBUG_LEVEL >= 1
/**
 * @brief address2str Convert address to human readable string.
 *
 * @param[in] a_address Address to convert.
 * @return The created string.
 */
static char * address2str(pifs_address_t * a_address)
{
    static char str[32];

    snprintf(str, sizeof(str), "BA%i/PA%i @0x%X", a_address->block_address, a_address->page_address,
           a_address->block_address * PIFS_FLASH_BLOCK_SIZE_BYTE
           + a_address->page_address * PIFS_FLASH_PAGE_SIZE_BYTE);

    return str;
}

/**
 * @brief ba_pa2str Convert adress to human readable string.
 *
 * @param[in] a_block_address Block address.
 * @param[in] a_page_address Page address.
 * @return The created string.
 */
static char * ba_pa2str(pifs_block_address_t a_block_address, pifs_page_address_t a_page_address)
{
    static char str[32];

    snprintf(str, sizeof(str), "BA%i/PA%i @0x%X", a_block_address, a_page_address,
           a_block_address * PIFS_FLASH_BLOCK_SIZE_BYTE
           + a_page_address * PIFS_FLASH_PAGE_SIZE_BYTE);

    return str;
}
#endif

#if PIFS_DEBUG_LEVEL >= 5
/**
 * @brief pifs_print_cache Print content of page buffer.
 */
static void pifs_print_cache(void)
{
    print_buffer(pifs.cache_page_buf, sizeof(pifs.cache_page_buf),
                 pifs.cache_page_buf_address.block_address * PIFS_FLASH_BLOCK_SIZE_BYTE
                 + pifs.cache_page_buf_address.page_address * PIFS_FLASH_PAGE_SIZE_BYTE);
}
#else
#define pifs_print_cache()
#endif

static inline bool_t pifs_is_address_valid(pifs_address_t * a_address)
{
    bool_t valid = (a_address->block_address < PIFS_FLASH_BLOCK_NUM_ALL)
            && (a_address->page_address < PIFS_FLASH_PAGE_PER_BLOCK);

    return valid;
}

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
static pifs_status_t pifs_calc_free_space_pos(const pifs_address_t * a_free_space_bitmap_address,
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
        bit_pos = ((a_block_address - PIFS_FLASH_BLOCK_RESERVED_NUM) * PIFS_FLASH_PAGE_PER_BLOCK + a_page_address) << 1;
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
                             ba_pa2str(a_block_address, a_page_address));
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
static void pifs_calc_address(pifs_bit_pos_t a_bit_pos,
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
static pifs_status_t pifs_flush(void)
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
                           address2str(&pifs.cache_page_buf_address));
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
static pifs_status_t pifs_read(pifs_block_address_t a_block_address,
                        pifs_page_address_t a_page_address,
                        pifs_page_offset_t a_page_offset,
                        void * const a_buf,
                        size_t a_buf_size)
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
 * @param[in] a_block_address Block address of page to write.
 * @param[in] a_page_address    Page address of page to write.
 * @param[in] a_page_offset     Offset in page.
 * @param[in] a_buf             Pointer to buffer to write or NULL if
 *                              pifs.cache_page_buf is directly written.
 * @param[in] a_buf_size        Size of buffer. Ignored if a_buf is NULL.
 * @return PIFS_SUCCESS if data write successfully.
 */
static pifs_status_t pifs_write(pifs_block_address_t a_block_address,
                                pifs_page_address_t a_page_address,
                                pifs_page_offset_t a_page_offset,
                                const void * const a_buf,
                                size_t a_buf_size)
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
static pifs_status_t pifs_erase(pifs_block_address_t a_block_address)
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
 * @brief pifs_read_delta_map_page Read delta map pages to memory buffer.
 *
 * @return PIFS_SUCCES
 */
static pifs_status_t pifs_read_delta_map_page(void)
{
    pifs_block_address_t ba = pifs.header.delta_map_address.block_address;
    pifs_page_address_t  pa = pifs.header.delta_map_address.page_address;
    size_t               i;
    pifs_status_t        ret = PIFS_SUCCESS;

    for (i = 0; i < PIFS_DELTA_MAP_PAGE_NUM && ret == PIFS_SUCCESS; i++)
    {
        ret = pifs_read(ba, pa, 0, &pifs.delta_map_page_buf[i], PIFS_FLASH_PAGE_SIZE_BYTE);
        pa++;
        if (pa == PIFS_FLASH_PAGE_PER_BLOCK)
        {
            pa = 0;
            ba++;
            if (ba >= PIFS_FLASH_BLOCK_NUM_ALL)
            {
                PIFS_ERROR_MSG("End of flash. %s", ba_pa2str(ba, pa));
                ret = PIFS_ERROR_INTERNAL_RANGE;
            }
        }
    }

    return ret;
}

/**
 * @brief pifs_write_delta_map_page Read delta map pages to memory buffer.
 *
 * @return PIFS_SUCCES
 */
static pifs_status_t pifs_write_delta_map_page(size_t delta_map_page_idx)
{
    pifs_block_address_t ba = pifs.header.delta_map_address.block_address;
    pifs_page_address_t  pa = pifs.header.delta_map_address.page_address;
    size_t               i;
    pifs_status_t        ret = PIFS_SUCCESS;

    /* TODO calculate delta page address instead of increasing */
    for (i = 0; i < PIFS_DELTA_MAP_PAGE_NUM && ret == PIFS_SUCCESS; i++)
    {
        if (i == delta_map_page_idx)
        {
            ret = pifs_write(ba, pa, 0, &pifs.delta_map_page_buf[i], PIFS_FLASH_PAGE_SIZE_BYTE);
            PIFS_DEBUG_MSG("%s ret: %i\r\n", ba_pa2str(ba, pa), ret);
            break;
        }
        pa++;
        if (pa == PIFS_FLASH_PAGE_PER_BLOCK)
        {
            pa = 0;
            ba++;
            if (ba >= PIFS_FLASH_BLOCK_NUM_ALL)
            {
                PIFS_ERROR_MSG("End of flash. %s", ba_pa2str(ba, pa));
                ret = PIFS_ERROR_INTERNAL_RANGE;
            }
        }
    }

    return ret;
}

static pifs_status_t pifs_find_delta_page(pifs_block_address_t a_block_address,
                                          pifs_page_address_t a_page_address,
                                          pifs_block_address_t * a_delta_block_address,
                                          pifs_page_address_t * a_delta_page_address,
                                          bool_t * a_is_map_full)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    size_t               i;
    size_t               j;
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
                                   ba_pa2str(a_block_address, a_page_address));
                    PIFS_DEBUG_MSG("%s\r\n",
                                   ba_pa2str(ba, pa));
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

static pifs_status_t pifs_append_delta_map_entry(pifs_delta_entry_t * a_new_delta_entry)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    size_t               i;
    size_t               j;
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

static pifs_status_t pifs_merge_management(void)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t hba = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_page_address_t  hpa = PIFS_PAGE_ADDRESS_INVALID;
    pifs_header_t        old_header = pifs.header;
    size_t               i;
    pifs_block_address_t ba = pifs.header.entry_list_address.block_address;
    pifs_page_address_t  pa = pifs.header.entry_list_address.page_address;
    pifs_entry_t       * entry = (pifs_entry_t*) pifs.cache_page_buf;

    for (i = 0; i < PIFS_MANAGEMENT_BLOCKS && ret == PIFS_SUCCESS; i++)
    {
        ret = pifs_erase(pifs.header.next_management_blocks[i]);
    }

    if (ret == PIFS_SUCCESS)
    {
        hba = old_header.next_management_blocks[0];
        hpa = 0;
        ret = pifs_header_init(hba, hpa, &pifs.header);
    }
    if (ret == PIFS_SUCCESS)
    {
        while (ba < old_header.entry_list_address.block_address + PIFS_MANAGEMENT_BLOCKS
               && ret == PIFS_SUCCESS)
        {
            while (pa < PIFS_FLASH_PAGE_PER_BLOCK && ret == PIFS_SUCCESS)
            {
                ret = pifs_read(ba, pa, 0, NULL, PIFS_ENTRY_PER_PAGE * PIFS_ENTRY_SIZE_BYTE);
                for (i = 0; i < PIFS_ENTRY_PER_PAGE; i++)
                {
                    /* Check if name matches */
                    if (!pifs_is_buffer_erased(&entry[i], PIFS_ENTRY_SIZE_BYTE)
                            && (entry[i].attrib != 0))
                    {
                        ret = pifs_write(ba, pa, 0, NULL, PIFS_ENTRY_PER_PAGE * PIFS_ENTRY_SIZE_BYTE);
                    }
                }
                pa++;
            }
            pa = 0;
            ba++;
        }
    }
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_header_write(hba, hpa, &pifs.header);
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
static pifs_status_t pifs_read_delta(pifs_block_address_t a_block_address,
                                     pifs_page_address_t a_page_address,
                                     pifs_page_offset_t a_page_offset,
                                     void * const a_buf,
                                     size_t a_buf_size)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba;
    pifs_page_address_t  pa;

    ret = pifs_find_delta_page(a_block_address, a_page_address, &ba, &pa, NULL);
    if (ret == PIFS_SUCCESS)
    {
        PIFS_DEBUG_MSG("%s\r\n",
                       ba_pa2str(ba, pa));
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
static pifs_status_t pifs_write_delta(pifs_block_address_t a_block_address,
                                      pifs_page_address_t a_page_address,
                                      pifs_page_offset_t a_page_offset,
                                      const void * const a_buf,
                                      size_t a_buf_size,
                                      bool_t * a_is_delta)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    size_t               i;
    size_t               j;
    pifs_delta_entry_t   delta_entry;
    bool_t               delta_needed = FALSE;
    bool_t               delta_written = FALSE;
    pifs_block_address_t ba;
    pifs_page_address_t  pa;
    uint8_t            * buf = (uint8_t*) a_buf;
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
        for (i = a_page_offset, j = 0; i < a_buf_size && !delta_needed; i++, j++)
        {
            /* Checking if only bit programming is needed.
             * When erased value if 0xFF, bit programming generated falling edge
             * on data.
             */
#if PIFS_FLASH_ERASED_BYTE_VALUE == 0xFF
            if ((pifs.page_buf[i] ^ buf[j]) & buf[j])         /* Detecting rising edge */
#else
            if ((pifs.page_buf[i] ^ buf[j]) & pifs.cache_page_buf[i]) /* Detecting falling edge */
#endif
            {
                /* Bit erasing would be necessary to store the data,
                 * therefore a delta page is needed.
                 */
                delta_needed = TRUE;
                PIFS_DEBUG_MSG("Delta page needed! orig: 0x%x new: 0x%X\r\n",
                               pifs.page_buf[i], buf[j]);
            }
        }
    }
    if (ret == PIFS_SUCCESS)
    {
        if (delta_needed)
        {
            /* Find a new data page */
            *a_is_delta = TRUE;
            if (is_delta_map_full)
            {
                PIFS_WARNING_MSG("Management blocks shall be merged!\r\n");
                ret = pifs_merge_management();
            }
            if (ret == PIFS_SUCCESS)
            {
                ret = pifs_find_page(1, 1, PIFS_BLOCK_TYPE_DATA, TRUE, &fba, &fpa, &page_count_found);
            }
            if (ret == PIFS_SUCCESS)
            {
                PIFS_DEBUG_MSG("free page %s\r\n", ba_pa2str(fba, fpa));

                delta_entry.orig_address.block_address = a_block_address;
                delta_entry.orig_address.page_address = a_page_address;
                delta_entry.delta_address.block_address = fba;
                delta_entry.delta_address.page_address = fpa;
                PIFS_DEBUG_MSG("delta page %s -> ",
                               ba_pa2str(a_block_address, a_page_address));
                PIFS_DEBUG_MSG("%s\r\n",
                               ba_pa2str(fba, fpa));
                ret = pifs_write(fba, fpa, a_page_offset, a_buf, PIFS_FLASH_PAGE_SIZE_BYTE);
                if (ret == PIFS_SUCCESS)
                {
                    ret = pifs_append_delta_map_entry(&delta_entry);
                }
                if (ret == PIFS_SUCCESS)
                {
                    /* Mark new page as used */
                    ret = pifs_mark_page(fba, fpa, 1, TRUE);
                    PIFS_DEBUG_MSG("Mark page %s as used: %i\r\n", ba_pa2str(fba, fpa), ret);
                }
                if (ret == PIFS_SUCCESS)
                {
                    /* Mark old page (original or previous delta)
                     * as to be released */
                    ret = pifs_mark_page(ba, pa, 1, FALSE);
                    PIFS_DEBUG_MSG("Mark page %s as to be released: %i\r\n", ba_pa2str(a_block_address, a_page_address), ret);
                    delta_written = TRUE;
                }
            }
        }
        else
        {
            /* No delta page needed, simple write */
            *a_is_delta = FALSE;
            ret = pifs_write(ba, pa, a_page_offset, a_buf, a_buf_size);
            if (!pifs_is_page_free(ba, pa))
            {
                PIFS_WARNING_MSG("Page is not free %s\r\n", ba_pa2str(ba, pa));
            }
            if (ret == PIFS_SUCCESS && pifs_is_page_free(ba, pa))
            {
                /* Mark new page as used */
                ret = pifs_mark_page(ba, pa, 1, TRUE);
            }
        }
    }

    return ret;
}

/**
 * @brief pifs_is_page_free Check if page is used.
 *
 * @param[in] a_block_address   Block address of page(s).
 * @param[in] a_page_address    Page address of page(s).
 * @return TRUE: page is free. FALSE: page is used.
 */
static bool_t pifs_is_page_free(pifs_block_address_t a_block_address,
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
static bool_t pifs_is_page_to_be_released(pifs_block_address_t a_block_address,
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
static pifs_status_t pifs_mark_page(pifs_block_address_t a_block_address,
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
//            print_buffer(pifs.cache_page_buf, sizeof(pifs.cache_page_buf), 0);
//            PIFS_DEBUG_MSG("-Free space byte:    0x%02X\r\n", pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS]);
            is_free_space = pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS] & (1u << (bit_pos % PIFS_BYTE_BITS));
            is_not_to_be_released = pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS] & (1u << ((bit_pos % PIFS_BYTE_BITS) + 1));
//            PIFS_DEBUG_MSG("-Free space bit:     %i\r\n", is_free_space);
//            PIFS_DEBUG_MSG("-Release space bit:  %i\r\n", is_not_to_be_released);
//            PIFS_DEBUG_MSG("-Free space bit:     %i\r\n", (pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS] >> (bit_pos % PIFS_BYTE_BITS)) & 1);
//            PIFS_DEBUG_MSG("-Release space bit:  %i\r\n", (pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS] >> ((bit_pos % PIFS_BYTE_BITS) + 1)) & 1);
            if (a_mark_used)
            {
                /* Mark page used */
                if (is_free_space)
                {
                    /* Clear free bit */
                    pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS] &= ~(1u << (bit_pos % PIFS_BYTE_BITS));
                }
                else
                {
                    /* The space has already allocated */
                    PIFS_FATAL_ERROR_MSG("Page has already allocated! %s\r\n", ba_pa2str(a_block_address, a_page_address));
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
                        PIFS_FATAL_ERROR_MSG("Page has already marked to be released %s\r\n", ba_pa2str(a_block_address, a_page_address));
                        ret = PIFS_ERROR_INTERNAL_ALLOCATION;
                    }
                }
                else
                {
                    /* The space has not yet allocated */
                    PIFS_FATAL_ERROR_MSG("Page has not yet allocated! %s\r\n", ba_pa2str(a_block_address, a_page_address));
                    ret = PIFS_ERROR_INTERNAL_ALLOCATION;
                }
            }
//            PIFS_DEBUG_MSG("+Free space byte:    0x%02X\r\n", pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS]);
//            PIFS_DEBUG_MSG("+Free space bit:     %i\r\n", (pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS] >> (bit_pos % PIFS_BYTE_BITS)) & 1);
//            PIFS_DEBUG_MSG("+Release space bit:  %i\r\n", (pifs.cache_page_buf[bit_pos / PIFS_BYTE_BITS] >> ((bit_pos % PIFS_BYTE_BITS) + 1)) & 1);
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
                    PIFS_FATAL_ERROR_MSG("Trying to mark invalid address! %s\r\n", ba_pa2str(a_block_address, a_page_address));
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
 * @brief pifs_is_block_type Checks if the given block address is block type.
 * @param[in] a_block_address Block address to check.
 * @param[in] a_block_type    Block type.
 * @return TRUE: If block address is equal to block type.
 */
static bool_t pifs_is_block_type(pifs_block_address_t a_block_address, pifs_block_type_t a_block_type)
{
    size_t               i = 0;
    bool_t               is_block_type = TRUE;

    is_block_type = (a_block_type == PIFS_BLOCK_TYPE_DATA);
#if PIFS_MANAGEMENT_BLOCKS > 1
    for (i = 0; i < PIFS_MANAGEMENT_BLOCKS; i++)
#endif
    {
        if (pifs.header.management_blocks[i] == a_block_address)
        {
            is_block_type = (a_block_type == PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT);
        }
        if (pifs.header.next_management_blocks[i] == a_block_address)
        {
            is_block_type = (a_block_type == PIFS_BLOCK_TYPE_SECONDARY_MANAGEMENT);
        }
    }
#if PIFS_FLASH_BLOCK_RESERVED_NUM
    if (a_block_address < PIFS_FLASH_BLOCK_RESERVED_NUM)
    {
        is_block_type = (a_block_type == PIFS_BLOCK_TYPE_RESERVED);
    }
#endif

    return is_block_type;
}

/**
 * @brief pifs_is_page_erased Checks if the given block address is block type.
 * @param[in] a_block_address Block address to check.
 * @param[in] a_page_address  Page address to check.
 * @return TRUE: If page is erased.
 */
static bool_t pifs_is_page_erased(pifs_block_address_t a_block_address,
                                  pifs_page_address_t a_page_address)
{
    pifs_status_t status;
    bool_t is_erased = FALSE;

    status = pifs_read(a_block_address, a_page_address, 0, NULL, 0);
    if (status == PIFS_SUCCESS)
    {
        is_erased = pifs_is_buffer_erased(pifs.cache_page_buf, PIFS_FLASH_PAGE_SIZE_BYTE);
    }
    return is_erased;
}

/**
 * @brief pifs_find_page Find free or to be released page(s) in free space
 * memory bitmap.
 * It tries to find 'a_page_count_desired' pages, but at least
 * 'a_page_count_minimum'.
 * FIXME there should be an option for maximum search time.
 *
 * @param[in] a_page_count_minimum Number of pages needed at least.
 * @param[in] a_page_count_desired Number of pages needed.
 * @param[in] a_blocktype          Block type to find.
 * @param[in] a_is_free            TRUE: find free page, FALSE: find to be released page.
 * @param[out] a_block_address     Block address of page(s).
 * @param[out] a_page_address      Page address of page(s).
 * @param[out] a_page_count_found  Number of free pages found.
 * @return PIFS_SUCCESS: if free pages found. PIFS_ERROR: if no free pages found.
 */
static pifs_status_t pifs_find_page(pifs_page_count_t a_page_count_minimum,
                                    pifs_page_count_t a_page_count_desired,
                                    pifs_block_type_t a_block_type,
                                    bool_t a_is_free,
                                    pifs_block_address_t * a_block_address,
                                    pifs_page_address_t * a_page_address,
                                    pifs_page_count_t * a_page_count_found)
{
    pifs_status_t           ret = PIFS_ERROR;
    pifs_block_address_t    fba = PIFS_FLASH_BLOCK_RESERVED_NUM;
    pifs_page_address_t     fpa = 0;
    pifs_block_address_t    fba_start = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_page_address_t     fpa_start = PIFS_PAGE_ADDRESS_INVALID;
    pifs_block_address_t    ba = pifs.header.free_space_bitmap_address.block_address;
    pifs_page_address_t     pa = pifs.header.free_space_bitmap_address.page_address;
    pifs_page_offset_t      po = 0;
    size_t                  i;
    pifs_page_count_t       page_count_found = 0;
    uint8_t                 free_space_bitmap = 0;
    bool_t                  found = FALSE;
    size_t                  byte_cntr = PIFS_FREE_SPACE_BITMAP_SIZE_BYTE;
    pifs_bit_pos_t          bit_pos = 0;
    uint8_t                 mask = 1; /**< Mask for finding free page */

    PIFS_ASSERT(pifs.is_header_found);

    if (!a_is_free)
    {
        /* Find to be released page */
        mask = 2;
    }

    do
    {
        ret = pifs_read(ba, pa, po, &free_space_bitmap, sizeof(free_space_bitmap));
        if (ret == PIFS_SUCCESS)
        {
//            PIFS_DEBUG_MSG("%s %i 0x%X\r\n", ba_pa2str(ba, pa), po, free_space_bitmap);
            for (i = 0; i < (PIFS_BYTE_BITS / 2) && !found; i++)
            {
                if ((free_space_bitmap & mask) && pifs_is_block_type(fba, a_block_type))
                {
#if PIFS_CHECK_IF_PAGE_IS_ERASED
                    if (pifs_is_page_erased(fba, fpa))
#endif
                    {
                        if (page_count_found == 0)
                        {
                            fba_start = fba;
                            fpa_start = fpa;
                        }
                        page_count_found++;
                        if (page_count_found >= a_page_count_minimum)
                        {
                            *a_block_address = fba_start;
                            *a_page_address = fpa_start;
                            *a_page_count_found = page_count_found;
                        }
                        if (page_count_found == a_page_count_desired)
                        {
                            found = TRUE;
                        }
                    }
#if PIFS_CHECK_IF_PAGE_IS_ERASED
                    else
                    {
                        PIFS_ERROR_MSG("Flash page should be erased, but it is not! %s\r\n", ba_pa2str(fba, fpa));
                    }
#endif
                }
                else
                {
                    page_count_found = 0;
                }
                free_space_bitmap >>= 2;
                bit_pos += 2;
                fpa++;
                if (fpa == PIFS_FLASH_PAGE_PER_BLOCK)
                {
                    fpa = 0;
                    fba++;
                    if (fba >= PIFS_FLASH_BLOCK_NUM_ALL)
                    {
                        ret = PIFS_ERROR_NO_MORE_SPACE;
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
    } while (byte_cntr-- > 0 && ret == PIFS_SUCCESS && !found);

    if (ret == PIFS_SUCCESS && !found)
    {
        ret = PIFS_ERROR_NO_MORE_SPACE;
    }

    return ret;
}

static char * byte2bin_str(uint8_t byte)
{
    uint8_t i;
    static char s[12];

    s[0] = 0;
    for (i = 0; i < PIFS_BYTE_BITS; i++)
    {
        strncat(s, (byte & 0x80) ? "1" : "0", sizeof(s));
        byte <<= 1;
    }
    return s;
}

/**
 * @brief pifs_get_free_pages Find free page(s) in free space memory bitmap.
 *
 * @return PIFS_SUCCESS: if free pages found. PIFS_ERROR: if no free pages found.
 */
static pifs_status_t pifs_get_free_pages(size_t * a_free_management_page_count,
                                         size_t * a_free_data_page_count)
{
    pifs_status_t           ret = PIFS_ERROR;
    pifs_block_address_t    fba = PIFS_FLASH_BLOCK_RESERVED_NUM;
    pifs_page_address_t     fpa = 0;
    pifs_block_address_t    ba = pifs.header.free_space_bitmap_address.block_address;
    pifs_page_address_t     pa = pifs.header.free_space_bitmap_address.page_address;
    pifs_page_offset_t      po = 0;
    size_t                  i;
    uint8_t                 free_space_bitmap = 0;
    bool_t                  found = FALSE;
    bool_t                  end = FALSE;
    size_t                  byte_cntr = PIFS_FREE_SPACE_BITMAP_SIZE_BYTE;
    pifs_bit_pos_t          bit_pos = 0;
    uint8_t                 mask = 1; /**< Mask for finding free page */

    PIFS_ASSERT(pifs.is_header_found);

#if 0
    if (!a_is_free)
    {
        /* Find to be released page */
        mask = 2;
    }
#endif

    *a_free_management_page_count = 0;
    *a_free_data_page_count = 0;

    do
    {
        ret = pifs_read(ba, pa, po, &free_space_bitmap, sizeof(free_space_bitmap));
        if (ret == PIFS_SUCCESS)
        {
#if PIFS_DEBUG_LEVEL >= 6
            printf("%s ", byte2bin_str(free_space_bitmap));
            if (((po + 1) % 8) == 0)
            {
                printf("\r\n");
            }
#endif
            for (i = 0; i < (PIFS_BYTE_BITS / 2); i++)
            {
                if ((free_space_bitmap & mask))
                {
#if PIFS_CHECK_IF_PAGE_IS_ERASED
                    if (pifs_is_page_erased(fba, fpa))
#endif
                    {
                        found = TRUE;
                        if (pifs_is_block_type(fba, PIFS_BLOCK_TYPE_DATA))
                        {
                            (*a_free_data_page_count)++;
                        }
                        else if (pifs_is_block_type(fba, PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT))
                        {
                            /* Only count primary management, because secondary
                             * management pages will be used when this management
                             * area is full.
                             */
                            (*a_free_management_page_count)++;
                        }
                    }
#if PIFS_CHECK_IF_PAGE_IS_ERASED
                    else
                    {
                        PIFS_ERROR_MSG("Flash page should be erased, but it is not! %s\r\n", ba_pa2str(fba, fpa));
                    }
#endif
                }
                free_space_bitmap >>= 2;
                bit_pos += 2;
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
 * @brief pifs_get_free_space Find free page(s) in free space memory bitmap.
 *
 * @return PIFS_SUCCESS: if free pages found. PIFS_ERROR: if no free pages found.
 */
pifs_status_t pifs_get_free_space(size_t * a_free_management_bytes,
                                  size_t * a_free_data_bytes,
                                  size_t * a_free_management_page_count,
                                  size_t * a_free_data_page_count)
{
    pifs_status_t           ret = PIFS_ERROR;

    ret = pifs_get_free_pages(a_free_management_page_count, a_free_data_page_count);

    if (ret == PIFS_SUCCESS)
    {
        *a_free_management_bytes = *a_free_management_page_count * PIFS_FLASH_PAGE_SIZE_BYTE;
        *a_free_data_bytes = *a_free_data_page_count * PIFS_FLASH_PAGE_SIZE_BYTE;
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
static pifs_status_t pifs_header_init(pifs_block_address_t a_block_address,
                                      pifs_page_address_t a_page_address,
                                      pifs_header_t * a_header)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    size_t               i = 0;
    pifs_block_address_t ba = a_block_address;
    /* FIXME use random block? */
    PIFS_DEBUG_MSG("Creating managamenet block %s\r\n", ba_pa2str(a_block_address, a_page_address));
    a_header->magic = PIFS_MAGIC;
#if ENABLE_PIFS_VERSION
    a_header->majorVersion = PIFS_MAJOR_VERSION;
    a_header->minorVersion = PIFS_MINOR_VERSION;
#endif
    a_header->counter++;
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
        a_header->next_management_blocks[i] = ba;
#if PIFS_MANAGEMENT_BLOCKS > 1
        ba++;
        if (ba >= PIFS_FLASH_BLOCK_NUM_ALL)
        {
            ba = PIFS_FLASH_BLOCK_RESERVED_NUM;
        }
#endif
    }
    a_header->checksum = pifs_calc_header_checksum(a_header);

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
static pifs_status_t pifs_header_write(pifs_block_address_t a_block_address,
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
        /* Mark first delta page map as used */
        ret = pifs_mark_page(a_header->delta_map_address.block_address,
                             a_header->delta_map_address.page_address,
                             PIFS_DELTA_MAP_PAGE_NUM, TRUE);
    }

    return ret;
}

#include "buffer.h" // FIXME DEBUG

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
    size_t               free_management_bytes;
    size_t               free_data_bytes;
    size_t               free_management_pages;
    size_t               free_data_pages;

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
    PIFS_INFO_MSG("Object ID size:                     %lu bytes\r\n", sizeof(pifs_object_id_t));
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
        PIFS_ERROR_MSG("Cannot fit data into management block!\r\n");
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
                PIFS_DEBUG_MSG("Management page found: %s\r\n", ba_pa2str(ba, pa));
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
            ret = pifs_header_init(ba, pa, &pifs.header);
            if (ret == PIFS_SUCCESS)
            {
                ret = pifs_header_write(ba, pa, &pifs.header);
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
                          address2str(&pifs.header.entry_list_address));
            PIFS_INFO_MSG("Free space bitmap at %s\r\n",
                          address2str(&pifs.header.free_space_bitmap_address));
            PIFS_INFO_MSG("Delta page map at %s\r\n",
                          address2str(&pifs.header.delta_map_address));
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
                    printf("%i %i\r\n", i, pifs_is_block_type(i, PIFS_BLOCK_TYPE_DATA));
                }
                printf("PRIMARY MANAGEMENT blocks\r\n");
                for (i = 0; i < PIFS_FLASH_BLOCK_NUM_ALL; i++)
                {
                    printf("%i %i\r\n", i, pifs_is_block_type(i, PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT));
                }
                printf("SECONDARY MANAGEMENT blocks\r\n");
                for (i = 0; i < PIFS_FLASH_BLOCK_NUM_ALL; i++)
                {
                    printf("%i %i\r\n", i, pifs_is_block_type(i, PIFS_BLOCK_TYPE_SECONDARY_MANAGEMENT));
                }
                printf("RESERVED blocks\r\n");
                for (i = 0; i < PIFS_FLASH_BLOCK_NUM_ALL; i++)
                {
                    printf("%i %i\r\n", i, pifs_is_block_type(i, PIFS_BLOCK_TYPE_RESERVED));
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
                                   ba_pa2str(ba0, pa0), count0, ret);
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
 * @brief pifs_is_buffer_erased Check if buffer is erased or contains
 * programmed bytes.
 *
 * @param[in] a_buf[in]         Pointer to buffer.
 * @param[in] a_buf_size[in]    Size of buffer.
 * @return TRUE: if buffer is erased.
 * FALSE: if buffer contains at least one programmed bit.
 */
static bool_t pifs_is_buffer_erased(const void * a_buf, size_t a_buf_size)
{
    uint8_t * buf = (uint8_t*) a_buf;
    size_t    i;
    bool_t    ret = TRUE;

    for (i = 0; i < a_buf_size && ret; i++)
    {
        if (buf[i] != PIFS_FLASH_ERASED_BYTE_VALUE)
        {
            ret = FALSE;
        }
    }

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
    size_t               i;

    /* Invert attribute bits */
    a_entry->attrib ^= a_entry->attrib;
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
    a_entry->attrib ^= a_entry->attrib;

    return ret;
}

/**
 * @brief pifs_find_entry Find entry in entry list.
 *
 * @param a_name[in]    Pointer to name to find.
 * @param a_entry[out]  Pointer to entry to fill. NULL: clear entry.
 * @return PIFS_SUCCESS if entry found.
 * PIFS_ERROR_ENTRY_NOT_FOUND if entry not found.
 */
static pifs_status_t pifs_find_entry(const char * a_name, pifs_entry_t * const a_entry)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba = pifs.header.entry_list_address.block_address;
    pifs_page_address_t  pa = pifs.header.entry_list_address.page_address;
    bool_t               found = FALSE;
    pifs_entry_t       * entry = (pifs_entry_t*) pifs.cache_page_buf;
    size_t               i;

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
                        a_entry->attrib ^= a_entry->attrib;
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
        ret = PIFS_ERROR_ENTRY_NOT_FOUND;
    }

    return ret;
}

/**
 * @brief pifs_clear_entry Find entry in entry list and invalidate it.
 *
 * @param a_name[in]    Pointer to name to find.
 * @return PIFS_SUCCESS if entry found and cleared.
 * PIFS_ERROR_ENTRY_NOT_FOUND if entry not found.
 */
static pifs_status_t pifs_clear_entry(const char * a_name)
{
    return pifs_find_entry(a_name, NULL);
}

/**
 * @brief pifs_parse_open_mode Parse string of open mode.
 *
 * @param a_file[in]    Pointer to file's internal structure.
 * @param a_modes[in]   String of mode.
 */
static void pifs_parse_open_mode(pifs_file_t * a_file, const char * a_modes)
{
    uint8_t i;

    /* Reset mode */
    a_file->mode_create_new_file = FALSE;
    a_file->mode_read = FALSE;
    a_file->mode_write = FALSE;
    a_file->mode_append = FALSE;
    a_file->mode_file_shall_exist = FALSE;
    for (i = 0; a_modes[i] && i < 4; i++)
    {
        switch (a_modes[i])
        {
            case 'r':
                /* Read */
                a_file->mode_read = TRUE;
                a_file->mode_file_shall_exist = TRUE;
                break;
            case 'w':
                /* Write */
                a_file->mode_write = TRUE;
                a_file->mode_create_new_file = TRUE;
                break;
            case '+':
                if (a_file->mode_write)
                {
                    /* mode "w+" */
                    a_file->mode_read = TRUE;
                    a_file->mode_create_new_file = TRUE;
                }
                else if (a_file->mode_read)
                {
                    /* mode "r+" */
                    a_file->mode_write = TRUE;
                    a_file->mode_file_shall_exist = TRUE;
                }
                else if (a_file->mode_append)
                {
                    /* mode "a+" */
                    a_file->mode_read = TRUE;
                }
                break;
            case 'a':
                a_file->mode_append = TRUE;
                break;
            case 'b':
                /* Binary, all operations are binary! */
                break;
            default:
                a_file->status = PIFS_ERROR_INVALID_OPEN_MODE;
                PIFS_ERROR_MSG("Invalid open mode '%s'\r\n", a_modes);
                break;
        }
    }

    PIFS_DEBUG_MSG("create_new_file: %i\r\n", a_file->mode_create_new_file);
    PIFS_DEBUG_MSG("read: %i\r\n", a_file->mode_read);
    PIFS_DEBUG_MSG("write: %i\r\n", a_file->mode_write);
    PIFS_DEBUG_MSG("append: %i\r\n", a_file->mode_append);
    PIFS_DEBUG_MSG("file_shall_exist: %i\r\n", a_file->mode_file_shall_exist);
}

/**
 * @brief pifs_find_file_pages Mark file map and file's pages to be released.
 *
 * @param a_file[in] Pointer of file structure.
 * @return TRUE: if all pages were succesfully marked to be released.
 */
static pifs_status_t pifs_find_file_pages(pifs_file_t * a_file)
{
    pifs_block_address_t    ba = a_file->entry.first_map_address.block_address;
    pifs_page_address_t     pa = a_file->entry.first_map_address.page_address;
    pifs_block_address_t    mba;
    pifs_page_address_t     mpa;
    pifs_page_count_t       page_count;
    size_t                  i;
    bool_t                  erased = FALSE;
    pifs_page_offset_t      po = PIFS_MAP_HEADER_SIZE_BYTE;

    PIFS_DEBUG_MSG("Searching in map entry at %s\r\n", ba_pa2str(ba, pa));

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
                                       ba_pa2str(mba, mpa));
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
                   address2str(&a_file->actual_map_address));
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
    }

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
static pifs_status_t pifs_append_map_entry(pifs_file_t * a_file,
                                           pifs_block_address_t a_block_address,
                                           pifs_page_address_t a_page_address,
                                           pifs_page_count_t a_page_count)
{
    pifs_block_address_t    ba = a_file->actual_map_address.block_address;
    pifs_page_address_t     pa = a_file->actual_map_address.page_address;
    bool_t                  is_written = FALSE;
    bool_t                  empty_entry_found = FALSE;
    pifs_page_count_t       page_count_found = 0;

    PIFS_DEBUG_MSG("Actual map address %s\r\n",
                   address2str(&a_file->actual_map_address));
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
        a_file->status = pifs_find_page(PIFS_MAP_PAGE_NUM, PIFS_MAP_PAGE_NUM, PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT, TRUE,
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
                           address2str(&a_file->actual_map_address));
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
                           address2str(&a_file->actual_map_address));
//            pifs_print_cache();
            a_file->map_entry_idx = 0;
        }
        if (a_file->status == PIFS_SUCCESS)
        {
            PIFS_DEBUG_MSG("### Mark page %s ###\r\n",
                           address2str(&a_file->actual_map_address));
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
                       ba_pa2str(a_block_address, a_page_address));
        a_file->status = pifs_write(ba, pa, PIFS_MAP_HEADER_SIZE_BYTE
                                    + a_file->map_entry_idx * PIFS_MAP_ENTRY_SIZE_BYTE,
                                    &a_file->map_entry,
                                    PIFS_MAP_ENTRY_SIZE_BYTE);
        PIFS_DEBUG_MSG("### New map entry %s ###\r\n",
                       ba_pa2str(ba, pa));
//        pifs_print_cache();
        is_written = TRUE;
    }

    return a_file->status;
}

/**
 * @brief pifs_fopen Open file, works like fopen().
 * Note: "a", "a+" not handled correctly.
 *
 * @param[in] a_filename    File name to open.
 * @param[in] a_modes       Open mode: "r", "r+", "w", "w+", "a" or "a+".
 * @return Pointer to file if file opened successfully.
 */
P_FILE * pifs_fopen(const char * a_filename, const char * a_modes)
{
    /* TODO search first free element of pifs.file[] */
    pifs_file_t        * file = &pifs.file[0];
    pifs_entry_t       * entry = &pifs.file[0].entry;
    pifs_block_address_t ba = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_page_address_t  pa = PIFS_PAGE_ADDRESS_INVALID;
    pifs_page_count_t    page_count_found = 0;
    size_t               free_management_pages = 0;
    size_t               free_data_pages = 0;

    /* TODO check validity of filename */
    if (pifs.is_header_found && strlen(a_filename))
    {
        file->status = PIFS_SUCCESS;
        file->write_pos = 0;
        file->write_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
        file->write_address.page_address = PIFS_PAGE_ADDRESS_INVALID;
        file->read_pos = 0;
        file->actual_map_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
        file->actual_map_address.page_address = PIFS_PAGE_ADDRESS_INVALID;
        pifs_parse_open_mode(file, a_modes);
        if (file->status == PIFS_SUCCESS)
        {
            file->status = pifs_find_entry(a_filename, entry);
            if (file->mode_file_shall_exist && file->status == PIFS_SUCCESS)
            {
                PIFS_DEBUG_MSG("Entry of %s found:\r\n", a_filename);
#if PIFS_DEBUG_LEVEL >= 6
                print_buffer(entry, sizeof(pifs_entry_t), 0);
#endif
                file->is_opened = TRUE;
            }
            if (file->mode_create_new_file)
            {
                pifs_get_free_pages(&free_management_pages, &free_data_pages);
                if (file->status == PIFS_SUCCESS && free_data_pages > 0)
                {
                    /* File already exist */
                    file->status = pifs_clear_entry(a_filename);
                    if (file->status == PIFS_SUCCESS)
                    {
                        /* Mark allocated pages to be released */
                        file->status = pifs_find_file_pages(file);
                    }
                }
                else
                {
                    /* File does not exists, no problem, we'll create it */
                    file->status = PIFS_SUCCESS;
                }
                /* Order of steps to create a file: */
                /* #1 Find a free page for map of file */
                /* #2 Create entry of file, which contains the map's address */
                /* #3 Mark map page */
                if (file->status == PIFS_SUCCESS)
                {
                    file->status = pifs_find_page(PIFS_MAP_PAGE_NUM, PIFS_MAP_PAGE_NUM, PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT, TRUE,
                                                  &ba, &pa, &page_count_found);
                }
                if (file->status == PIFS_SUCCESS)
                {
                    PIFS_DEBUG_MSG("Map page: %u free page found %s\r\n", page_count_found, ba_pa2str(ba, pa));
                    strncpy((char*)entry->name, a_filename, PIFS_FILENAME_LEN_MAX);
//                    entry->object_id = pifs.latest_object_id++;
                    entry->attrib = PIFS_ATTRIB_ARCHIVE;
                    entry->first_map_address.block_address = ba;
                    entry->first_map_address.page_address = pa;
                    file->status = pifs_append_entry(entry);
                    if (file->status == PIFS_SUCCESS)
                    {
                        PIFS_DEBUG_MSG("Entry created\r\n");
                        file->status = pifs_mark_page(ba, pa, PIFS_MAP_PAGE_NUM, TRUE);
                        if (file->status == PIFS_SUCCESS)
                        {
                            file->is_opened = TRUE;
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
            if (file->is_opened)
            {
                file->status = pifs_read_first_map_entry(file);
                PIFS_ASSERT(file->status == PIFS_SUCCESS);
                file->read_address = file->map_entry.address;
                file->read_page_count = file->map_entry.page_count;
                PIFS_DEBUG_MSG("read_address: %s read_page_count: %i\r\n",
                               address2str(&file->read_address), file->read_page_count);
            }
        }
    }

    return file->is_opened ? (P_FILE*) file : (P_FILE*) NULL;
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
    size_t               data_size = a_size * a_count;
    size_t               written_size = 0;
    pifs_page_count_t    page_count_needed = 0;
    pifs_page_count_t    page_count_needed_limited = 0;
    pifs_page_count_t    page_count_found;
    pifs_page_count_t    page_cound_found_start;
    size_t               chunk_size;
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
            PIFS_ASSERT(pifs_is_address_valid(&file->write_address.block_address));
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

        if (file->status == PIFS_SUCCESS && data_size > 0)
        {
            page_count_needed = (data_size + PIFS_FLASH_PAGE_SIZE_BYTE - 1) / PIFS_FLASH_PAGE_SIZE_BYTE;
            do
            {
                page_count_needed_limited = page_count_needed;
                if (page_count_needed_limited >= PIFS_PAGE_COUNT_INVALID)
                {
                    page_count_needed_limited = PIFS_PAGE_COUNT_INVALID - 1;
                }
                file->status = pifs_find_page(1, page_count_needed_limited, PIFS_BLOCK_TYPE_DATA, TRUE,
                                              &ba, &pa, &page_count_found);
                PIFS_DEBUG_MSG("%u pages found. %s\r\n", page_count_found, ba_pa2str(ba, pa));
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
                        PIFS_DEBUG_MSG("%s is_delta: %i status: %i\r\n", ba_pa2str(ba, pa),
                                       is_delta, file->status);
                        /* Save last page's address for future use */
                        file->write_address.block_address = ba;
                        file->write_address.page_address = pa;
                        pa++;
                        if (pa == PIFS_FLASH_PAGE_PER_BLOCK)
                        {
                            pa = 0;
                            ba++;
                            if (ba >= PIFS_FLASH_BLOCK_NUM_ALL)
                            {
                                PIFS_FATAL_ERROR_MSG("Trying to write to invalid flash address! %s\r\n",
                                                     ba_pa2str(ba, pa));
                            }
                        }
                        data += chunk_size;
                        data_size -= chunk_size;
                        written_size += chunk_size;
                        page_count_found--;
                        page_count_needed--;
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
        file->write_pos += written_size;
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
    //PIFS_DEBUG_MSG("started %s\r\n", address2str(&a_file->read_address));
    a_file->read_page_count--;
    if (a_file->read_page_count)
    {
        a_file->read_address.page_address++;
        if (a_file->read_address.page_address >= PIFS_FLASH_PAGE_PER_BLOCK)
        {
            a_file->read_address.page_address = 0;
            a_file->read_address.block_address++;
            if (a_file->read_address.block_address == PIFS_FLASH_BLOCK_NUM_ALL)
            {
                PIFS_FATAL_ERROR_MSG("Trying to read from invalid address! %s\r\n",
                                     address2str(&a_file->read_address));
            }
        }
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
    //  PIFS_DEBUG_MSG("exited %s\r\n", address2str(&a_file->read_address));
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
    size_t               read_size = 0;
    size_t               chunk_size = 0;
    size_t               data_size = a_size * a_count;
    size_t               page_count = 0;
    pifs_page_offset_t   po;

    if (pifs.is_header_found && file && file->is_opened && file->mode_read)
    {
        po = file->read_pos % PIFS_FLASH_PAGE_SIZE_BYTE;
        /* Check if last page was not fully read */
        if (po)
        {
            /* There is some data in the last page */
            //          PIFS_DEBUG_MSG("po != 0  %s\r\n", address2str(&file->read_address));
            PIFS_ASSERT(pifs_is_address_valid(&file->read_address.block_address));
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
                PIFS_ASSERT(pifs_is_address_valid(&file->read_address.block_address));
                chunk_size = PIFS_MIN(data_size, PIFS_FLASH_PAGE_SIZE_BYTE);
                //PIFS_DEBUG_MSG("read %s\r\n", address2str(&file->read_address));
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
        pifs_flush();
        file->is_opened = FALSE;
        ret = 0;
    }

    return ret;
}

/**
 * @brief pifs_remove Remove file.
 *
 * @param[in] a_filename Pointer to filename to be removed.
 * @return 0 if file removed. -1 if file not found.
 */
int pifs_remove(const char * a_filename)
{
    int ret = -1; /* FIXME error code! */
    pifs_file_t * file;

    file = (pifs_file_t*) pifs_fopen(a_filename, "r");
    if (file)
    {
        /* File already exist */
        file->status = pifs_clear_entry(a_filename);
        if (file->status == PIFS_SUCCESS)
        {
            /* Mark allocated pages to be released */
            file->status = pifs_find_file_pages(file);
        }
        ret = pifs_fclose(file);
    }

    return ret;
}
