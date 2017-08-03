/**
 * @file        pifs_wear.c
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

#define PIFS_DEBUG_LEVEL 3
#include "pifs_debug.h"

/**
 * @brief pifs_wear_level_list_init Write initial wear level list with all
 * zeros.
 *
 * @return PIFS_SUCCESS if written successfully.
 */
pifs_status_t pifs_wear_level_list_init(void)
{
    pifs_status_t             ret = PIFS_SUCCESS;
    pifs_size_t               i;
    pifs_address_t            address;
    pifs_wear_level_entry_t * wear_level_entry = (pifs_wear_level_entry_t*) pifs.dmw_page_buf;

    memset(pifs.dmw_page_buf, PIFS_FLASH_ERASED_BYTE_VALUE, PIFS_LOGICAL_PAGE_SIZE_BYTE);
    address = pifs.header.wear_level_list_address;
    for (i = 0; i < PIFS_WEAR_LEVEL_ENTRY_PER_PAGE; i++)
    {
        wear_level_entry[i].wear_level_cntr = 0;
        wear_level_entry[i].wear_level_bits = PIFS_FLASH_ERASED_BYTE_VALUE;
    }

    for (i = 0; i < PIFS_WEAR_LEVEL_LIST_SIZE_PAGE && ret == PIFS_SUCCESS; i++)
    {
        //PIFS_WARNING_MSG("%s\r\n", pifs_address2str(&address));
        ret = pifs_write(address.block_address, address.page_address, 0,
                         pifs.dmw_page_buf, PIFS_LOGICAL_PAGE_SIZE_BYTE);
        if (ret == PIFS_SUCCESS)
        {
            (void)pifs_inc_address(&address);
        }
    }

    return ret;
}

/**
 * @brief pifs_get_wear_level Get wear level of a block.
 *
 * @param[in] a_block_address   Address of block.
 * @param[in] a_header          File system's header.
 * @param[out] a_wear_level     Erase count.
 * @return PIFS_SUCCESS if wear level successfully read.
 */
pifs_status_t pifs_get_wear_level(pifs_block_address_t a_block_address,
                                  pifs_header_t * a_header,
                                  pifs_wear_level_entry_t * a_wear_level)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_address_t       address;
    pifs_size_t          po;
    pifs_size_t          i;

//    PIFS_NOTICE_MSG("Wear level list at %s\r\n", pifs_address2str(&a_header->wear_level_list_address));
    address = a_header->wear_level_list_address;
    po = (a_block_address % PIFS_WEAR_LEVEL_ENTRY_PER_PAGE) * PIFS_WEAR_LEVEL_ENTRY_SIZE_BYTE;
//    PIFS_WARNING_MSG("po: %i ba: %i\r\n", po, a_block_address / PIFS_WEAR_LEVEL_ENTRY_PER_PAGE);
    ret = pifs_add_address(&address, a_block_address / PIFS_WEAR_LEVEL_ENTRY_PER_PAGE);
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_read(address.block_address, address.page_address, po,
                        a_wear_level, PIFS_WEAR_LEVEL_ENTRY_SIZE_BYTE);
#if 0
        PIFS_WARNING_MSG("BA%i wear level counter: %i, bits: 0x%02X\r\n",
                         a_block_address,
                         a_wear_level->wear_level_cntr,
                         a_wear_level->wear_level_bits);
#endif
        /* Add wear_level_bits to wear_level_count! */
        for (i = 0; i < sizeof(a_wear_level->wear_level_bits) * PIFS_BYTE_BITS; i++)
        {
#if PIFS_FLASH_ERASED_BYTE_VALUE == 0xFF
            if (!(a_wear_level->wear_level_bits & 1))
#else
            if (a_wear_level->wear_level_bits & 1)
#endif
            {
                a_wear_level->wear_level_cntr++;
            }
            a_wear_level->wear_level_bits >>= 1;
        }
    }

    return ret;
}

/**
 * @brief pifs_inc_wear_level_level Increment wear level of a block.
 *
 * @param[in] a_block_address   Address of block.
 * @param[in] a_header          File system's header.
 * @return PIFS_SUCCESS if wear level successfully incremented.
 */
pifs_status_t pifs_inc_wear_level(pifs_block_address_t a_block_address,
                                  pifs_header_t * a_header)
{
    pifs_status_t           ret = PIFS_SUCCESS;
    pifs_address_t          address;
    pifs_size_t             po;
    pifs_size_t             i;
    pifs_wear_level_entry_t wear_level;

//    PIFS_NOTICE_MSG("Wear level list at %s\r\n", pifs_address2str(&a_header->wear_level_list_address));
    address = a_header->wear_level_list_address;
    po = (a_block_address % PIFS_WEAR_LEVEL_ENTRY_PER_PAGE) * PIFS_WEAR_LEVEL_ENTRY_SIZE_BYTE;
//    PIFS_WARNING_MSG("po: %i ba: %i\r\n", po, a_block_address / PIFS_WEAR_LEVEL_ENTRY_PER_PAGE);
    ret = pifs_add_address(&address, a_block_address / PIFS_WEAR_LEVEL_ENTRY_PER_PAGE);
    if (ret == PIFS_SUCCESS)
    {
        po %= PIFS_LOGICAL_PAGE_SIZE_BYTE;
        ret = pifs_read(address.block_address, address.page_address, po,
                        &wear_level, PIFS_WEAR_LEVEL_ENTRY_SIZE_BYTE);
        if (ret == PIFS_SUCCESS)
        {
            ret = PIFS_ERROR_NO_MORE_SPACE;
            for (i = 0; i < sizeof(wear_level.wear_level_bits) * PIFS_BYTE_BITS && ret != PIFS_SUCCESS; i++)
            {
#if PIFS_FLASH_ERASED_BYTE_VALUE == 0xFF
                if (wear_level.wear_level_bits & (1u << i))
#else
                if (!(wear_level.wear_level_bits & (1u << i)))
#endif
                {
                    ret = PIFS_SUCCESS;
                    wear_level.wear_level_bits ^= 1u << i;
                    PIFS_NOTICE_MSG("BA%i inverting bit %i\r\n", a_block_address, i);
                }
            }
            if (ret == PIFS_SUCCESS)
            {
                ret = pifs_write(address.block_address, address.page_address, po,
                                 &wear_level, PIFS_WEAR_LEVEL_ENTRY_SIZE_BYTE);
            }
        }
    }

    return ret;
}

/**
 * @brief pifs_write_wear_level_level Write wear level of a block.
 *
 * @param[in] a_block_address   Address of block.
 * @param[in] a_header          File system's header.
 * @param[out] a_wear_level     Erase count.
 * @return PIFS_SUCCESS if wear level successfully written.
 */
pifs_status_t pifs_write_wear_level(pifs_block_address_t a_block_address,
                                    pifs_header_t * a_header,
                                    pifs_wear_level_entry_t * a_wear_level)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_address_t       address;
    pifs_size_t          po;

//    PIFS_NOTICE_MSG("Wear level list at %s\r\n", pifs_address2str(&a_header->wear_level_list_address));
    address = a_header->wear_level_list_address;
    po = (a_block_address % PIFS_WEAR_LEVEL_ENTRY_PER_PAGE) * PIFS_WEAR_LEVEL_ENTRY_SIZE_BYTE;
//    PIFS_WARNING_MSG("po: %i ba: %i\r\n", po, a_block_address / PIFS_WEAR_LEVEL_ENTRY_PER_PAGE);
    ret = pifs_add_address(&address, a_block_address / PIFS_WEAR_LEVEL_ENTRY_PER_PAGE);
    if (ret == PIFS_SUCCESS)
    {
        po %= PIFS_LOGICAL_PAGE_SIZE_BYTE;
        PIFS_NOTICE_MSG("BA%i wear level counter: %i, bits: 0x%02X\r\n",
                         a_block_address,
                         a_wear_level->wear_level_cntr,
                         a_wear_level->wear_level_bits);
        ret = pifs_write(address.block_address, address.page_address, po,
                         a_wear_level, PIFS_WEAR_LEVEL_ENTRY_SIZE_BYTE);
    }

    return ret;

}

/**
 * @brief pifs_wear_level_list_copy Copy wear level list.
 *
 * @return PIFS_SUCCESS if copied successfully.
 */
pifs_status_t pifs_copy_wear_level_list(pifs_header_t * a_old_header, pifs_header_t * a_new_header)
{
    pifs_status_t             ret = PIFS_SUCCESS;
    pifs_block_address_t      ba;
    pifs_wear_level_entry_t   wear_level_entry;

    for (ba = PIFS_FLASH_BLOCK_RESERVED_NUM; ba < PIFS_FLASH_BLOCK_NUM_FS && ret == PIFS_SUCCESS; ba++)
    {
        ret = pifs_get_wear_level(ba, a_old_header, &wear_level_entry);
        if (ret == PIFS_SUCCESS)
        {
            wear_level_entry.wear_level_bits = PIFS_FLASH_ERASED_BYTE_VALUE;
            ret = pifs_write_wear_level(ba, a_new_header, &wear_level_entry);
        }
    }

    return ret;
}

/**
 * @brief pifs_get_block_wear_stats Get least weared block.
 *
 * @return PIFS_SUCCESS if get successfully.
 */
pifs_status_t pifs_get_block_wear_stats(pifs_block_type_t a_block_type,
                                        pifs_header_t * a_header,
                                        pifs_block_address_t * a_block_address,
                                        pifs_wear_level_cntr_t * a_wear_level_cntr_min,
                                        pifs_wear_level_cntr_t * a_wear_level_cntr_max)
{
    pifs_status_t             ret = PIFS_SUCCESS;
    pifs_block_address_t      ba;
    pifs_wear_level_entry_t   wear_level_entry;
    pifs_wear_level_cntr_t    wear_level_cntr_min = PIFS_WEAR_LEVEL_CNTR_MAX;
    pifs_wear_level_cntr_t    wear_level_cntr_max = 0;
    pifs_block_address_t      ba_min = PIFS_FLASH_BLOCK_RESERVED_NUM;

    for (ba = PIFS_FLASH_BLOCK_RESERVED_NUM; ba < PIFS_FLASH_BLOCK_NUM_FS && ret == PIFS_SUCCESS; ba++)
    {
        if (pifs_is_block_type(ba, a_block_type, a_header))
        {
            ret = pifs_get_wear_level(ba, a_header, &wear_level_entry);
            if (ret == PIFS_SUCCESS)
            {
                if (wear_level_entry.wear_level_cntr < wear_level_cntr_min)
                {
                    ba_min = ba;
                    wear_level_cntr_min = wear_level_entry.wear_level_cntr;
                }
                if (wear_level_entry.wear_level_cntr > wear_level_cntr_max)
                {
                    wear_level_cntr_max = wear_level_entry.wear_level_cntr;
                }
            }
        }
    }

    if (ret == PIFS_SUCCESS)
    {
//        PIFS_NOTICE_MSG("BA%i\r\n", ba_min);
        *a_block_address = ba_min;
        if (a_wear_level_cntr_min)
        {
            *a_wear_level_cntr_min = wear_level_cntr_min;
        }
        if (a_wear_level_cntr_max)
        {
            *a_wear_level_cntr_max = wear_level_cntr_max;
        }
    }

    return ret;
}

pifs_status_t pifs_generate_least_weared_blocks(pifs_header_t * a_header)
{
    pifs_status_t             ret = PIFS_SUCCESS;
    pifs_block_address_t      ba;
    pifs_wear_level_entry_t   wear_level_entry;
    pifs_wear_level_cntr_t    wear_level_cntr;
    pifs_wear_level_cntr_t    last_wear_level_cntr;
    pifs_size_t               i;
    pifs_size_t               j;
    bool_t                    used = FALSE;

    ret = pifs_get_block_wear_stats(PIFS_BLOCK_TYPE_DATA,
            a_header,
            &(a_header->least_weared_blocks[0].block_address),
            &wear_level_cntr,
            &(a_header->wear_level_cntr_max));

#if PIFS_LEAST_WEARED_BLOCK_NUM > 1
    for (i = 1; i < PIFS_LEAST_WEARED_BLOCK_NUM && ret == PIFS_SUCCESS; i++)
    {
        last_wear_level_cntr = PIFS_WEAR_LEVEL_CNTR_MAX;
        for (ba = PIFS_FLASH_BLOCK_RESERVED_NUM; ba < PIFS_FLASH_BLOCK_NUM_FS
             && ret == PIFS_SUCCESS; ba++)
        {
            if (pifs_is_block_type(ba, PIFS_BLOCK_TYPE_DATA, a_header))
            {
                /* Check if this block has not added already to the list */
                used = FALSE;
                for (j = 0; j < i && !used; j++)
                {
                    if (a_header->least_weared_blocks[j].block_address == ba)
                    {
                        used = TRUE;
                    }
                }
                if (!used)
                {
                    ret = pifs_get_wear_level(ba, a_header, &wear_level_entry);
                    if (ret == PIFS_SUCCESS
                            && wear_level_entry.wear_level_cntr >= wear_level_cntr
                            && wear_level_entry.wear_level_cntr < last_wear_level_cntr )
                    {
                        a_header->least_weared_blocks[i].block_address = ba;
                        last_wear_level_cntr = wear_level_entry.wear_level_cntr;
                    }
                }
            }
        }
        wear_level_cntr = last_wear_level_cntr;
    }
#endif

    return ret;
}

/**
 * @brief pifs_check_block Check if specified block is used by file as data
 * block.
 * This function is used for static wear leveling.
 *
 * @param[in] a_filename        File name to check.
 * @param[in] a_block_address   Block address to look for.
 * @param[out] a_is_block_used  TRUE: block address is used by file.
 * @return PIFS_SUCCESS if file was scanned successfully.
 */
pifs_status_t pifs_check_block(pifs_char_t * a_filename,
                               pifs_block_address_t a_block_address,
                               bool_t * a_is_block_used)
{
    pifs_status_t ret;
    bool_t        is_block_used = FALSE;

    ret = pifs_internal_open(&pifs.internal_file, a_filename, "r", FALSE);
    if (ret == PIFS_SUCCESS)
    {
        do
        {
            if (pifs.internal_file.read_address.block_address == a_block_address)
            {
                is_block_used = TRUE;
            }
            ret = pifs_inc_read_address(&pifs.internal_file);
        } while (ret == PIFS_SUCCESS && !is_block_used);
        if (ret == PIFS_ERROR_END_OF_FILE)
        {
            /* Reaching end of file is not an error */
            ret = PIFS_SUCCESS;
        }
        ret = pifs_fclose(&pifs.internal_file);
    }
    *a_is_block_used = is_block_used;

    return ret;
}

/**
 * @brief pifs_empty_block Create copy of all files which found in
 * the specified block. The older files will be deleted, therefore the
 * specified block can be released.
 * Note: there shall be no free pages in the specified block!
 * This function is used for static wear leveling.
 * TODO copy only pages found in the specified block!
 *
 * @param[in] a_block_address Block address to find.
 * @return PIFS_SUCCESS if block was succuessfuly released.
 */
pifs_status_t pifs_empty_block(pifs_block_address_t a_block_address,
                               bool_t * a_is_emptied)
{
    pifs_status_t   ret = PIFS_ERROR_NO_MORE_RESOURCE;
    pifs_char_t     path[] = { PIFS_PATH_SEPARATOR_CHAR, 0 };
    pifs_DIR      * dir;
    pifs_dirent_t * dirent;
    bool_t          is_block_used;
    pifs_char_t     tmp_filename[PIFS_FILENAME_LEN_MAX];

    *a_is_emptied = FALSE;
    dir = pifs_opendir(path);
    if (dir != NULL)
    {
        ret = PIFS_SUCCESS;
        while ((dirent = pifs_readdir(dir)) && ret == PIFS_SUCCESS)
        {
//            printf("%-32s", dirent->d_name);
//            printf("  %8i", pifs_filesize(dirent->d_name));
//            printf("  %s", pifs_ba_pa2str(dirent->d_first_map_block_address, dirent->d_first_map_page_address));
//            printf("\r\n");
            ret = pifs_check_block(dirent->d_name, a_block_address, &is_block_used);
            if (ret == PIFS_SUCCESS && is_block_used)
            {
                *a_is_emptied = TRUE;
                PIFS_NOTICE_MSG("File '%s' uses block %i\r\n", dirent->d_name, a_block_address);
                strncpy(tmp_filename, dirent->d_name, PIFS_FILENAME_LEN_MAX);
                /* TODO check filename length! */
                /* TODO check if tmp_filename does not exists! */
                strncat(tmp_filename, PIFS_FILENAME_TEMP_STR, PIFS_FILENAME_LEN_MAX);
                PIFS_NOTICE_MSG("Copy '%s' to '%s'...\r\n", dirent->d_name, tmp_filename);
                ret = pifs_copy(dirent->d_name, tmp_filename);
                if (ret == PIFS_SUCCESS)
                {
                    PIFS_NOTICE_MSG("Done\r\n");
                    PIFS_NOTICE_MSG("Rename '%s' to '%s'...\r\n", tmp_filename, dirent->d_name);
                    ret = pifs_rename(tmp_filename, dirent->d_name);
                    if (ret == PIFS_SUCCESS)
                    {
                        PIFS_NOTICE_MSG("Done\r\n");
                    }
                    else
                    {
                        PIFS_ERROR_MSG("Cannot rename '%s' to '%s'!\r\n",
                                       tmp_filename, dirent->d_name);
                    }
                }
                else
                {
                    PIFS_ERROR_MSG("Cannot copy '%s' to '%s'!\r\n",
                                   dirent->d_name, tmp_filename);
                }
            }
        }
        if (pifs_closedir (dir) != 0)
        {
            PIFS_ERROR_MSG("Cannot close directory!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }

    return ret;
}

/**
 * @brief pifs_static_wear_leveling Do static wear leveling by moving files
 * from least weared blocks.
 *
 * @param[in] a_max_block_num Maximum number of blocks to process.
 * @return PIFS_SUCCESS if blocks were processed successfully.
 */
pifs_status_t pifs_static_wear_leveling(pifs_size_t a_max_block_num)
{
    pifs_status_t           ret = PIFS_SUCCESS;
    pifs_size_t             i;
    pifs_size_t             free_data_pages;
    pifs_size_t             free_management_pages;
    pifs_block_address_t    ba;
    bool_t                  is_emptied;
    pifs_wear_level_cntr_t  diff;

    for (i = 0; i < PIFS_LEAST_WEARED_BLOCK_NUM && ret == PIFS_SUCCESS
         && a_max_block_num; i++)
    {
        ba = pifs.header.least_weared_blocks[i].block_address;
        /* TODO this static wear leveling calculation should be revised! */
        diff = pifs.header.wear_level_cntr_max - pifs.header.least_weared_blocks[i].wear_level_cntr;
        ret = pifs_get_pages(TRUE, ba,
                             1, &free_management_pages, &free_data_pages);
        PIFS_NOTICE_MSG("Block %i, free data pages: %i: diff: %i\r\n", ba, free_data_pages, diff);
        if (ret == PIFS_SUCCESS && !free_data_pages
                && diff > PIFS_STATIC_WEAR_LEVEL_LIMIT)
        {
            ret = pifs_empty_block(ba, &is_emptied);
            if (is_emptied)
            {
                a_max_block_num--;
            }
        }
    }

    return ret;
}
