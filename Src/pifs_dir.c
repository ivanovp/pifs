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

#define PIFS_DEBUG_LEVEL 3
#include "pifs_debug.h"

/**
 * @brief pifs_opendir Open directory for listing.
 *
 * @param a_name Pointer to directory name. TODO currently name is omitted!
 * @return Pointer to file system's directory.
 */
pifs_DIR * pifs_opendir(const char *a_name)
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
        pifs_inc_address(&a_dir->entry_list_address);
        a_dir->entry_page_index++;
        if (a_dir->entry_page_index < PIFS_ENTRY_LIST_SIZE_PAGE)
        {
            ret = PIFS_ERROR_NO_MORE_ENTRY;
        }
    }

    return ret;
}

/**
 * @brief pifs_readdir Read one directory entry from opened directory.
 *
 * @param a_dirp Pointer to the opened directory.
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
        if (ret == PIFS_SUCCESS && entry.attrib == PIFS_FLASH_PROGRAMMED_BYTE_VALUE)
        {
            ret = pifs_inc_entry(dir);
        }
    } while (ret == PIFS_SUCCESS && entry.attrib == PIFS_FLASH_PROGRAMMED_BYTE_VALUE);
#endif
    if (ret == PIFS_SUCCESS
            && !pifs_is_buffer_erased(&entry, PIFS_ENTRY_SIZE_BYTE)
            && (entry.attrib != 0))
    {
        /* Copy entry */
        dir->directory_entry.d_ino = dir->entry_list_index; /* FIXME Not unique, better calculation */
        strncpy(dir->directory_entry.d_name, entry.name, sizeof(dir->directory_entry.d_name));
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
 * @param a_dirp Pointer to directory to close.
 * @return 0 if successfully closed, -1 if directory was not opened.
 */
int pifs_closedir(pifs_DIR *a_dirp)
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
