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
#include "pifs_wear.h"
#include "buffer.h" /* DEBUG */

#define PIFS_DEBUG_LEVEL 2
#include "pifs_debug.h"

/**
 * @brief pifs_internal_open Internally used function to open a file.
 *
 * @param[in] a_file                Pointer to internal file structure.
 * @param[in] a_filename            Pointer to file name.
 * @param[in] a_modes               Pointer to open mode. NULL: open with existing modes.
 * @param[in] a_is_merge_allowed    TRUE: merge can be started.
 */
pifs_status_t pifs_internal_open(pifs_file_t * a_file,
                                 const pifs_char_t * a_filename,
                                 const pifs_char_t * a_modes,
                                 bool_t a_is_merge_allowed)
{
    pifs_entry_t       * entry = &a_file->entry;
    pifs_block_address_t ba = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_page_address_t  pa = PIFS_PAGE_ADDRESS_INVALID;
    pifs_page_count_t    page_count_found = 0;

    PIFS_ASSERT(!a_file->is_opened);
    a_file->status = PIFS_SUCCESS;
    a_file->write_pos = 0;
    a_file->write_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
    a_file->write_address.page_address = PIFS_PAGE_ADDRESS_INVALID;
    a_file->read_pos = 0;
    a_file->actual_map_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
    a_file->actual_map_address.page_address = PIFS_PAGE_ADDRESS_INVALID;
    a_file->is_size_changed = FALSE;
    if (a_modes)
    {
        pifs_parse_open_mode(a_file, a_modes);
    }
    if (a_file->status == PIFS_SUCCESS)
    {
        a_file->status = pifs_find_entry(PIFS_FIND_ENTRY, a_filename, entry,
                                         pifs.header.entry_list_address.block_address,
                                         pifs.header.entry_list_address.page_address);
        if ((a_file->mode_file_shall_exist || a_file->mode_append) && a_file->status == PIFS_SUCCESS)
        {
            PIFS_DEBUG_MSG("Entry of %s found\r\n", a_filename);
#if PIFS_DEBUG_LEVEL >= 6
            print_buffer(entry, sizeof(pifs_entry_t), 0);
#endif
#if PIFS_ENABLE_DIRECTORIES
            if (!(a_file->entry.attrib & PIFS_ATTRIB_DIR))
#endif
            {
                /* Check if file size is valid or file is opened during merge */
                if ((a_file->entry.file_size < PIFS_FILE_SIZE_ERASED && a_is_merge_allowed)
                        || !a_is_merge_allowed)
                {
                    a_file->is_opened = TRUE;
                }
                else
                {
                    PIFS_DEBUG_MSG("File size of %s is invalid!\r\n", a_filename);
                    a_file->status = PIFS_ERROR_FILE_NOT_FOUND;
                }
            }
#if PIFS_ENABLE_DIRECTORIES
            else
            {
                a_file->status = PIFS_ERROR_IS_A_DIRECTORY;
            }
#endif
        }
        if (a_file->mode_create_new_file || (a_file->mode_append && !a_file->is_opened))
        {
            if (a_file->status == PIFS_SUCCESS)
            {
                /* File already exist */
#if PIFS_USE_DELTA_FOR_ENTRIES == 0
                a_file->status = pifs_delete_entry(a_filename,
                                                   pifs.header.entry_list_address.block_address,
                                                   pifs.header.entry_list_address.page_address);
                if (a_file->status == PIFS_SUCCESS)
#endif
                {
                    /* Mark allocated pages to be released */
                    a_file->status = pifs_release_file_pages(a_file);
                    a_file->is_size_changed = TRUE;
                }
            }
            else
            {
                /* File does not exists, no problem, we'll create it */
                a_file->status = PIFS_SUCCESS;
            }
            if (a_file->status == PIFS_SUCCESS && a_is_merge_allowed)
            {
                a_file->status = pifs_merge_check(NULL, 1);
            }
            /* Order of steps to create a file: */
            /* #1 Find a free page for map of file */
            /* #2 Create entry of a_file, which contains the map's address */
            /* #3 Mark map page */
            if (a_file->status == PIFS_SUCCESS)
            {
                a_file->status = pifs_find_free_page_wl(PIFS_MAP_PAGE_NUM, PIFS_MAP_PAGE_NUM,
                                                        PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT,
                                                        &ba, &pa, &page_count_found);
            }
            if (a_file->status == PIFS_SUCCESS)
            {
                PIFS_DEBUG_MSG("Map page: %u free page found %s\r\n", page_count_found, pifs_ba_pa2str(ba, pa));
                memset(entry, PIFS_FLASH_ERASED_BYTE_VALUE, PIFS_ENTRY_SIZE_BYTE);
                strncpy((char*)entry->name, a_filename, PIFS_FILENAME_LEN_MAX);
#if PIFS_ENABLE_ATTRIBUTES
                entry->attrib = PIFS_ATTRIB_ARCHIVE;
#endif
                entry->first_map_address.block_address = ba;
                entry->first_map_address.page_address = pa;
                a_file->status = pifs_append_entry(entry,
                                                   pifs.header.entry_list_address.block_address,
                                                   pifs.header.entry_list_address.page_address);
                if (a_file->status == PIFS_SUCCESS)
                {
                    PIFS_DEBUG_MSG("Entry created\r\n");
                    a_file->status = pifs_mark_page(ba, pa, PIFS_MAP_PAGE_NUM, TRUE);
                    if (a_file->status == PIFS_SUCCESS)
                    {
                        a_file->is_opened = TRUE;
                    }
                }
                else
                {
                    PIFS_DEBUG_MSG("Cannot create entry!\r\n");
                    PIFS_SET_ERRNO(PIFS_ERROR_NO_MORE_ENTRY);
                }
            }
            else
            {
                PIFS_DEBUG_MSG("No free page found!\r\n");
                PIFS_SET_ERRNO(PIFS_ERROR_NO_MORE_SPACE);
            }
        }
        if (a_file->is_opened)
        {
            a_file->status = pifs_read_first_map_entry(a_file);
            PIFS_ASSERT(a_file->status == PIFS_SUCCESS);
            a_file->read_address = a_file->map_entry.address;
            a_file->read_page_count = a_file->map_entry.page_count;
        }
    }

    return a_file->status;
}

/**
 * @brief pifs_fopen Open file, works like fopen().
 *
 * @param[in] a_filename    File name to open.
 * @param[in] a_modes       Open mode: "r", "r+", "w", "w+", "a" or "a+".
 * @return Pointer to file if file opened successfully.
 */
P_FILE * pifs_fopen(const pifs_char_t * a_filename, const pifs_char_t * a_modes)
{
    pifs_file_t  * file = NULL;
    pifs_status_t  ret;

    PIFS_NOTICE_MSG("filename: '%s' modes: %s\r\n", a_filename, a_modes);
    ret = pifs_get_file(&file);
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_check_filename(a_filename);
    }
    if (ret == PIFS_SUCCESS)
    {
        (void)pifs_internal_open(file, a_filename, a_modes, TRUE);
        PIFS_NOTICE_MSG("status: %i is_opened: %i\r\n", file->status, file->is_opened);
        if (file->status == PIFS_SUCCESS && file->is_opened)
        {
            PIFS_NOTICE_MSG("file size: %i\r\n", file->entry.file_size);
        }
        else
        {
            file = NULL;
        }
    }

    PIFS_SET_ERRNO(ret);
    return (P_FILE*) file;
}

/**
 * @brief pifs_inc_write_address Increment write address for file writing.
 *
 * @param[in] a_file Pointer to the internal file structure.
 */
pifs_status_t pifs_inc_write_address(pifs_file_t * a_file)
{
    //PIFS_DEBUG_MSG("started %s\r\n", pifs_address2str(&a_file->write_address));
    a_file->write_page_count--;
    if (a_file->write_page_count)
    {
        a_file->status = pifs_inc_address(&a_file->write_address);
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
                a_file->write_address = a_file->map_entry.address;
                a_file->write_page_count = a_file->map_entry.page_count;
            }
        }
        else
        {
            //      PIFS_DEBUG_MSG("status: %i\r\n", a_file->status);
        }
    }
    //  PIFS_DEBUG_MSG("exited %s\r\n", pifs_address2str(&a_file->write_address));
    return a_file->status;
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
    pifs_size_t          data_size = a_size * a_count;
    pifs_size_t          written_size = 0;
    pifs_page_count_t    page_count_needed = 0;
    pifs_page_count_t    page_count_needed_limited = 0;
    pifs_page_count_t    page_count_found;
    pifs_page_count_t    page_cound_found_start;
    pifs_size_t          chunk_size;
    pifs_block_address_t ba = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_block_address_t ba_start = PIFS_BLOCK_ADDRESS_INVALID;
    pifs_page_address_t  pa = PIFS_PAGE_ADDRESS_INVALID;
    pifs_page_address_t  pa_start = PIFS_PAGE_ADDRESS_INVALID;
    pifs_page_offset_t   po = PIFS_PAGE_OFFSET_INVALID;
    bool_t               is_delta = FALSE;
    bool_t               is_free_map_entry;
    pifs_size_t          free_management_page_count = 0;
    pifs_size_t          free_data_page_count = 0;

    PIFS_NOTICE_MSG("filename: '%s', size: %i, count: %i\r\n", file->entry.name, a_size, a_count);
    if (pifs.is_header_found && file && file->is_opened && file->mode_write)
    {
        file->status = PIFS_SUCCESS;
        /* If opened in "a" mode always jump to end of file */
        if (file->mode_append && file->write_pos != file->entry.file_size)
        {
            pifs_fseek(file, file->entry.file_size, PIFS_SEEK_SET);
        }
        if (file->status == PIFS_SUCCESS)
        {
            po = file->write_pos % PIFS_LOGICAL_PAGE_SIZE_BYTE;
            /* Check if last page was not fully used up */
            if (po && file->write_pos == file->entry.file_size)
            {
                /* There is some space in the last page */
                PIFS_ASSERT(pifs_is_address_valid(&file->write_address));
                chunk_size = PIFS_MIN(data_size, PIFS_LOGICAL_PAGE_SIZE_BYTE - po);
                //PIFS_DEBUG_MSG("--------> pos: %i po: %i data_size: %i chunk_size: %i\r\n",
                //               file->write_pos, po, data_size, chunk_size);
                file->status = pifs_write(file->write_address.block_address,
                                          file->write_address.page_address,
                                          po, data, chunk_size);
                //pifs_print_cache();
                if (file->status == PIFS_SUCCESS)
                {
                    data += chunk_size;
                    data_size -= chunk_size;
                    written_size += chunk_size;
                }
            }
        }

        if (data_size > 0)
        {
            if (file->status == PIFS_SUCCESS)
            {
                page_count_needed = (data_size + PIFS_LOGICAL_PAGE_SIZE_BYTE - 1) / PIFS_LOGICAL_PAGE_SIZE_BYTE;
                /* Check if merge needed and do it if necessary */
                file->status = pifs_merge_check(a_file, page_count_needed);
            }
            if (file->status == PIFS_SUCCESS && file->entry.file_size != PIFS_FILE_SIZE_ERASED
                    && file->write_pos < file->entry.file_size)
            {
                /* Overwriting existing pages in the file */
                PIFS_WARNING_MSG("Overwriting pages, pos: %i, size: %i\r\n", file->write_pos, file->entry.file_size);
                //page_count_needed = (data_size + PIFS_LOGICAL_PAGE_SIZE_BYTE - 1) / PIFS_LOGICAL_PAGE_SIZE_BYTE;
                while (page_count_needed && file->status == PIFS_SUCCESS)
                {
                    chunk_size = PIFS_MIN(data_size, PIFS_LOGICAL_PAGE_SIZE_BYTE);
                    if (file->write_pos + chunk_size > file->entry.file_size)
                    {
                        chunk_size = file->entry.file_size - file->write_pos;
                    }
                    PIFS_NOTICE_MSG("write %s, page_count_needed: %i, chunk size: %i\r\n",
                                    pifs_address2str(&file->write_address), page_count_needed, chunk_size);
                    PIFS_ASSERT(pifs_is_address_valid(&file->write_address));
                    file->status = pifs_write_delta(file->write_address.block_address,
                                                    file->write_address.page_address,
                                                    0, data, chunk_size, &is_delta,
                                                    &pifs.header);
                    if (file->status == PIFS_SUCCESS && chunk_size == PIFS_LOGICAL_PAGE_SIZE_BYTE)
                    {
                        pifs_inc_write_address(file);
                    }
                    data += chunk_size;
                    data_size -= chunk_size;
                    written_size += chunk_size;
                    page_count_needed--;
                }
            }
        }
        if (data_size > 0)
        {
            /* Map entry is needed when write position is at file's end */
            if (file->status == PIFS_SUCCESS && file->write_pos == file->entry.file_size)
            {
                /* Check if at least one map can be added after writing */
                file->status = pifs_is_free_map_entry(a_file, &is_free_map_entry);
                if (!is_free_map_entry)
                {
                    /* No free entry in actual map page, so check if there are */
                    /* at least one free management page and a new map page */
                    /* can be allocated. */
                    file->status = pifs_get_free_pages(&free_management_page_count,
                                                       &free_data_page_count);
                    if (file->status == PIFS_SUCCESS && !free_management_page_count)
                    {
                        file->status = PIFS_ERROR_NO_MORE_SPACE;
                    }
                }
            }
            if (file->status == PIFS_SUCCESS)
            {
                /* Appending new pages to the file */
                PIFS_WARNING_MSG("Appending pages\r\n");
                do
                {
                    page_count_needed_limited = page_count_needed;
                    if (page_count_needed_limited >= PIFS_MAP_PAGE_COUNT_INVALID)
                    {
                        page_count_needed_limited = PIFS_MAP_PAGE_COUNT_INVALID - 1;
                    }
                    file->status = pifs_find_free_page_wl(1, page_count_needed_limited,
                                                          PIFS_BLOCK_TYPE_DATA,
                                                          &ba, &pa, &page_count_found);
                    PIFS_DEBUG_MSG("%u pages found. %s, status: %i\r\n",
                                   page_count_found, pifs_ba_pa2str(ba, pa), file->status);
                    if (file->status == PIFS_SUCCESS)
                    {
                        ba_start = ba;
                        pa_start = pa;
                        page_cound_found_start = page_count_found;
                        do
                        {
                            if (data_size > PIFS_LOGICAL_PAGE_SIZE_BYTE)
                            {
                                chunk_size = PIFS_LOGICAL_PAGE_SIZE_BYTE;
                            }
                            else
                            {
                                chunk_size = data_size;
                            }
                            file->status = pifs_write_delta(ba, pa, 0, data, chunk_size, &is_delta,
                                                            &pifs.header);
                            PIFS_DEBUG_MSG("%s is_delta: %i status: %i\r\n", pifs_ba_pa2str(ba, pa),
                                           is_delta, file->status);
                            /* Save last page's address for future use */
                            file->write_address.block_address = ba;
                            file->write_address.page_address = pa;
                            data += chunk_size;
                            data_size -= chunk_size;
                            written_size += chunk_size;
                            page_count_found--;
                            page_count_needed--;
                            if (page_count_found)
                            {
                                file->status = pifs_inc_ba_pa(&ba, &pa);
                            }
                        } while (page_count_found && file->status == PIFS_SUCCESS);

                        if (file->status == PIFS_SUCCESS && !is_delta)
                        {
                            /* Write pages to file map entry after successfully write */
                            file->status = pifs_append_map_entry(file, ba_start, pa_start, page_cound_found_start);
                            //PIFS_ASSERT(file->status == PIFS_SUCCESS);
                        }
                    }
                } while (page_count_needed && file->status == PIFS_SUCCESS);
            }
        }
        file->write_pos += written_size;
        if (file->write_pos > file->entry.file_size
                || file->entry.file_size == PIFS_FILE_SIZE_ERASED)
        {
            file->is_size_changed = TRUE;
            file->entry.file_size = file->write_pos;
            PIFS_DEBUG_MSG("File size increased to %i bytes\r\n", file->entry.file_size);
        }
        if (!file->mode_append)
        {
            file->read_pos = file->write_pos;
            file->read_address = file->write_address;
        }
    }

    PIFS_SET_ERRNO(file->status);
    return written_size / a_size;
}

/**
 * @brief pifs_inc_read_address Increment read address for file reading.
 *
 * @param[in] a_file Pointer to the internal file structure.
 */
pifs_status_t pifs_inc_read_address(pifs_file_t * a_file)
{
    //PIFS_DEBUG_MSG("started %s\r\n", pifs_address2str(&a_file->read_address));
    a_file->read_page_count--;
    if (a_file->read_page_count)
    {
        a_file->status = pifs_inc_address(&a_file->read_address);
    }
    else
    {
        a_file->status = pifs_read_next_map_entry(a_file);
        if (a_file->status == PIFS_SUCCESS)
        {
            //      print_buffer(&a_file->map_entry, PIFS_MAP_ENTRY_SIZE_BYTE, 0);
            if (pifs_is_buffer_erased(&a_file->map_entry, PIFS_MAP_ENTRY_SIZE_BYTE))
            {
                PIFS_WARNING_MSG("End of file: %i\r\n", a_file->read_pos);
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
    //  PIFS_DEBUG_MSG("exited %s\r\n", pifs_address2str(&a_file->read_address));
    return a_file->status;
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
    pifs_size_t          read_size = 0;
    pifs_size_t          chunk_size = 0;
    pifs_size_t          data_size = a_size * a_count;
    pifs_size_t          page_count = 0;
    pifs_page_offset_t   po;

    PIFS_NOTICE_MSG("filename: '%s', size: %i, count: %i\r\n", file->entry.name, a_size, a_count);
    if (pifs.is_header_found && file && file->is_opened && file->mode_read)
    {
        if (file->read_pos + data_size > file->entry.file_size)
        {
            PIFS_WARNING_MSG("Trying to read more than file size: %i, file size: %i\r\n",
                             file->read_pos + data_size,
                             file->entry.file_size);
            data_size = file->entry.file_size - file->read_pos;
        }
        po = file->read_pos % PIFS_LOGICAL_PAGE_SIZE_BYTE;
        /* Check if last page was not fully read */
        if (po)
        {
            /* There is some data in the last page */
            //          PIFS_DEBUG_MSG("po != 0  %s\r\n", pifs_address2str(&file->read_address));
            PIFS_ASSERT(pifs_is_address_valid(&file->read_address));
            chunk_size = PIFS_MIN(data_size, PIFS_LOGICAL_PAGE_SIZE_BYTE - po);
            PIFS_DEBUG_MSG("--------> pos: %i po: %i data_size: %i chunk_size: %i\r\n",
                           file->read_pos, po, data_size, chunk_size);
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
                if (po + chunk_size >= PIFS_LOGICAL_PAGE_SIZE_BYTE)
                {
                    pifs_inc_read_address(file);
                }
            }
        }
        if (file->status == PIFS_SUCCESS && data_size > 0 && file->read_pos < file->entry.file_size)
        {
            page_count = (data_size + PIFS_LOGICAL_PAGE_SIZE_BYTE - 1) / PIFS_LOGICAL_PAGE_SIZE_BYTE;
            while (page_count && file->status == PIFS_SUCCESS)
            {
                chunk_size = PIFS_MIN(data_size, PIFS_LOGICAL_PAGE_SIZE_BYTE);
                if (file->read_pos + chunk_size > file->entry.file_size)
                {
                    chunk_size = file->entry.file_size - file->read_pos;
                }
                PIFS_NOTICE_MSG("read %s, page_count: %i, chunk size: %i\r\n",
                               pifs_address2str(&file->read_address), page_count, chunk_size);
                PIFS_ASSERT(pifs_is_address_valid(&file->read_address));
                file->status = pifs_read_delta(file->read_address.block_address,
                                               file->read_address.page_address,
                                               0, data, chunk_size);
                if (file->status == PIFS_SUCCESS && chunk_size == PIFS_LOGICAL_PAGE_SIZE_BYTE)
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
        if (!file->mode_append)
        {
            file->write_pos = file->read_pos;
            file->write_address = file->read_address;
        }
    }

    PIFS_SET_ERRNO(file->status);
    return read_size / a_size;
}

/**
 * @brief pifs_fflush Flush cache, update file size.
 * @param[in] a_file File to flush.
 * @return 0 if file close, PIFS_EOF if error occurred.
 */
int pifs_fflush(P_FILE * a_file)
{
    int ret = PIFS_EOF;
    pifs_file_t * file = (pifs_file_t*) a_file;

    PIFS_NOTICE_MSG("filename: '%s'\r\n", file->entry.name);
    if (pifs.is_header_found && file && file->is_opened)
    {
        PIFS_DEBUG_MSG("mode_write: %i, is_size_changed: %i, file_size: %i\r\n",
                       file->mode_write, file->is_size_changed,
                       file->entry.file_size);
        if (file->mode_write && (file->is_size_changed || !file->entry.file_size))
        {
            file->status = pifs_update_entry(file->entry.name, &file->entry,
                                             pifs.header.entry_list_address.block_address,
                                             pifs.header.entry_list_address.page_address);
        }
        pifs_flush();
        ret = 0;
    }

    PIFS_SET_ERRNO(file->status);
    return ret;
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

    PIFS_NOTICE_MSG("filename: '%s'\r\n", file->entry.name);
    ret = pifs_fflush(a_file);
    if (ret == 0)
    {
        file->is_opened = FALSE;
        file->is_used = FALSE;
    }

    return ret;
}

/**
 * @brief pifs_fseek Seek in opened file.
 *
 * @param[in] a_file    File to seek.
 * @param[in] a_offset  Offset to seek.
 * @param[in] a_origin  Origin of offset. Can be
 *                      PIFS_SEEK_SET, PIFS_SEEK_CUR, PIFS_SEEK_END.
 *                      @see pifs_fseek_origin_t
 * @return 0 if seek was successful. Non-zero if error occured.
 */
int pifs_fseek(P_FILE * a_file, long int a_offset, int a_origin)
{
    int                 ret = PIFS_ERROR_GENERAL;
    pifs_file_t       * file = (pifs_file_t*) a_file;
    pifs_size_t         seek_size = 0;
    pifs_size_t         chunk_size = 0;
    pifs_size_t         data_size = 0;
    pifs_size_t         page_count = 0;
    pifs_page_offset_t  po;
    pifs_size_t         target_pos = 0;
    pifs_size_t         file_size = 0;

    PIFS_NOTICE_MSG("filename: '%s', offset: %i, origin: %i, read_pos: %i\r\n",
                    file->entry.name, a_offset, a_origin, file->read_pos);
    if (pifs.is_header_found && file && file->is_opened)
    {
        switch (a_origin)
        {
            case PIFS_SEEK_CUR:
                target_pos = file->read_pos + a_offset;
                if (a_offset > 0)
                {
                    data_size = a_offset;
                }
                else
                {
                    /* TODO implement better method: */
                    /* if read_pos + a_offset > read_pos / 2 */
                    /* (if position is close to current position) */
                    /* go backward using map header's previous address entry */
                    data_size = file->read_pos + a_offset;
                    pifs_rewind(file); /* Zeroing file->read_pos! */
                }
                break;
            case PIFS_SEEK_SET:
                if ((long int)file->read_pos < a_offset)
                {
                    data_size = a_offset - file->read_pos;
                }
                else
                {
                    data_size = a_offset;
                    pifs_rewind(file); /* Zeroing file->read_pos! */
                }
                target_pos = a_offset;
                break;
            case PIFS_SEEK_END:
                if (file->entry.file_size == PIFS_FILE_SIZE_ERASED)
                {
                    file->status = PIFS_ERROR_SEEK_NOT_POSSIBLE;
                }
                else
                {
                    data_size = file->entry.file_size + a_offset;
                    if (data_size >= file->read_pos)
                    {
                        data_size -= file->read_pos;
                    }
                    else
                    {
                        pifs_rewind(file); /* Zeroing file->read_pos! */
                    }
                    target_pos = data_size;
                }
                break;
            default:
                break;
        }

        if (file->status == PIFS_SUCCESS)
        {
            if (file->entry.file_size != PIFS_FILE_SIZE_ERASED)
            {
                file_size = file->entry.file_size;
            }
            if (target_pos > file_size)
            {
                PIFS_WARNING_MSG("Trying to seek position %i while file size is %i bytes\r\n",
                                 target_pos, file_size);
                data_size -= target_pos - file_size;
                PIFS_WARNING_MSG("data_size: %i bytes\r\n", data_size);
            }

            po = file->read_pos % PIFS_LOGICAL_PAGE_SIZE_BYTE;
            /* Check if last page was not fully read */
            if (po)
            {
                /* There is some data in the last page */
                PIFS_ASSERT(pifs_is_address_valid(&file->read_address));
                chunk_size = PIFS_MIN(data_size, PIFS_LOGICAL_PAGE_SIZE_BYTE - po);
                if (file->status == PIFS_SUCCESS)
                {
                    data_size -= chunk_size;
                    seek_size += chunk_size;
                    if (po + chunk_size >= PIFS_LOGICAL_PAGE_SIZE_BYTE)
                    {
                        pifs_inc_read_address(file);
                    }
                }
            }
            if (file->status == PIFS_SUCCESS && data_size > 0)
            {
                page_count = (data_size + PIFS_LOGICAL_PAGE_SIZE_BYTE - 1) / PIFS_LOGICAL_PAGE_SIZE_BYTE;
                while (page_count && file->status == PIFS_SUCCESS)
                {
                    PIFS_ASSERT(pifs_is_address_valid(&file->read_address));
                    chunk_size = PIFS_MIN(data_size, PIFS_LOGICAL_PAGE_SIZE_BYTE);
                    //PIFS_DEBUG_MSG("read %s\r\n", pifs_address2str(&file->read_address));
                    if (file->status == PIFS_SUCCESS && chunk_size == PIFS_LOGICAL_PAGE_SIZE_BYTE)
                    {
                        pifs_inc_read_address(file);
                        if (file->status == PIFS_ERROR_END_OF_FILE)
                        {
                            /* Reaching end of file is not an error */
                            file->status = PIFS_SUCCESS;
                        }
                    }
                    data_size -= chunk_size;
                    seek_size += chunk_size;
                    page_count--;
                }
            }
            file->read_pos += seek_size;
            file->write_pos = file->read_pos;
            file->write_address = file->read_address;
            /* Check if we are at end of file and data needs to be written to reach */
            /* target position */
            if (target_pos > file->read_pos)
            {
                if (file->mode_write)
                {
#if PIFS_ENABLE_FSEEK_BEYOND_FILE
#if PIFS_ENABLE_FSEEK_ERASED_VALUE
                    memset(pifs.sc_page_buf, PIFS_FLASH_ERASED_BYTE_VALUE, PIFS_LOGICAL_PAGE_SIZE_BYTE);
#else
                    memset(pifs.sc_page_buf, PIFS_FLASH_PROGRAMMED_BYTE_VALUE, PIFS_LOGICAL_PAGE_SIZE_BYTE);
#endif
                    data_size = target_pos - file->read_pos;
                    page_count = (data_size + PIFS_LOGICAL_PAGE_SIZE_BYTE - 1) / PIFS_LOGICAL_PAGE_SIZE_BYTE;
                    while (page_count && file->status == PIFS_SUCCESS)
                    {
                        chunk_size = PIFS_MIN(data_size, PIFS_LOGICAL_PAGE_SIZE_BYTE);
                        file->status = pifs_fwrite(pifs.sc_page_buf, 1, chunk_size, file);
                        data_size -= chunk_size;
                        page_count--;
                    }
                    if (data_size && file->status == PIFS_SUCCESS)
                    {
                        file->status = pifs_fwrite(pifs.sc_page_buf, 1, data_size, file);
                    }
#else
                    file->status = PIFS_ERROR_SEEK_NOT_POSSIBLE;
#endif
                }
                else
                {
                    file->status = PIFS_ERROR_SEEK_NOT_POSSIBLE;
                }
            }
        }
        ret = file->status;
    }

    PIFS_SET_ERRNO(file->status);
    return ret;
}

/**
 * @brief pifs_rewind Set file positions to zero.
 *
 * @param[in] a_file Pointer to file.
 */
void pifs_rewind(P_FILE * a_file)
{
    pifs_file_t * file = (pifs_file_t*) a_file;

    PIFS_NOTICE_MSG("filename: '%s'\r\n", file->entry.name);
    if (pifs.is_header_found && file && file->is_opened)
    {
        file->write_pos = 0;
        file->write_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
        file->write_address.page_address = PIFS_PAGE_ADDRESS_INVALID;
        file->read_pos = 0;
        file->actual_map_address.block_address = PIFS_BLOCK_ADDRESS_INVALID;
        file->actual_map_address.page_address = PIFS_PAGE_ADDRESS_INVALID;
        file->status = pifs_read_first_map_entry(file);
        PIFS_ASSERT(file->status == PIFS_SUCCESS);
        file->read_address = file->map_entry.address;
        file->read_page_count = file->map_entry.page_count;
    }
    PIFS_SET_ERRNO(file->status);
}

/**
 * @brief pifs_ftell Get current position in file.
 *
 * @param[in] a_file Pointer to file.
 * @return -1 if error or position in file.
 */
long int pifs_ftell(P_FILE * a_file)
{
    pifs_file_t * file = (pifs_file_t*) a_file;
    long int pos = -1;

    PIFS_NOTICE_MSG("filename: '%s'\r\n", file->entry.name);
    if (pifs.is_header_found && file && file->is_opened)
    {
        pos = file->read_pos;
    }

    return pos;
}

#if PIFS_ENABLE_USER_DATA
/**
 * @brief pifs_fgetuserdata Non-standard function to get user defined data of
 * file.
 *
 * @param[in] a_file        Pointer to file.
 * @param[out] a_user_data  Pointer to user data structure to fill.
 * @return 0 if success.
 */
int pifs_fgetuserdata(P_FILE * a_file, pifs_user_data_t * a_user_data)
{
    pifs_status_t ret = PIFS_ERROR_GENERAL;
    pifs_file_t * file = (pifs_file_t*) a_file;

    if (file->is_opened)
    {
        memcpy(a_user_data, &file->entry.user_data, sizeof(pifs_user_data_t));
        ret = PIFS_SUCCESS;
    }

    return ret;
}

/**
 * @brief pifs_fsetuserdata Non-standard function to set user defined data of
 * file.
 *
 * @param[in] a_file        Pointer to file.
 * @param[out] a_user_data  Pointer to user data structure to set.
 * @return 0 if success.
 */
int pifs_fsetuserdata(P_FILE * a_file, const pifs_user_data_t * a_user_data)
{
    pifs_file_t * file = (pifs_file_t*) a_file;

    if (file->is_opened)
    {
        memcpy(&file->entry.user_data, a_user_data, sizeof(pifs_user_data_t));

        file->status = pifs_update_entry(file->entry.name, &file->entry,
                                         pifs.header.entry_list_address.block_address,
                                         pifs.header.entry_list_address.page_address);
    }
    return file->status;
}
#endif

/**
 * @brief pifs_remove Remove file.
 *
 * @param[in] a_filename Pointer to filename to be removed.
 * @return 0 if file removed. Non-zero if file not found or file name is not valid.
 */
int pifs_remove(const pifs_char_t * a_filename)
{
    pifs_status_t ret;

    PIFS_NOTICE_MSG("filename: '%s'\r\n", a_filename);
    ret = pifs_check_filename(a_filename);
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_internal_open(&pifs.internal_file, a_filename, "r", FALSE);
        if (ret == PIFS_SUCCESS)
        {
            /* File already exist */
            ret = pifs_delete_entry(a_filename,
                                   pifs.header.entry_list_address.block_address,
                                   pifs.header.entry_list_address.page_address);
            if (ret == PIFS_SUCCESS)
            {
                /* Mark allocated pages to be released */
                ret = pifs_release_file_pages(&pifs.internal_file);
            }
            ret = pifs_fclose(&pifs.internal_file);
        }
    }

    PIFS_SET_ERRNO(ret);
    return ret;
}

/**
 * @brief pifs_rename Change name of file.
 *
 * @param[in] a_oldname Existing file's name.
 * @param[in] a_newname New file name.
 * @return 0: if rename was successfuly. Other: error occurred.
 */
int pifs_rename(const pifs_char_t * a_oldname, const pifs_char_t * a_newname)
{
    pifs_status_t  ret;
    pifs_entry_t * entry = &pifs.entry;

    PIFS_NOTICE_MSG("filename: '%s'\r\n", a_oldname);
    ret = pifs_check_filename(a_oldname);
    if (ret == PIFS_SUCCESS)
    {
        /* Checking NEW name */
        ret = pifs_find_entry(PIFS_FIND_ENTRY, a_newname, entry,
                              pifs.header.entry_list_address.block_address,
                              pifs.header.entry_list_address.page_address);
        if (ret == PIFS_SUCCESS)
        {
            /* File already exist, remove! */
            ret = pifs_remove(a_newname);
        }
        ret = pifs_find_entry(PIFS_FIND_ENTRY, a_oldname, entry,
                              pifs.header.entry_list_address.block_address,
                              pifs.header.entry_list_address.page_address);
        /* Checking OLD name */
        if (ret == PIFS_SUCCESS)
        {
            /* File already exist */
            ret = pifs_clear_entry(a_oldname,
                                   pifs.header.entry_list_address.block_address,
                                   pifs.header.entry_list_address.page_address);
        }
        if (ret == PIFS_SUCCESS)
        {
            /* Change name in entry and append new entry */
            strncpy(entry->name, a_newname, PIFS_FILENAME_LEN_MAX);
            ret = pifs_append_entry(entry,
                                    pifs.header.entry_list_address.block_address,
                                    pifs.header.entry_list_address.page_address);
        }
    }

    PIFS_SET_ERRNO(ret);
    return ret;
}

/**
 * @brief pifs_copy Copy file. This is a non-standard file operation.
 * Note: this function shall not use pifs.internal_file as merge could be
 * needed which uses the pifs.internal_file!
 *
 * @param[in] a_oldname Existing file's name.
 * @param[in] a_newname New file name.
 * @return 0: if copy was successfuly. Other: error occurred.
 */
int pifs_copy(const pifs_char_t * a_oldname, const pifs_char_t * a_newname)
{
    pifs_status_t    ret = PIFS_ERROR_NO_MORE_RESOURCE;
    pifs_file_t    * file = NULL;
    pifs_file_t    * file2 = NULL;
    pifs_size_t      read_bytes;
    pifs_size_t      written_bytes;

    PIFS_NOTICE_MSG("oldname: '%s', newname: '%s'\r\n", a_oldname, a_newname);

    file = pifs_fopen(a_oldname, "r");
    if (file)
    {
        file2 = pifs_fopen(a_newname, "w");
        if (file2)
        {
            do
            {
                read_bytes = pifs_fread(pifs.sc_page_buf, 1,
                                        PIFS_LOGICAL_PAGE_SIZE_BYTE, file);
                if (read_bytes > 0)
                {
                    written_bytes = pifs_fwrite(pifs.sc_page_buf, 1,
                                                read_bytes, file2);
                    if (read_bytes != written_bytes)
                    {
                        PIFS_ERROR_MSG("Cannot write: %i! Read bytes: %i, written bytes: %i\r\n",
                                       file2->status, read_bytes, written_bytes);
                        ret = file2->status;
                    }
                }
            } while (read_bytes > 0 && read_bytes == written_bytes);
            ret = pifs_fclose(file2);
        }
        else
        {
            PIFS_ERROR_MSG("Cannot open file '%s'\r\n", a_newname);
        }
        ret = pifs_fclose(file);
    }
    else
    {
        PIFS_ERROR_MSG("Cannot open file '%s'\r\n", a_oldname);
    }

    PIFS_SET_ERRNO(ret);
    return ret;
}

/**
 * @brief pifs_is_file_exist Check whether a file exist.
 * This is a non-standard file operation.
 *
 * @param[in] a_filename File name to be checked.
 * @return TRUE: if file exist. FALSE: file does not exist.
 */
bool_t pifs_is_file_exist(const pifs_char_t * a_filename)
{
    pifs_status_t    ret = PIFS_ERROR_NO_MORE_RESOURCE;
    bool_t           is_file_exist = FALSE;
    pifs_entry_t    * entry = &pifs.entry;

    PIFS_NOTICE_MSG("filename: '%s'\r\n", a_filename);

    ret = pifs_check_filename(a_filename);
    if (ret == PIFS_SUCCESS)
    {
        ret = pifs_find_entry(PIFS_FIND_ENTRY, a_filename, entry,
                              pifs.header.entry_list_address.block_address,
                              pifs.header.entry_list_address.page_address);
    }
    if (ret == PIFS_SUCCESS)
    {
        is_file_exist = TRUE;
    }

    return is_file_exist;
}

/**
 * @brief pifs_ferror Return last error of file.
 *
 * @param[in] a_file Pointer to file.
 * @return 0 (PIFS_SUCCESS): if no error occurred.
 */
int pifs_ferror(P_FILE * a_file)
{
    pifs_file_t * file = (pifs_file_t*) a_file;
    int ret = file->status;

    return ret;
}

/**
 * @brief pifs_feof Check end-of-file indicator.
 * @param[in] a_file Pointer to opened file.
 * @return 0: end-of-file reached, other: not end-of-file.
 */
int pifs_feof(P_FILE * a_file)
{
    pifs_file_t * file = (pifs_file_t*) a_file;
    int ret = file->read_pos == file->entry.file_size;

    return ret;
}

/**
 * @brief pifs_filesize Get size of a file.
 * @param[in] a_filename Pointer to the file name.
 * @return File size in bytes or -1 if file not found.
 */
long int pifs_filesize(const pifs_char_t * a_filename)
{
    long int filesize = -1;
    pifs_status_t status;
    pifs_entry_t  entry;

    status = pifs_find_entry(PIFS_FIND_ENTRY, a_filename, &entry,
                             pifs.header.entry_list_address.block_address,
                             pifs.header.entry_list_address.page_address);
    if (status == PIFS_SUCCESS)
    {
        filesize = entry.file_size;
    }

    return filesize;
}
