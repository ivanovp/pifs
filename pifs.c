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
    .latest_object_id = 1,
    .mgmt_block_address = PIFS_BLOCK_ADDRESS_INVALID,
    .mgmt_page_address = PIFS_PAGE_ADDRESS_INVALID,
    .is_mgmt_found = FALSE,
    .header = { 0 },
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

void pifs_calc_free_space_pos(pifs_block_address_t a_block_address,
                              pifs_page_address_t a_page_address,
                              pifs_block_address_t * a_free_space_block_address,
                              pifs_page_address_t * a_free_space_page_address,
                              uint32_t * a_free_space_bit_pos)
{
    uint32_t bit_pos;

    /* Shift left by one (<< 1) due to two bits are stored in free space bitmap */
    bit_pos = ((a_block_address - PIFS_FLASH_BLOCK_RESERVED_NUM) * PIFS_FLASH_PAGE_PER_BLOCK + a_page_address) << 1;
    PIFS_NOTICE_MSG("BA%i/PA%i bit_pos: %i\r\n", a_block_address, a_page_address, bit_pos);
    *a_free_space_block_address = pifs.header.free_space_bitmap_block_address + bit_pos / PIFS_FLASH_BLOCK_SIZE_BYTE;
    bit_pos %= PIFS_FLASH_BLOCK_SIZE_BYTE;
    *a_free_space_page_address = pifs.header.free_space_bitmap_page_address + bit_pos / PIFS_FLASH_PAGE_SIZE_BYTE;
    bit_pos %= PIFS_FLASH_PAGE_SIZE_BYTE;
    *a_free_space_bit_pos = bit_pos;
}

pifs_status_t pifs_page_mark(pifs_block_address_t a_block_address,
                                    pifs_page_address_t a_page_address,
                                    size_t a_page_count, bool_t mark_used)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    uint32_t             bit_pos;
    pifs_block_address_t ba;
    pifs_page_address_t  pa;
    pifs_block_address_t prev_ba = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_page_address_t  prev_pa = PIFS_PAGE_ADDRESS_INVALID;
    size_t               i;

    while (a_page_count > 0 && ret == PIFS_SUCCESS)
    {
        pifs_calc_free_space_pos(a_block_address, a_page_address, &ba, &pa, &bit_pos);
        /* Read actual status */
        ret = pifs_flash_read(ba, pa, 0, pifs.page_buf, sizeof( pifs.page_buf ) );
        if (ret == PIFS_SUCCESS)
        {
            print_buffer(pifs.page_buf, sizeof(pifs.page_buf), 0);
            PIFS_NOTICE_MSG("-Free space byte:    0x%02X\r\n", pifs.page_buf[bit_pos / 8]);
            PIFS_NOTICE_MSG("-Free space bit:     %i\r\n", (pifs.page_buf[bit_pos / 8] >> (bit_pos % 8)) & 1 );
            PIFS_NOTICE_MSG("-Release space bit:  %i\r\n", (pifs.page_buf[bit_pos / 8] >> ((bit_pos % 8) + 1)) & 1);
            if (mark_used)
            {
                /* Clear free bit */
                pifs.page_buf[bit_pos / 8] &= ~(1u << (bit_pos % 8));
            }
            else
            {
                /* Clear release bit */
                pifs.page_buf[bit_pos / 8] &= ~(1u << ((bit_pos % 8) + 1));
            }
            PIFS_NOTICE_MSG("+Free space byte:    0x%02X\r\n", pifs.page_buf[bit_pos / 8]);
            PIFS_NOTICE_MSG("+Free space bit:     %i\r\n", (pifs.page_buf[bit_pos / 8] >> (bit_pos % 8)) & 1 );
            PIFS_NOTICE_MSG("+Release space bit:  %i\r\n", (pifs.page_buf[bit_pos / 8] >> ((bit_pos % 8) + 1)) & 1);
            /* Write new status */
            ret = pifs_flash_write(ba, pa, 0, pifs.page_buf, sizeof( pifs.page_buf ) );
        }
        a_page_count--;
        if (a_page_count > 0)
        {
            a_page_address++;
            if (a_page_address == PIFS_FLASH_PAGE_PER_BLOCK)
            {
                a_page_address = 0;
                a_block_address++;
                if (a_block_address == PIFS_FLASH_BLOCK_NUM)
                {
                    PIFS_ERROR_MSG("Trying to mark invalid address! BA%i/PA%i\r\n", a_block_address, a_page_address);
                }
            }
        }
    }

    return ret;
}

pifs_status_t pifs_page_find_free(pifs_block_address_t * a_block_address,
                                    pifs_page_address_t * a_page_address,
                                    size_t a_page_count)
{
    pifs_status_t ret = PIFS_ERROR;

    return ret;
}

pifs_status_t pifs_init(void)
{
    pifs_status_t        ret = PIFS_FLASH_INIT_ERROR;
    pifs_block_address_t ba;
    pifs_page_address_t  pa;
    pifs_header_t        prev_header;
    pifs_checksum_t      checksum;

    pifs.is_mgmt_found = FALSE;

    PIFS_NOTICE_MSG("Block address size:     %lu bytes\r\n", sizeof(pifs_block_address_t));
    PIFS_NOTICE_MSG("Page address size:      %lu bytes\r\n", sizeof(pifs_page_address_t));
    PIFS_NOTICE_MSG("Object ID size:         %lu bytes\r\n", sizeof(pifs_object_id_t));
    PIFS_NOTICE_MSG("Header size:            %lu bytes\r\n", sizeof(pifs_header_t));
    PIFS_NOTICE_MSG("Entry size:             %lu bytes\r\n", sizeof(pifs_entry_t));
    PIFS_NOTICE_MSG("Header+entry list size: %lu bytes, %lu pages\r\n", PIFS_HEADER_ENTRY_LIST_SIZE_BYTE, PIFS_HEADER_ENTRY_LIST_SIZE_PAGE);
    PIFS_NOTICE_MSG("Free space bitmap size: %lu bytes\r\n", sizeof(pifs_free_space_bitmap_t));
    PIFS_NOTICE_MSG("Maximum number of management pages: %i\r\n",
            (PIFS_FLASH_BLOCK_NUM - PIFS_FLASH_BLOCK_RESERVED_NUM) * PIFS_MANAGEMENT_PAGES_MAX);

    ret = pifs_flash_init();

    if (ret == PIFS_SUCCESS)
    {
        /* Find latest management block */
        for (ba = PIFS_FLASH_BLOCK_RESERVED_NUM && !pifs.is_mgmt_found; ba < PIFS_FLASH_BLOCK_NUM; ba++)
        {
            for (pa = 0; pa < PIFS_MANAGEMENT_PAGES_MAX && !pifs.is_mgmt_found; pa++)
            {
                pifs_flash_read(ba, pa, 0, &pifs.header, sizeof(pifs.header));
                if (pifs.header.magic == PIFS_MAGIC
        #if ENABLE_PIFS_VERSION
                        && pifs.header.majorVersion == PIFS_MAJOR_VERSION
                        && pifs.header.minorVersion == PIFS_MINOR_VERSION
        #endif
                        )
                {
                    PIFS_NOTICE_MSG("Management page found: BA%i/PA%i\r\n", ba, pa);
                    checksum = pifs_calc_header_checksum(&pifs.header);
                    if (checksum == pifs.header.checksum)
                    {
                        PIFS_NOTICE_MSG("Checksum is valid\r\n");
                        if (!pifs.is_mgmt_found || prev_header.counter < pifs.header.counter)
                        {
                            pifs.is_mgmt_found = TRUE;
                            pifs.mgmt_block_address = ba;
                            pifs.mgmt_page_address = pa;
                            memcpy(&prev_header, &pifs.header, sizeof(prev_header));
                        }
                    }
                    else
                    {
                        PIFS_ERROR_MSG("Checksum is invalid! Calculated: 0x%02X, read: 0x%02X\r\n",
                                       checksum, pifs.header.checksum);
                    }
                }
            }
        }

        if (!pifs.is_mgmt_found)
        {
            /* FIXME use random block? */
            ba = PIFS_FLASH_BLOCK_RESERVED_NUM;
            pa = 0;
            PIFS_NOTICE_MSG("Creating managamenet block BA%i/PA%i\r\n", ba, pa);
            pifs.header.magic = PIFS_MAGIC;
#if ENABLE_PIFS_VERSION
            pifs.header.majorVersion = PIFS_MAJOR_VERSION;
            pifs.header.minorVersion = PIFS_MINOR_VERSION;
#endif
            pifs.header.counter = 1;
            pifs.header.block_type = PIFS_BLOCK_TYPE_MANAGEMENT_BLOCK;
            pifs.header.free_space_bitmap_block_address = ba;
            pifs.header.free_space_bitmap_page_address = pa + PIFS_HEADER_ENTRY_LIST_SIZE_PAGE;
            pifs.header.checksum = pifs_calc_header_checksum(&pifs.header);

            ret = pifs_flash_erase(ba);
            if (ret == PIFS_SUCCESS)
            {
                ret = pifs_flash_write(ba, pa, 0, &pifs.header, sizeof(pifs.header));
                if (ret == PIFS_SUCCESS)
                {
                    pifs.is_mgmt_found = TRUE;
                    pifs.mgmt_block_address = ba;
                    pifs.mgmt_page_address = pa;
                }
            }
        }

        if (pifs.is_mgmt_found)
        {
            print_buffer(&pifs.header, sizeof(pifs.header), 0);
            PIFS_NOTICE_MSG("Counter: %i\r\n",
                            pifs.header.counter);
            PIFS_NOTICE_MSG("Free space bitmap BA%i/PA%i\r\n",
                            pifs.header.free_space_bitmap_block_address,
                            pifs.header.free_space_bitmap_page_address);
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
    pifs_block_address_t ba = pifs.mgmt_block_address;
    pifs_page_address_t  pa = pifs.mgmt_page_address;
    pifs_page_offset_t   po;
    pifs_entry_t         entry;
    bool_t               created = FALSE;

    while (pa < PIFS_FLASH_PAGE_PER_BLOCK && !created)
    {
        /* Skip file system's header */
        po = sizeof(pifs_header_t);
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
    pifs_block_address_t ba = pifs.mgmt_block_address;
    pifs_page_address_t  pa = pifs.mgmt_page_address;
    pifs_page_offset_t   po;
    bool_t               found = FALSE;

    while (pa < PIFS_FLASH_PAGE_PER_BLOCK && !found)
    {
        /* Skip file system's header */
        po = sizeof(pifs_header_t);
        while (po < PIFS_FLASH_PAGE_SIZE_BYTE && !found)
        {
            ret = pifs_flash_read(ba, pa, po, a_entry, sizeof(pifs_entry_t));
            if (ret == PIFS_SUCCESS)
            {
                /* Check if this area is used */
                if (strncmp(a_entry->name, a_name, sizeof(a_entry->name)) == 0)
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

    if (pifs.is_mgmt_found && strlen (a_filename))
    {
        entry_found = pifs_find_entry(a_filename, &entry);
        pifs_page_mark(0, 0, 8, TRUE);
        pifs_page_mark(5, PIFS_FLASH_PAGE_PER_BLOCK - 1, 3, TRUE);
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

