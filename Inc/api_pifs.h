/**
 * @file        api_pifs.h
 * @brief       Pi file system API
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-16 13:08:53 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_API_PIFS_H_
#define _INCLUDE_API_PIFS_H_

#include <stdint.h>
#include "pifs_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PIFS_EOF            (-1)

typedef enum
{
    PIFS_SUCCESS = 0,
    PIFS_ERROR_GENERAL = 1,
    PIFS_ERROR_NOT_INITIALIZED = 2,
    PIFS_ERROR_INVALID_OPEN_MODE = 3,
    PIFS_ERROR_INVALID_FILE_NAME = 4,
    PIFS_ERROR_FILE_NOT_FOUND = 5,
    PIFS_ERROR_NO_MORE_RESOURCE = 6,    /**< Not enough file handle/directory resource */
    PIFS_ERROR_NO_MORE_SPACE = 7,
    PIFS_ERROR_NO_MORE_ENTRY = 8,
    PIFS_ERROR_END_OF_FILE = 9,
    PIFS_ERROR_CONFIGURATION = 10,
    PIFS_ERROR_FLASH_INIT = 11,
    PIFS_ERROR_FLASH_WRITE = 12,
    PIFS_ERROR_FLASH_READ = 13,
    PIFS_ERROR_FLASH_ERASE = 14,
    PIFS_ERROR_FLASH_TIMEOUT = 15,
    PIFS_ERROR_FLASH_GENERAL = 16,
    PIFS_ERROR_INTERNAL_ALLOCATION = 17,
    PIFS_ERROR_INTERNAL_RANGE = 18,
    PIFS_ERROR_SEEK_NOT_POSSIBLE = 19,
} pifs_status_t;

typedef void P_FILE;
typedef enum
{
    PIFS_SEEK_SET = 1,
    PIFS_SEEK_CUR,
    PIFS_SEEK_END,
} pifs_fseek_origin_t;

typedef uint32_t pifs_ino_t;

struct pifs_dirent
{
    pifs_ino_t  d_ino;                          /**< Unique ID of the file */
    uint32_t    d_first_map_block_address;      /**< Block address of first map page */
    uint32_t    d_first_map_page_address;       /**< Page address of first map page */
    char        d_name[PIFS_FILENAME_LEN_MAX];  /**< File/directory name */
};

typedef struct pifs_dirent pifs_dirent_t;
typedef void * pifs_DIR;

extern int pifs_errno;

void pifs_print_fs_info(void);
void pifs_print_header_info(void);
void pifs_print_free_space_info(void);
pifs_status_t pifs_init(void);
pifs_status_t pifs_delete(void);
P_FILE * pifs_fopen(const pifs_char_t * a_filename, const pifs_char_t * a_modes);
size_t pifs_fwrite(const void * a_data, size_t a_size, size_t a_count, P_FILE * a_file);
size_t pifs_fread(void * a_data, size_t a_size, size_t a_count, P_FILE * a_file);
int pifs_fflush(P_FILE * a_file);
int pifs_fclose(P_FILE * a_file);
int pifs_fseek(P_FILE * a_file, long int a_offset, int a_origin);
void pifs_rewind(P_FILE * a_file);
long int pifs_ftell(P_FILE * a_file);
int pifs_remove(const pifs_char_t * a_filename);
int pifs_rename(const pifs_char_t * a_oldname, const pifs_char_t * a_newname);
int pifs_ferror(P_FILE * a_file);
long int pifs_filesize(const pifs_char_t * a_filename);
pifs_DIR * pifs_opendir(const char *a_name);
struct pifs_dirent * pifs_readdir(pifs_DIR *a_dirp);
int pifs_closedir(pifs_DIR *a_dirp);

pifs_status_t pifs_get_free_space(size_t * a_free_management_bytes,
                                  size_t * a_free_data_bytes,
                                  size_t * a_free_management_page_count,
                                  size_t * a_free_data_page_count);
pifs_status_t pifs_get_to_be_released_space(size_t * a_to_be_released_management_bytes,
                                            size_t * a_to_be_released_data_bytes,
                                            size_t * a_to_be_released_management_page_count,
                                            size_t * a_to_be_released_data_page_count);
#ifdef __cplusplus
}
#endif

#endif /* _INCLUDE_API_PIFS_H_ */
