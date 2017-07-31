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
#include "buffer.h" /* DEBUG */

#define PIFS_DEBUG_LEVEL 2
#include "pifs_debug.h"

/**
 * @brief pifs_opendir Open directory for listing.
 *
 * @param[in] a_name Pointer to directory name. TODO currently name is omitted!
 * @return Pointer to file system's directory.
 */
pifs_DIR * pifs_opendir(const pifs_char_t * a_name)
{
    pifs_size_t  i;
    pifs_dir_t * dir = NULL;

    /* TODO only root directory is supported currently */
    (void) a_name;

    for (i = 0; i < PIFS_OPEN_DIR_NUM_MAX; i++)
    {
        dir = &pifs.dir[i];
        if (!dir->is_used)
        {
            dir->is_used = TRUE;
            dir->entry_page_index = 0;
            dir->entry_list_address = pifs.header.entry_list_address;
            dir->entry_list_index = 0;
        }
    }
    if (dir == NULL)
    {
        PIFS_SET_ERRNO(PIFS_ERROR_NO_MORE_RESOURCE);
    }

    return (pifs_DIR*) dir;
}

/**
 * @brief pifs_inc_entry Increase directory entry's pointer.
 *
 * @param[in] a_dir Pointer to directory structure.
 * @return
 */
static pifs_status_t pifs_inc_entry(pifs_dir_t * a_dir)
{
    pifs_status_t ret = PIFS_SUCCESS;

    a_dir->entry_list_index++;
    if (a_dir->entry_list_index >= PIFS_ENTRY_PER_PAGE)
    {
        a_dir->entry_list_index = 0;
        (void)pifs_inc_address(&a_dir->entry_list_address);
        a_dir->entry_page_index++;
        if (a_dir->entry_page_index >= PIFS_ENTRY_LIST_SIZE_PAGE)
        {
            ret = PIFS_ERROR_NO_MORE_ENTRY;
        }
    }

    return ret;
}

/**
 * @brief pifs_readdir Read one directory entry from opened directory.
 *
 * @param[in] a_dirp Pointer to the opened directory.
 * @return Entry if found or NULL.
 */
struct pifs_dirent * pifs_readdir(pifs_DIR * a_dirp)
{
    pifs_status_t   ret = PIFS_SUCCESS;
    pifs_dir_t    * dir = (pifs_dir_t*) a_dirp;
    pifs_entry_t    entry;
    pifs_dirent_t * dirent = NULL;

#if PIFS_USE_DELTA_FOR_ENTRIES
    ret = pifs_read_delta(dir->entry_list_address.block_address,
                          dir->entry_list_address.page_address,
                          dir->entry_list_index * PIFS_ENTRY_SIZE_BYTE, &entry,
                          PIFS_ENTRY_SIZE_BYTE);
#else
    do
    {
        ret = pifs_read(dir->entry_list_address.block_address,
                        dir->entry_list_address.page_address,
                        dir->entry_list_index * PIFS_ENTRY_SIZE_BYTE, &entry,
                        PIFS_ENTRY_SIZE_BYTE);
        if (ret == PIFS_SUCCESS && entry.name[0] == PIFS_FLASH_PROGRAMMED_BYTE_VALUE)
        {
            ret = pifs_inc_entry(dir);
        }
    } while (ret == PIFS_SUCCESS && entry.name[0] == PIFS_FLASH_PROGRAMMED_BYTE_VALUE);
#endif
    if (ret == PIFS_SUCCESS
            && !pifs_is_buffer_erased(&entry, PIFS_ENTRY_SIZE_BYTE)
            && (entry.name[0] != PIFS_FLASH_PROGRAMMED_BYTE_VALUE))
    {
        /* Copy entry */
        dir->directory_entry.d_ino = entry.first_map_address.block_address * PIFS_FLASH_BLOCK_SIZE_BYTE
                + entry.first_map_address.page_address * PIFS_LOGICAL_PAGE_SIZE_BYTE;
        dir->directory_entry.d_first_map_block_address = entry.first_map_address.block_address;
        dir->directory_entry.d_first_map_page_address = entry.first_map_address.page_address;
        strncpy(dir->directory_entry.d_name, entry.name, sizeof(dir->directory_entry.d_name));
#if PIFS_ENABLE_USER_DATA
        memcpy(&dir->directory_entry.d_user_data, &entry.user_data, sizeof(dir->directory_entry.d_user_data));
#endif
        dirent = &dir->directory_entry;
    }
    if (ret == PIFS_SUCCESS)
    {
        pifs_inc_entry(dir);
    }
    PIFS_SET_ERRNO(ret);

    return dirent;
}

/**
 * @brief pifs_closedir Close opened directory.
 *
 * @param[in] a_dirp Pointer to directory to close.
 * @return 0 if successfully closed, -1 if directory was not opened.
 */
int pifs_closedir(pifs_DIR * a_dirp)
{
    int           ret = -1;
    pifs_dir_t  * dir = (pifs_dir_t*) a_dirp;

    if (dir->is_used)
    {
        dir->is_used = FALSE;
        ret = 0;
    }

    return ret;
}

#if PIFS_ENABLE_DIRECTORIES
int pifs_mkdir(const pifs_char_t * a_filename)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_entry_t       * entry = &pifs.entry;
    pifs_block_address_t ba;
    pifs_page_address_t  pa;
    pifs_page_count_t    page_count_found;

    ret = pifs_find_entry(a_filename, entry);
    if (ret == PIFS_SUCCESS)
    {
        ret = PIFS_ERROR_FILE_ALREADY_EXIST;
    }
    else
    {
        /* Order of steps to create a directory: */
        /* #1 Find a free page for entry list */
        /* #2 Create entry of a_file, which contains the entry list's address */
        /* #3 Mark entry list page */
        if (ret == PIFS_SUCCESS)
        {
            ret = pifs_find_free_page_wl(PIFS_ENTRY_LIST_SIZE_PAGE, PIFS_ENTRY_LIST_SIZE_PAGE,
                                         PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT,
                                         &ba, &pa, &page_count_found);
        }
        if (ret == PIFS_SUCCESS)
        {
            PIFS_DEBUG_MSG("Entry list: %u free page found %s\r\n", page_count_found, pifs_ba_pa2str(ba, pa));
            memset(entry, PIFS_FLASH_ERASED_BYTE_VALUE, PIFS_ENTRY_SIZE_BYTE);
            strncpy((char*)entry->name, a_filename, PIFS_FILENAME_LEN_MAX);
            entry->attrib = PIFS_ATTRIB_ARCHIVE | PIFS_ATTRIB_DIR;
            entry->first_map_address.block_address = ba;
            entry->first_map_address.page_address = pa;
            ret = pifs_append_entry(entry);
            if (ret == PIFS_SUCCESS)
            {
                PIFS_DEBUG_MSG("Entry created\r\n");
                ret = pifs_mark_page(ba, pa, PIFS_ENTRY_LIST_SIZE_PAGE, TRUE);
            }
            else
            {
                PIFS_DEBUG_MSG("Cannot create entry!\r\n");
                PIFS_SET_ERRNO(PIFS_ERROR_NO_MORE_ENTRY);
            }
        }
    }

    return ret;
}

int pifs_rmdir(const pifs_char_t * a_filename)
{

}

int pifs_chdir(const pifs_char_t * a_filename)
{

}

pifs_char_t * pifs_getcwd(pifs_char_t * buffer, size_t a_size)
{

}
#endif
