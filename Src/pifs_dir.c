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

    return (pifs_DIR*) dir;
}

/**
 * @brief pifs_find_entry Find entry in entry list.
 *
 * @param a_name[in]    Pointer to name to find.
 * @param a_entry[out]  Pointer to entry to fill. NULL: clear entry.
 * @return PIFS_SUCCESS if entry found.
 * PIFS_ERROR_FILE_NOT_FOUND if entry not found.
 */
struct pifs_dirent * pifs_readdir(pifs_DIR * a_dirp)
{
    pifs_status_t   ret = PIFS_SUCCESS;
    pifs_dir_t    * dir = (pifs_dir_t*) a_dirp;
    pifs_entry_t    entry;
    pifs_dirent_t * dirent = NULL;

    if (dir->entry_list_index >= PIFS_ENTRY_PER_PAGE)
    {
        dir->entry_list_index = 0;
        dir->entry_list_address.page_address++;
    }
    if (dir->entry_list_address.page_address >= PIFS_FLASH_PAGE_PER_BLOCK)
    {
        dir->entry_list_address.page_address = 0;
        dir->entry_list_address.block_address++;
    }
    if (dir->entry_list_address.block_address < pifs.header.entry_list_address.block_address + PIFS_MANAGEMENT_BLOCKS)
    {
        ret = pifs_read_delta(dir->entry_list_address.block_address,
                              dir->entry_list_address.page_address,
                              dir->entry_list_index * PIFS_ENTRY_SIZE_BYTE, &entry,
                              PIFS_ENTRY_SIZE_BYTE);

        if (!pifs_is_buffer_erased(&entry, PIFS_ENTRY_SIZE_BYTE)
                && (entry.attrib != 0))
        {
            /* Copy entry */
            dir->directory_entry.d_ino = dir->entry_list_index; /* FIXME Not unique, better calculation */
            dir->directory_entry.d_off = dir->entry_list_index * PIFS_ENTRY_SIZE_BYTE;
            dir->directory_entry.d_reclen = PIFS_ENTRY_SIZE_BYTE;
            dir->directory_entry.d_type = 0;
            strncpy(dir->directory_entry.d_name, entry.name, sizeof(dir->directory_entry.d_name));
            dirent = &dir->directory_entry;
        }
        dir->entry_list_index++;
    }

    return dirent;
}

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
