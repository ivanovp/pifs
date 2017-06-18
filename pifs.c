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

/** Number of bits in a byte */
#define BYTE_BITS   8

static pifs_t pifs =
{
    .mgmt_address = { PIFS_BLOCK_ADDRESS_INVALID, PIFS_PAGE_ADDRESS_INVALID },
    .is_mgmt_found = FALSE,
    .header = { 0 },
    .latest_object_id = 1,
    .page_buf = { 0 }
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
 * @param[in] a_free_space_bitmap_address   Start address of free space bitmap area.
 * @param[in] a_block_address   Block address of page to be calculated.
 * @param[in] a_page_address    Page address of page to be calculated.
 * @param[out] a_free_space_block_address Block address of free space memory map.
 * @param[out] a_free_space_page_address  Page address of free space memory map.
 * @param[out] a_free_space_bit_pos       Bit position in free space memory map.
 */
void pifs_calc_free_space_pos(const pifs_address_t * a_free_space_bitmap_address,
                              pifs_block_address_t a_block_address,
                              pifs_page_address_t a_page_address,
                              pifs_block_address_t * a_free_space_block_address,
                              pifs_page_address_t * a_free_space_page_address,
                              uint32_t * a_free_space_bit_pos)
{
    uint32_t bit_pos;

    /* Shift left by one (<< 1) due to two bits are stored in free space bitmap */
    bit_pos = ((a_block_address - PIFS_FLASH_BLOCK_RESERVED_NUM) * PIFS_FLASH_PAGE_PER_BLOCK + a_page_address) << 1;
//    PIFS_DEBUG_MSG("BA%i/PA%i bit_pos: %i\r\n", a_block_address, a_page_address, bit_pos);
    *a_free_space_block_address = a_free_space_bitmap_address->block_address + bit_pos / PIFS_FLASH_BLOCK_SIZE_BYTE;
    bit_pos %= PIFS_FLASH_BLOCK_SIZE_BYTE;
    *a_free_space_page_address = a_free_space_bitmap_address->page_address + bit_pos / PIFS_FLASH_PAGE_SIZE_BYTE;
    bit_pos %= PIFS_FLASH_PAGE_SIZE_BYTE;
    *a_free_space_bit_pos = bit_pos;
}

/**
 * @brief pifs_page_mark Mark page(s) as used (or to be released) in free space
 * memory bitmap.
 * @param a_block_address Block address of page(s).
 * @param a_page_address Page address of page(s).
 * @param a_page_count Number of pages.
 * @param mark_used TRUE: Mark page used, FALSE: mark page to be released.
 * @return PIFS_SUCCESS: if page was successfully marked.
 */
pifs_status_t pifs_page_mark(pifs_block_address_t a_block_address,
                                    pifs_page_address_t a_page_address,
                                    size_t a_page_count, bool_t a_mark_used)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    uint32_t             bit_pos;
    pifs_block_address_t ba;
    pifs_page_address_t  pa;
    pifs_block_address_t prev_ba = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_page_address_t  prev_pa = PIFS_PAGE_ADDRESS_INVALID;
    bool_t               is_free_space;
    bool_t               is_to_be_released;

    while (a_page_count > 0 && ret == PIFS_SUCCESS)
    {
        pifs_calc_free_space_pos(&pifs.header.free_space_bitmap_address,
                                 a_block_address, a_page_address, &ba, &pa, &bit_pos);
        /* Check if page has not already read */
        if (prev_ba != ba || prev_pa != pa)
        {
            if (prev_ba != PIFS_BLOCK_ADDRESS_INVALID && prev_pa != PIFS_PAGE_ADDRESS_INVALID)
            {
                /* Write new status */
                ret = pifs_flash_write(prev_ba, prev_pa, 0, pifs.page_buf, sizeof(pifs.page_buf));
            }
            /* Read actual status of free space memory bitmap */
            ret = pifs_flash_read(ba, pa, 0, pifs.page_buf, sizeof(pifs.page_buf));
            prev_ba = ba;
            prev_pa = pa;
        }
        if (ret == PIFS_SUCCESS)
        {
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
            /* Write new status */
            ret = pifs_flash_write(ba, pa, 0, pifs.page_buf, sizeof(pifs.page_buf));
        }
    }

    return ret;
}

/**
 * @brief pifs_page_find_free Find free page(s) in free space memory bitmap.
 * @param[in] a_page_count_needed Number of pages needed.
 * @param[in] a_page_type         Page type to find.
 * @param[out] a_block_address    Block address of page(s).
 * @param[out] a_page_address     Page address of page(s).
 * @param[out] a_page_count_found Number of free pages found.
 * @return PIFS_SUCCESS: if free pages found. PIFS_ERROR: if no free pages found.
 */
pifs_status_t pifs_page_find_free(size_t a_page_count_needed,
                                  pifs_page_type_t a_page_type,
                                  pifs_block_address_t * a_block_address,
                                  pifs_page_address_t * a_page_address,
                                  size_t * a_page_count_found)
{
    pifs_status_t ret = PIFS_ERROR;

    return ret;
}

pifs_status_t pifs_init(void)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_block_address_t ba;
    pifs_page_address_t  pa;
    pifs_header_t        prev_header;
    pifs_checksum_t      checksum;

    pifs.is_mgmt_found = FALSE;

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
        for (ba = PIFS_FLASH_BLOCK_RESERVED_NUM; ba < PIFS_FLASH_BLOCK_NUM_ALL && !pifs.is_mgmt_found; ba++)
        {
            for (pa = 0; pa < PIFS_MANAGEMENT_PAGE_PER_BLOCK_MAX && !pifs.is_mgmt_found; pa++)
            {
                pifs_flash_read(ba, pa, 0, &pifs.header, sizeof(pifs.header));
                if (pifs.header.magic == PIFS_MAGIC
        #if ENABLE_PIFS_VERSION
                        && pifs.header.majorVersion == PIFS_MAJOR_VERSION
                        && pifs.header.minorVersion == PIFS_MINOR_VERSION
        #endif
                        )
                {
                    PIFS_INFO_MSG("Management page found: BA%i/PA%i\r\n", ba, pa);
                    checksum = pifs_calc_header_checksum(&pifs.header);
                    if (checksum == pifs.header.checksum)
                    {
                        PIFS_INFO_MSG("Checksum is valid\r\n");
                        if (!pifs.is_mgmt_found || prev_header.counter < pifs.header.counter)
                        {
                            pifs.is_mgmt_found = TRUE;
                            pifs.mgmt_address.block_address = ba;
                            pifs.mgmt_address.page_address = pa;
                            memcpy(&prev_header, &pifs.header, sizeof(prev_header));
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

        if (!pifs.is_mgmt_found)
        {
            /* FIXME use random block? */
#if 1
            ba = PIFS_FLASH_BLOCK_RESERVED_NUM;
#else
            ba = PIFS_FLASH_BLOCK_RESERVED_NUM + rand() % (PIFS_FLASH_BLOCK_NUM_FS);
#endif
            pa = 0;
            PIFS_INFO_MSG("Creating managamenet block BA%i/PA%i\r\n", ba, pa);
            pifs.header.magic = PIFS_MAGIC;
#if ENABLE_PIFS_VERSION
            pifs.header.majorVersion = PIFS_MAJOR_VERSION;
            pifs.header.minorVersion = PIFS_MINOR_VERSION;
#endif
            pifs.header.counter = 1;
            pifs.header.block_type = PIFS_BLOCK_TYPE_MANAGEMENT;
            pifs.header.entry_list_address.block_address = ba;
            pifs.header.entry_list_address.page_address = pa + PIFS_HEADER_SIZE_PAGE;
            pifs.header.free_space_bitmap_address.block_address = ba;
            pifs.header.free_space_bitmap_address.page_address = pa + PIFS_HEADER_SIZE_PAGE + PIFS_ENTRY_LIST_SIZE_PAGE;
            pifs.header.checksum = pifs_calc_header_checksum(&pifs.header);

            ret = pifs_flash_erase(ba);
            if (ret == PIFS_SUCCESS)
            {
                ret = pifs_flash_write(ba, pa, 0, &pifs.header, sizeof(pifs.header));
            }
            if (ret == PIFS_SUCCESS)
            {
                pifs.is_mgmt_found = TRUE;
                pifs.mgmt_address.block_address = ba;
                pifs.mgmt_address.page_address = pa;

                /* Mark file system header as used */
                ret = pifs_page_mark(pifs.mgmt_address.block_address,
                                     pifs.mgmt_address.page_address,
                                     PIFS_HEADER_SIZE_PAGE, TRUE);
            }
            if ( ret == PIFS_SUCCESS )
            {
                /* Mark entry list as used */
                ret = pifs_page_mark(pifs.header.entry_list_address.block_address,
                                     pifs.header.entry_list_address.page_address,
                                     PIFS_ENTRY_LIST_SIZE_PAGE, TRUE);
            }
            if ( ret == PIFS_SUCCESS )
            {
                /* Mark free space bitmap as used */
                ret = pifs_page_mark(pifs.header.free_space_bitmap_address.block_address,
                                     pifs.header.free_space_bitmap_address.page_address,
                                     PIFS_FREE_SPACE_BITMAP_SIZE_PAGE, TRUE);
            }
        }

        if (pifs.is_mgmt_found)
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
    pifs_page_offset_t   po;
    pifs_entry_t         entry;
    bool_t               created = FALSE;

    while (pa < PIFS_FLASH_PAGE_PER_BLOCK && !created)
    {
        while (po < PIFS_FLASH_PAGE_SIZE_BYTE && !created)
        {
            ret = pifs_flash_read(ba, pa, po, &entry, sizeof(entry));
            if (ret == PIFS_SUCCESS)
            {
                /* Check if this area is used */
                if (pifs_is_buffer_erased(&entry, sizeof(entry)))
                {
                    /* Empty entry found */
                    ret = pifs_flash_write(ba, pa, po, a_entry, sizeof(pifs_entry_t));
                    if (ret == PIFS_SUCCESS)
                    {
                        created = TRUE;
                    }
                }
            }
            po += sizeof(pifs_entry_t);
        }
        pa++;
    }

    return ret;
}

pifs_status_t pifs_find_entry(const char * a_name, pifs_entry_t * const a_entry)
{
    pifs_status_t        ret = PIFS_ERROR;
    pifs_block_address_t ba = pifs.header.entry_list_address.block_address;
    pifs_page_address_t  pa = pifs.header.entry_list_address.page_address;
    pifs_page_offset_t   po = 0;
    bool_t               found = FALSE;

    while (pa < PIFS_FLASH_PAGE_PER_BLOCK && !found)
    {
        po = 0;
        while (po < PIFS_FLASH_PAGE_SIZE_BYTE && !found)
        {
            ret = pifs_flash_read(ba, pa, po, a_entry, sizeof(pifs_entry_t));
            if (ret == PIFS_SUCCESS)
            {
                /* Check if this area is used */
                if (strncmp((char*)a_entry->name, a_name, sizeof(a_entry->name)) == 0)
                {
                    /* Entry found */
                    found = TRUE;
                }
            }
            po += sizeof(pifs_entry_t);
        }
        pa++;
    }

    return ret;
}

P_FILE * pifs_fopen(const char * a_filename, const char *a_modes)
{
    P_FILE        * file = NULL;
    pifs_entry_t    entry;
    bool_t          entry_found = FALSE;

    if (pifs.is_mgmt_found && strlen(a_filename))
    {
        entry_found = pifs_find_entry(a_filename, &entry);
        pifs_page_mark(20, 0, 8, TRUE);
        pifs_page_mark(25, PIFS_FLASH_PAGE_PER_BLOCK - 1, 3, TRUE);
        pifs_page_mark(PIFS_FLASH_BLOCK_NUM_ALL - 1, PIFS_FLASH_PAGE_PER_BLOCK - 1, 1, TRUE);
        /* This should generate an error */
//        pifs_page_mark(PIFS_FLASH_BLOCK_NUM_ALL - 1, PIFS_FLASH_PAGE_PER_BLOCK - 1, 3, TRUE);
    }

    return file;
}

size_t pifs_fwrite(const void * a_data, size_t a_size, size_t a_count, P_FILE *a_file)
{
    size_t written_size = 0;

    if (pifs.is_mgmt_found && a_file)
    {
    }

    return written_size;
}

size_t pifs_fread(void * a_data, size_t a_size, size_t a_count, P_FILE * a_file)
{
    size_t read_size = 0;

    if (pifs.is_mgmt_found && a_file)
    {
    }

    return read_size;
}

int pifs_fclose(P_FILE * a_file)
{
    if (pifs.is_mgmt_found && a_file)
    {
    }

    return 0;
}

