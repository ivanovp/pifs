/**
 * @file        pifs.c
 * @brief       Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-11-30 17:42:49 ivanovp {Time-stamp}
 * Licence:     GPL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PIFS_DEBUG_LEVEL 2
#include "pifs_debug.h"

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
#include "pifs_dir.h"
#include "buffer.h" /* DEBUG */

#if PIFS_ENABLE_DIRECTORIES
/**
 * @brief pifs_delete_chars Delete character in a string.
 *
 * @param[out] a_string String to be cutted.
 * @param[in] a_idx     Start index of to be deleted characters.
 * @param[in] a_count   Number of characters to delete.
 */
void pifs_delete_chars(pifs_char_t * a_string, pifs_size_t a_idx, pifs_size_t a_count)
{
    pifs_size_t i;
    pifs_size_t len;

    //PIFS_DEBUG_MSG("before %s, idx: %i, count: %i\r\n", a_string, a_idx, a_count);
    len = strlen(a_string) - a_count + 1;
    for (i = a_idx; i < len; i++)
    {
        a_string[i] = a_string[i + a_count];
    }
    //PIFS_DEBUG_MSG("after %s\r\n", a_string);
}

/**
 * @brief pifs_normalize_path Change "/aaa/bbb/ccc/../.." to "/aaa".
 * Used when directory changed.
 *
 * @param[out] a_path Path to be normalized.
 */
void pifs_normalize_path(pifs_char_t * const a_path)
{
    typedef enum
    {
        NORM_start = 0,
        NORM_separator,
        NORM_dot,
        NORM_dot2,
        NORM_other,
    } norm_state_t;
    norm_state_t norm_state;
    size_t       i;
    size_t       separator_pos[2];
    size_t       path_len;
    bool_t       is_deleted;
    bool_t       end;

    do
    {
        //PIFS_DEBUG_MSG("start %s\r\n", a_path);
        separator_pos[1] = 0; /* Befor last separator's position */
        separator_pos[0] = 0; /* Last separator's position */
        is_deleted = FALSE;
        end = FALSE;
        path_len = strlen(a_path);
        norm_state = NORM_start;
        for (i = 0; i < path_len && !end; i++)
        {
            if (a_path[i] != PIFS_DOT_CHAR && a_path[i] != PIFS_PATH_SEPARATOR_CHAR)
            {
                norm_state = NORM_other;
            }
            if (a_path[i] == PIFS_DOT_CHAR)
            {
                //PIFS_DEBUG_MSG(".state: %i\r\n", norm_state);
                if (norm_state == NORM_separator)
                {
                    norm_state = NORM_dot;
                }
                else if (norm_state == NORM_dot)
                {
                    norm_state = NORM_dot2;
                }
            }
            if (a_path[i] == PIFS_PATH_SEPARATOR_CHAR
                     || i == (path_len - 1))
            {
                //PIFS_DEBUG_MSG("/state: %i\r\n", norm_state);
                //PIFS_DEBUG_MSG("separator_pos[1]: %i\r\n", separator_pos[1]);
                //PIFS_DEBUG_MSG("separator_pos[0]: %i\r\n", separator_pos[0]);
                if (i == (path_len - 1))
                {
                    i++;
                }
                if (norm_state == NORM_dot)
                {
                    pifs_delete_chars(a_path, i - 2, 2);
                    is_deleted = TRUE;
                    end = TRUE;
                }
                else if (norm_state == NORM_dot2)
                {
                    pifs_delete_chars(a_path, separator_pos[1], i - separator_pos[1]);
                    is_deleted = TRUE;
                    end = TRUE;
                }
                norm_state = NORM_separator;
                separator_pos[1] = separator_pos[0];
                separator_pos[0] = i;
            }
        }
        //PIFS_DEBUG_MSG("end %s\r\n", a_path);
    } while (is_deleted);
}

/**
 * @brief pifs_resolve_dir Walk through directories and find last directory's
 * entry.
 * Example: a_path = "/aaa/bbb/ccc/ddd". After running
 * a_resolved_entry_list_address will be address of "ddd".
 *
 * @param[in] a_path                         Path to be walked.
 * @param[in] a_current_entry_list_address   Current entry list's address.
 * @param[out] a_resolved_entry_list_address Last directory's entry list.
 * @return PIFS_SUCCESS if entry list's address is resolved.
 */
pifs_status_t pifs_resolve_dir(pifs_char_t * const a_path,
                                pifs_address_t a_current_entry_list_address,
                                pifs_address_t * const a_resolved_entry_list_address)
{
    pifs_status_t       ret = PIFS_SUCCESS;
    pifs_char_t       * curr_path_pos = a_path;
    pifs_char_t       * curr_separator_pos = NULL;
    pifs_address_t      entry_list_address = a_current_entry_list_address;
    pifs_char_t         name[PIFS_FILENAME_LEN_MAX];
    pifs_entry_t      * entry = &pifs.entry;
    pifs_size_t         len;
    bool_t              end = FALSE;

    PIFS_DEBUG_MSG("path: [%s]\r\n", a_path);
    /* Check if it is an absolute path */
    if (a_path[0] == PIFS_PATH_SEPARATOR_CHAR)
    {
        /* Absolute path, start from root entry list */
        curr_path_pos++;
        entry_list_address = pifs.header.root_entry_list_address;
    }
    do
    {
        curr_separator_pos = strchr(curr_path_pos, PIFS_PATH_SEPARATOR_CHAR);
        if (!curr_separator_pos)
        {
            curr_separator_pos = curr_path_pos + strlen(curr_path_pos);
            end = TRUE;
        }
        len = curr_separator_pos - curr_path_pos;
        memcpy(name, curr_path_pos, len);
        name[len] = PIFS_EOS;
        PIFS_DEBUG_MSG("name: [%s]\r\n", name);
        ret = pifs_find_entry(PIFS_FIND_ENTRY, name, entry,
                              entry_list_address.block_address,
                              entry_list_address.page_address);
        if (ret == PIFS_SUCCESS)
        {
            if (PIFS_IS_DIR(entry->attrib))
            {
                entry_list_address = entry->first_map_address;
            }
            else
            {
                PIFS_ERROR_MSG("'%s' is not directory!\r\n", entry->name);
                ret = PIFS_ERROR_IS_NOT_DIRECTORY;
            }
        }
        curr_path_pos = curr_separator_pos + 1;
    } while (ret == PIFS_SUCCESS && !end);
    if (ret == PIFS_SUCCESS)
    {
        *a_resolved_entry_list_address = entry_list_address;
    }
    PIFS_INFO_MSG("entry list address: %s\r\n",
                   pifs_address2str(a_resolved_entry_list_address));

    return ret;
}

/**
 * @brief pifs_resolve_path Walk through directories and find last directory's
 * entry.
 * Example: a_path = "/aaa/bbb/ccc/ddd/name.txt". After running a_filename
 * will be "name.txt", a_resolved_entry_list_address will be address of "ddd".
 *
 * @param[in] a_path                         Path to be walked.
 * @param[in] a_current_entry_list_address   Current entry list's address.
 * @param[out] a_filename                    Last element of path, which can be file name
 *                                           or directory as well.
 * @param[out] a_resolved_entry_list_address Last directory's entry list.
 * @return PIFS_SUCCESS if entry list's address is resolved.
 */
pifs_status_t pifs_resolve_path(const pifs_char_t * a_path,
                                pifs_address_t a_current_entry_list_address,
                                pifs_char_t * const a_filename,
                                pifs_address_t * const a_resolved_entry_list_address)
{
    pifs_status_t       ret = PIFS_SUCCESS;
    const pifs_char_t * curr_path_pos = a_path;
    pifs_char_t       * curr_separator_pos = NULL;
    pifs_address_t      entry_list_address = a_current_entry_list_address;
    pifs_char_t         name[PIFS_FILENAME_LEN_MAX];
    pifs_entry_t      * entry = &pifs.entry;
    pifs_size_t         len;

    PIFS_DEBUG_MSG("path: [%s]\r\n", a_path);
    /* Check if it is an absolute path */
    if (a_path[0] == PIFS_PATH_SEPARATOR_CHAR)
    {
        /* Absolute path, start from root entry list */
        curr_path_pos++;
        entry_list_address = pifs.header.root_entry_list_address;
    }
    while ((curr_separator_pos = strchr(curr_path_pos, PIFS_PATH_SEPARATOR_CHAR))
           && ret == PIFS_SUCCESS)
    {
        len = curr_separator_pos - curr_path_pos;
        memcpy(name, curr_path_pos, len);
        name[len] = PIFS_EOS;
        PIFS_DEBUG_MSG("name: [%s]\r\n", name);
        ret = pifs_find_entry(PIFS_FIND_ENTRY, name, entry,
                              entry_list_address.block_address,
                              entry_list_address.page_address);
        if (ret == PIFS_SUCCESS)
        {
            if (PIFS_IS_DIR(entry->attrib))
            {
                entry_list_address = entry->first_map_address;
            }
            else
            {
                PIFS_ERROR_MSG("'%s' is not directory!\r\n", entry->name);
                ret = PIFS_ERROR_IS_NOT_DIRECTORY;
            }
        }
        curr_path_pos = curr_separator_pos + 1;
    }
    if (curr_path_pos == a_path)
    {
        strncpy(a_filename, a_path, PIFS_FILENAME_LEN_MAX);
    }
    else
    {
        strncpy(a_filename, curr_path_pos, PIFS_FILENAME_LEN_MAX);
    }
    if (ret == PIFS_SUCCESS)
    {
        *a_resolved_entry_list_address = entry_list_address;
    }
    PIFS_INFO_MSG("a_filename: [%s] entry list address: %s\r\n",
                   a_filename, pifs_address2str(a_resolved_entry_list_address));

    return ret;
}

/**
 * @brief pifs_is_directory_empty Check whether a directory is empty.
 * Directory is empty if only "." and ".." entries exist.
 *
 * @param[in] a_path Path to directory to check.
 * @return TRUE: directory is empty. FALSE: directory is not empty.
 */
bool_t pifs_is_directory_empty(const pifs_char_t * a_path)
{
    bool_t          empty = TRUE;
    pifs_dir_t    * dir;
    pifs_dirent_t * dirent;

    dir = pifs_internal_opendir(a_path);
    if (dir != NULL)
    {
        while ((dirent = pifs_internal_readdir(dir)))
        {
            if (!PIFS_IS_DOT_DIR(dirent->d_name))
            {
                empty = FALSE;
            }
        }
        if (pifs_internal_closedir (dir) != 0)
        {
            PIFS_ERROR_MSG("Cannot close directory!\r\n");
        }
    }

    PIFS_DEBUG_MSG("%s empty: %s\r\n", a_path, pifs_yes_no(empty));
    return empty;
}

/**
 * @brief pifs_get_task_cwd Get current task's working directory.
 * Note: caller shall provide mutex protection!
 *
 * @return Pointer to task's current working directory.
 */
pifs_char_t * pifs_get_task_cwd(void)
{
    PIFS_OS_TASK_ID_TYPE task_id;
    pifs_char_t        * cwd = pifs.cwd[0];

    task_id = PIFS_OS_GET_TASK_ID();

    if (task_id < PIFS_TASK_COUNT_MAX)
    {
        cwd = pifs.cwd[task_id];
    }

    return cwd;
}

/**
 * @brief pifs_get_task_current_entry_list_address Get current task's working directory.
 * Note: caller shall provide mutex protection!
 *
 * @return Pointer to task's current working directory.
 */
pifs_address_t * pifs_get_task_current_entry_list_address(void)
{
    PIFS_OS_TASK_ID_TYPE task_id;
    pifs_address_t     * current_entry_list_address = &pifs.current_entry_list_address[0];

    task_id = PIFS_OS_GET_TASK_ID();

    if (task_id < PIFS_TASK_COUNT_MAX)
    {
        current_entry_list_address = &pifs.current_entry_list_address[task_id];
    }
    else
    {
        task_id = 0;
    }

    if (current_entry_list_address->block_address == PIFS_BLOCK_ADDRESS_INVALID
            || current_entry_list_address->page_address == PIFS_PAGE_ADDRESS_INVALID)
    {
        pifs_resolve_dir(pifs.cwd[task_id], pifs.header.root_entry_list_address,
                         current_entry_list_address);
    }

    return current_entry_list_address;
}
#endif

/**
 * @brief pifs_opendir Open directory for listing.
 *
 * @param[in] a_name Pointer to directory name.
 * @return Pointer to file system's directory.
 */
pifs_DIR * pifs_opendir(const pifs_char_t * a_name)
{
    pifs_DIR dir;

    PIFS_GET_MUTEX();

    dir = pifs_internal_opendir(a_name);

    PIFS_PUT_MUTEX();

    return dir;
}

/**
 * @brief pifs_internal_opendir Open directory for listing.
 *
 * @param[in] a_name Pointer to directory name. 
 * @return Pointer to file system's directory.
 */
pifs_dir_t * pifs_internal_opendir(const pifs_char_t * a_name)
{
    pifs_status_t   ret = PIFS_SUCCESS;
    pifs_size_t     i;
    pifs_dir_t    * dir = NULL;
#if PIFS_ENABLE_DIRECTORIES
    pifs_address_t  entry_list_address = *pifs_get_task_current_entry_list_address();
    pifs_char_t     filename[PIFS_FILENAME_LEN_MAX];
    pifs_entry_t  * entry = &pifs.entry;
#endif

#if PIFS_ENABLE_DIRECTORIES
    /* Resolve a_filename's relative/absolute file path and update
     * entry_list_address regarding that */
    if (!a_name || (a_name[0] == PIFS_PATH_SEPARATOR_CHAR
                    && a_name[1] == PIFS_EOS))
    {
        /* Root directory: "/" or "\" */
        entry_list_address = pifs.header.root_entry_list_address;
    }
    else if (a_name[0] == PIFS_DOT_CHAR
            && a_name[1] == PIFS_EOS)
    {
        /* Current directory: "." */
        entry_list_address = *pifs_get_task_current_entry_list_address();
    }
    else
    {
        ret = pifs_resolve_path(a_name, entry_list_address,
                                filename, &entry_list_address);
        if (ret == PIFS_SUCCESS)
        {
            ret = pifs_find_entry(PIFS_FIND_ENTRY, filename, entry,
                                  entry_list_address.block_address,
                                  entry_list_address.page_address);
        }
        if (ret == PIFS_SUCCESS)
        {
            entry_list_address = entry->first_map_address;
        }
    }
#else
    (void) a_name;
#endif

    if (ret == PIFS_SUCCESS)
    {
        for (i = 0; i < PIFS_OPEN_DIR_NUM_MAX; i++)
        {
            dir = &pifs.dir[i];
            if (!dir->is_used)
            {
                dir->is_used = TRUE;
                dir->entry_page_index = 0;
#if PIFS_ENABLE_DIRECTORIES
                dir->entry_list_address = entry_list_address;
#else
                dir->entry_list_address = pifs.header.root_entry_list_address;
#endif
                PIFS_DEBUG_MSG("Opening directory at %s\r\n",
                                 pifs_address2str(&dir->entry_list_address));
                dir->entry_list_index = 0;
            }
        }
        if (dir == NULL)
        {
            PIFS_ERROR_MSG("No more directory structure!\r\n");
            PIFS_SET_ERRNO(PIFS_ERROR_NO_MORE_RESOURCE);
        }
    }
    else
    {
        PIFS_ERROR_MSG("Cannot open directory: %i\r\n", ret);
        PIFS_SET_ERRNO(ret);
    }

    return dir;
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
    struct pifs_dirent * dirent;

    PIFS_GET_MUTEX();

    dirent = (struct pifs_dirent*) pifs_internal_readdir((pifs_dir_t*) a_dirp);

    PIFS_PUT_MUTEX();

    return dirent;
}

/**
 * @brief pifs_internal_readdir Read one directory entry from opened directory.
 *
 * @param[in] a_dirp Pointer to the opened directory.
 * @return Entry if found or NULL.
 */
pifs_dirent_t * pifs_internal_readdir(pifs_dir_t * a_dirp)
{
    pifs_status_t   ret = PIFS_SUCCESS;
    pifs_dir_t    * dir = (pifs_dir_t*) a_dirp;
    pifs_entry_t  * entry = &dir->entry;
    pifs_dirent_t * dirent = NULL;
    bool_t          entry_found = FALSE;

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
                        dir->entry_list_index * PIFS_ENTRY_SIZE_BYTE, entry,
                        PIFS_ENTRY_SIZE_BYTE);
        if (ret == PIFS_SUCCESS)
        {
            if (!pifs_is_entry_deleted(entry))
            {
                entry_found = TRUE;
            }
            else
            {
                ret = pifs_inc_entry(dir);
            }
        }
    } while (ret == PIFS_SUCCESS && !entry_found);
#endif
    if (ret == PIFS_SUCCESS
            && !pifs_is_buffer_erased(entry, PIFS_ENTRY_SIZE_BYTE)
            && entry_found)
    {
        /* Copy entry */
        dir->directory_entry.d_ino = entry->first_map_address.block_address * PIFS_FLASH_BLOCK_SIZE_BYTE
                + entry->first_map_address.page_address * PIFS_LOGICAL_PAGE_SIZE_BYTE;
        strncpy(dir->directory_entry.d_name, entry->name, sizeof(dir->directory_entry.d_name));
        dir->directory_entry.d_filesize = entry->file_size;
#if PIFS_ENABLE_ATTRIBUTES
        dir->directory_entry.d_attrib = entry->attrib;
#endif
        dir->directory_entry.d_first_map_block_address = entry->first_map_address.block_address;
        dir->directory_entry.d_first_map_page_address = entry->first_map_address.page_address;
#if PIFS_ENABLE_USER_DATA
        memcpy(&dir->directory_entry.d_user_data, &entry->user_data, sizeof(dir->directory_entry.d_user_data));
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
int pifs_closedir(pifs_DIR * const a_dirp)
{
    int ret;

    PIFS_GET_MUTEX();

    ret = pifs_internal_closedir((pifs_dir_t*) a_dirp);

    PIFS_PUT_MUTEX();

    return ret;
}

/**
 * @brief pifs_internal_closedir Close opened directory.
 *
 * @param[in] a_dirp Pointer to directory to close.
 * @return 0 if successfully closed, -1 if directory was not opened.
 */
int pifs_internal_closedir(pifs_dir_t * const a_dirp)
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

/**
 * @brief pifs_walk_dir Walk through directory.
 * TODO implement a non-recursive method: it should not use
 * pifs_opendir() and store positions
 * size_t current_entry_pos[PIFS_MAX_DIR_DEPTH];
 *
 * @param[in] a_path            Path to walk.
 * @param[in] a_recursive       Enter directories and walk them too.
 * @param[in] a_stop_at_error   TRUE: Stop at first error.
 * @param[in] a_dir_walker_func Pointer to callback function.
 * @param[in] a_func_data       User-data to pass to callback function.
 * @return PIFS_SUCCESS if successfully walked.
 */
pifs_status_t pifs_walk_dir(const pifs_char_t * const a_path, bool_t a_recursive, bool_t a_stop_at_error,
                            pifs_dir_walker_func_t a_dir_walker_func, void * a_func_data)
{
    pifs_status_t   ret = PIFS_ERROR_FILE_NOT_FOUND;
    pifs_status_t   ret2;
    pifs_status_t   ret_error = PIFS_SUCCESS;
    pifs_DIR      * dir;
    pifs_dirent_t * dirent;
#if PIFS_ENABLE_DIRECTORIES
    pifs_char_t     path[PIFS_PATH_LEN_MAX];
#endif

#if !PIFS_ENABLE_DIRECTORIES
    (void)a_recursive;
#endif

    dir = pifs_opendir(a_path);
    if (dir != NULL)
    {
        ret = PIFS_SUCCESS;
        while ((dirent = pifs_readdir(dir)) && ret == PIFS_SUCCESS)
        {
            ret2 = (*a_dir_walker_func)(dirent, a_func_data);
            if (a_stop_at_error)
            {
                ret = ret2;
            }
            else if (ret2 != PIFS_SUCCESS)
            {
                ret_error = ret2;
            }
#if PIFS_ENABLE_DIRECTORIES
            if (ret == PIFS_SUCCESS)
            {
                if (a_recursive && PIFS_IS_DIR(dirent->d_attrib))
                {
                    if (a_path)
                    {
                        strncpy(path, a_path, PIFS_PATH_LEN_MAX);
                    }
                    else
                    {
                        path[0] = PIFS_ROOT_CHAR;
                        path[1] = PIFS_EOS;
                    }
                    pifs_append_path(path, PIFS_PATH_LEN_MAX, dirent->d_name);
                    PIFS_NOTICE_MSG("Entering directory '%s'...\r\n", path);
                    ret = pifs_walk_dir(path, TRUE, a_stop_at_error, a_dir_walker_func, a_func_data);
                }
            }
#endif
        }
        if (pifs_closedir(dir) != 0)
        {
            PIFS_ERROR_MSG("Cannot close directory!\r\n");
            ret = PIFS_ERROR_GENERAL;
        }
    }

    if (!a_stop_at_error && ret == PIFS_SUCCESS)
    {
        ret = ret_error;
    }

    return ret;
}

#if PIFS_ENABLE_DIRECTORIES
/**
 * @brief pifs_mkdir Create directory.
 *
 * @param[in] a_filename Path to create.
 * @return PIFS_SUCCESS if directory successfully created.
 */
int pifs_mkdir(const pifs_char_t * const a_filename)
{
    int ret;

    PIFS_GET_MUTEX();

    ret = pifs_internal_mkdir(a_filename, TRUE);

    PIFS_PUT_MUTEX();

    return ret;
}

/**
 * @brief pifs_mkdir Create directory.
 *
 * @param[in] a_filename Path to create.
 * @return PIFS_SUCCESS if directory successfully created.
 */
pifs_status_t pifs_internal_mkdir(const pifs_char_t * const a_filename, bool_t a_is_merge_allowed)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_entry_t       * entry = &pifs.entry;
    pifs_block_address_t ba;
    pifs_page_address_t  pa;
    pifs_page_count_t    page_count_found;
    pifs_address_t       entry_list_address;
    pifs_char_t          filename[PIFS_FILENAME_LEN_MAX];

    if (a_is_merge_allowed)
    {
        ret = pifs_merge_check(NULL, 1);
    }
    if (ret == PIFS_SUCCESS)
    {
        /* Get entry list's address AFTER merge as it can change during merge! */
        entry_list_address = *pifs_get_task_current_entry_list_address();
        /* Resolve a_filename's relative/absolute file path and update
         * entry_list_address regarding that */
        ret = pifs_resolve_path(a_filename, entry_list_address,
                                filename, &entry_list_address);
    }
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_find_entry(PIFS_FIND_ENTRY, filename, entry,
                              entry_list_address.block_address,
                              entry_list_address.page_address);
    }
    if (ret == PIFS_SUCCESS)
    {
        ret = PIFS_ERROR_FILE_ALREADY_EXIST;
    }
    else if (ret == PIFS_ERROR_FILE_NOT_FOUND)
    {
        ret = PIFS_SUCCESS;
        /* Order of steps to create a directory: */
        /* #1 Find free pages for entry list */
        /* #2 Create entry of a_file, which contains the entry list's address */
        /* #3 Mark entry list page */
        /* #4 Create "." and ".." entries */
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
            strncpy((char*)entry->name, filename, PIFS_FILENAME_LEN_MAX);
            PIFS_SET_ATTRIB(entry->attrib, PIFS_ATTRIB_ARCHIVE | PIFS_ATTRIB_DIR);
            entry->first_map_address.block_address = ba;
            entry->first_map_address.page_address = pa;
            ret = pifs_append_entry(entry,
                                    entry_list_address.block_address,
                                    entry_list_address.page_address);
            if (ret == PIFS_SUCCESS)
            {
                PIFS_DEBUG_MSG("Entry created\r\n");
                ret = pifs_mark_page(ba, pa, PIFS_ENTRY_LIST_SIZE_PAGE, TRUE, FALSE);
                if (ret == PIFS_SUCCESS)
                {
                    /* Add current directory's entry "." */
                    memset(entry, PIFS_FLASH_ERASED_BYTE_VALUE, PIFS_ENTRY_SIZE_BYTE);
                    strncpy((char*)entry->name, PIFS_DOT_STR, PIFS_FILENAME_LEN_MAX);
                    PIFS_SET_ATTRIB(entry->attrib, PIFS_ATTRIB_ARCHIVE | PIFS_ATTRIB_DIR);
                    entry->first_map_address.block_address = ba;
                    entry->first_map_address.page_address = pa;
                    ret = pifs_append_entry(entry, ba, pa);
                }
                if (ret == PIFS_SUCCESS)
                {
                    /* Add upper directory's entry ".." */
                    strncpy((char*)entry->name, PIFS_DOUBLE_DOT_STR, PIFS_FILENAME_LEN_MAX);
                    PIFS_SET_ATTRIB(entry->attrib, PIFS_ATTRIB_ARCHIVE | PIFS_ATTRIB_DIR);
                    entry->first_map_address.block_address = entry_list_address.block_address;
                    entry->first_map_address.page_address = entry_list_address.page_address;
                    ret = pifs_append_entry(entry, ba, pa);
                }
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

/**
 * @brief pifs_rmdir Remove directory.
 *
 * @param[in] a_filename Path and directory to remove.
 * @return PIFS_SUCCESS if directory removed.
 */
int pifs_rmdir(const pifs_char_t * const a_filename)
{
    pifs_status_t        ret = PIFS_SUCCESS;
    pifs_entry_t       * entry = &pifs.entry;
    pifs_address_t       entry_list_address;
    pifs_char_t          filename[PIFS_FILENAME_LEN_MAX];

    PIFS_GET_MUTEX();

    entry_list_address = *pifs_get_task_current_entry_list_address();

    if (pifs_is_directory_empty(a_filename))
    {
        /* Resolve a_filename's relative/absolute file path and update */
        /* entry_list_address regarding that */
        ret = pifs_resolve_path(a_filename, entry_list_address,
                                filename, &entry_list_address);

        if (ret == PIFS_SUCCESS)
        {
            if (!PIFS_IS_DOT_DIR(filename))
            {
                ret = pifs_find_entry(PIFS_DELETE_ENTRY, filename, entry,
                                      entry_list_address.block_address,
                                      entry_list_address.page_address);
            }
            else
            {
                /* "." and ".." directories cannot be removed. */
                ret = PIFS_ERROR_GENERAL;
            }
        }
    }
    else
    {
        ret = PIFS_ERROR_DIRECTORY_NOT_EMPTY;
    }

    PIFS_PUT_MUTEX();

    return ret;
}

/**
 * @brief pifs_chdir Change current directory.
 *
 * @param[in] a_filename Path to directory.
 * @return PIFS_SUCCESS if directory successfully changed.
 */
int pifs_chdir(pifs_char_t * const a_filename)
{
    int ret;

    PIFS_GET_MUTEX();

    ret = pifs_internal_chdir(a_filename);

    PIFS_PUT_MUTEX();

    return ret;
}

/**
 * @brief pifs_chdir Change current directory.
 *
 * @param[in] a_filename Path to directory.
 * @return PIFS_SUCCESS if directory successfully changed.
 */
pifs_status_t pifs_internal_chdir(pifs_char_t * const a_filename)
{
    pifs_status_t     ret = PIFS_SUCCESS;
    pifs_entry_t    * entry = &pifs.entry;
    pifs_address_t    entry_list_address;
    pifs_address_t  * current_entry_list_address;
    pifs_char_t       separator[2] = { PIFS_PATH_SEPARATOR_CHAR, 0 };
    pifs_char_t     * cwd;

    current_entry_list_address = pifs_get_task_current_entry_list_address();
    entry_list_address = *current_entry_list_address;
    cwd = pifs_get_task_cwd();

    if (a_filename[0] == PIFS_PATH_SEPARATOR_CHAR
            && a_filename[1] == PIFS_EOS)
    {
        /* Root directory: "/" or "\" */
        *current_entry_list_address = pifs.header.root_entry_list_address;
        cwd[0] = PIFS_PATH_SEPARATOR_CHAR;
        cwd[1] = PIFS_EOS;
    }
    else
    {
        /* Resolve a_filename's relative/absolute file path and update */
        /* entry_list_address regarding that */
        ret = pifs_resolve_dir(a_filename, *current_entry_list_address,
                               &entry_list_address);

        if (ret == PIFS_SUCCESS)
        {
            /* Update current working directory (cwd) */
            *current_entry_list_address = entry->first_map_address;
            if (cwd[strlen(cwd) - 1] != PIFS_PATH_SEPARATOR_CHAR)
            {
                strncat(cwd, separator, PIFS_PATH_LEN_MAX);
            }
            strncat(cwd, a_filename, PIFS_PATH_LEN_MAX);
            pifs_normalize_path(cwd);
            if (cwd[0] == PIFS_EOS)
            {
                cwd[0] = PIFS_PATH_SEPARATOR_CHAR;
                cwd[1] = PIFS_EOS;
            }
        }
    }

    return ret;
}

/**
 * @brief pifs_getcwd Get current working directory.
 *
 * @param[out] a_buffer Path to fill.
 * @param[in] a_size    Size of a_buffer.
 * @return Pointer to a_buffer.
 */
pifs_char_t * pifs_getcwd(pifs_char_t * a_buffer, size_t a_size)
{
    pifs_char_t * cwd;

    PIFS_GET_MUTEX();

    cwd = pifs_get_task_cwd();
    strncpy(a_buffer, cwd, a_size);

    PIFS_PUT_MUTEX();

    return a_buffer;
}

/**
 * @brief pifs_append_path Append path to another path.
 *
 * @param[out] a_path_dst   Destination path. Source path will be appended to this one.
 * @param[in] a_size        Maximum length of destination path.
 * @param[in] a_path2       Source path.
 * @return Pointer to a_path_dst.
 */
pifs_char_t * pifs_append_path(pifs_char_t * a_path_dst, size_t a_size, const pifs_char_t * a_path_src)
{
    size_t len;

    len = strlen(a_path_dst);
    if (a_path_dst[len - 1] != PIFS_PATH_SEPARATOR_CHAR
            && a_path_src[0] != PIFS_PATH_SEPARATOR_CHAR)
    {
        strncat(a_path_dst, PIFS_PATH_SEPARATOR_STR, a_size);
    }
    strncat(a_path_dst, a_path_src, a_size);

    return a_path_dst;
}
#endif
