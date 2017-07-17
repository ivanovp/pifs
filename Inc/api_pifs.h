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
    PIFS_SUCCESS,
    PIFS_ERROR_GENERAL,
    PIFS_ERROR_NOT_INITIALIZED,
    PIFS_ERROR_INVALID_OPEN_MODE,
    PIFS_ERROR_INVALID_FILE_NAME,
    PIFS_ERROR_FILE_NOT_FOUND,
    PIFS_ERROR_NO_MORE_RESOURCE,    /**< Not enough file handle/directory resource */
    PIFS_ERROR_NO_MORE_SPACE,
    PIFS_ERROR_NO_MORE_ENTRY,
    PIFS_ERROR_END_OF_FILE,
    PIFS_ERROR_CONFIGURATION,
    PIFS_ERROR_FLASH_INIT,
    PIFS_ERROR_FLASH_WRITE,
    PIFS_ERROR_FLASH_READ,
    PIFS_ERROR_FLASH_ERASE,
    PIFS_ERROR_FLASH_TIMEOUT,
    PIFS_ERROR_FLASH_GENERAL,
    PIFS_ERROR_INTERNAL_ALLOCATION,
    PIFS_ERROR_INTERNAL_RANGE,
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
int pifs_fclose(P_FILE * a_file);
int pifs_fseek(P_FILE * a_file, long int a_offset, int a_origin);
void pifs_rewind(P_FILE * a_file);
long int pifs_ftell(P_FILE * a_file);
int pifs_remove(const pifs_char_t * a_filename);
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
