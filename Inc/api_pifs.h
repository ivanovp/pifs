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
#include "flash_config.h"
#include "pifs_config.h"
#include "common.h"

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
    PIFS_ERROR_FILE_ALREADY_EXIST = 6,
    PIFS_ERROR_NO_MORE_RESOURCE = 7,    /**< Not enough file handle/directory resource */
    PIFS_ERROR_NO_MORE_SPACE = 8,
    PIFS_ERROR_NO_MORE_ENTRY = 9,
    PIFS_ERROR_END_OF_FILE = 10,
    PIFS_ERROR_CONFIGURATION = 11,
    PIFS_ERROR_FLASH_INIT = 12,
    PIFS_ERROR_FLASH_WRITE = 13,
    PIFS_ERROR_FLASH_READ = 14,
    PIFS_ERROR_FLASH_ERASE = 15,
    PIFS_ERROR_FLASH_TIMEOUT = 16,
    PIFS_ERROR_FLASH_GENERAL = 17,
    PIFS_ERROR_INTERNAL_ALLOCATION = 18,
    PIFS_ERROR_INTERNAL_RANGE = 19,
    PIFS_ERROR_SEEK_NOT_POSSIBLE = 20,
    PIFS_ERROR_NO_A_DIRECTORY = 21,
    PIFS_ERROR_IS_DIRECTORY = 22,
    PIFS_ERROR_IS_NOT_DIRECTORY = 23,
    PIFS_ERROR_DIRECTORY_NOT_EMPTY = 24,
    PIFS_ERROR_INTEGRITY = 25,
} pifs_status_t;

#define PIFS_EACCES     PIFS_ERROR_FILE_NOT_FOUND
#define PIFS_EEXIST     PIFS_ERROR_FILE_ALREADY_EXIST
#define PIFS_ENOTDIR    PIFS_ERROR_NO_A_DIRECTORY
#define PIFS_EISDIR     PIFS_ERROR_IS_A_DIRECTORY
#define PIFS_EMFILE     PIFS_ERROR_NO_MORE_RESOURCE
#define PIFS_ENOSPC     PIFS_ERROR_NO_MORE_SPACE
#define PIFS_ESPIPE     PIFS_ERROR_SEEK_NOT_POSSIBLE
#define PIFS_ENOTEMPTY  PIFS_ERROR_DIRECTORY_NOT_EMPTY

#define PIFS_ATTRIB_READONLY                0x01u
#define PIFS_ATTRIB_HIDDEN                  0x02u
#define PIFS_ATTRIB_SYSTEM                  0x04u
#define PIFS_ATTRIB_DIR                     0x10u
#define PIFS_ATTRIB_ARCHIVE                 0x20u
#define PIFS_ATTRIB_DELETED                 0x80u
#define PIFS_ATTRIB_ALL                     UINT8_MAX

#if PIFS_FLASH_ERASED_BYTE_VALUE == 0xFF
/* If erased value is 0xFF, invert attribute bits. */
/* Therefore bits can be updated in the existing entry, no new entry is needed. */
#define PIFS_INVERT_ATTRIBUTE_BITS      1
#else
#define PIFS_INVERT_ATTRIBUTE_BITS      0
#endif

#if PIFS_INVERT_ATTRIBUTE_BITS
#define PIFS_IS_READONLY(attrib)        (!((attrib) & PIFS_ATTRIB_READONLY))
#define PIFS_IS_HIDDEN(attrib)          (!((attrib) & PIFS_ATTRIB_HIDDEN))
#define PIFS_IS_SYSTEM(attrib)          (!((attrib) & PIFS_ATTRIB_SYSTEM))
#define PIFS_IS_DIR(attrib)             (!((attrib) & PIFS_ATTRIB_DIR))
#define PIFS_IS_ARCHIVE(attrib)         (!((attrib) & PIFS_ATTRIB_ARCHIVE))
#define PIFS_IS_DELETED(attrib)         (!((attrib) & PIFS_ATTRIB_DELETED))
#define PIFS_SET_ATTRIB(attrib,bits)    do { \
        (attrib) &= ~(bits); \
    } while (0)
#define PIFS_CLEAR_ATTRIB(attrib,bits)  do { \
        (attrib) |= (bits); \
    } while (0)
#else
#define PIFS_IS_READONLY(attrib)        ((attrib) & PIFS_ATTRIB_READONLY)
#define PIFS_IS_HIDDEN(attrib)          ((attrib) & PIFS_ATTRIB_HIDDEN)
#define PIFS_IS_SYSTEM(attrib)          ((attrib) & PIFS_ATTRIB_SYSTEM)
#define PIFS_IS_DIR(attrib)             ((attrib) & PIFS_ATTRIB_DIR)
#define PIFS_IS_ARCHIVE(attrib)         ((attrib) & PIFS_ATTRIB_ARCHIVE)
#define PIFS_IS_DELETED(attrib)         ((attrib) & PIFS_ATTRIB_DELETED)
#define PIFS_SET_ATTRIB(attrib,bits)  do { \
        (attrib) |= (bits); \
    } while (0)
#define PIFS_CLEAR_ATTRIB(attrib,bits)    do { \
        (attrib) &= ~(bits); \
    } while (0)
#endif

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
    pifs_ino_t       d_ino;                          /**< Unique ID of the file */
    char             d_name[PIFS_FILENAME_LEN_MAX];  /**< File/directory name */
    /* Non-standard fields */
    size_t           d_filesize;
    uint8_t          d_attrib;
    uint32_t         d_first_map_block_address;      /**< Block address of first map page */
    uint32_t         d_first_map_page_address;       /**< Page address of first map page */
#if PIFS_ENABLE_USER_DATA
    pifs_user_data_t d_user_data;
#endif
};

typedef struct pifs_dirent pifs_dirent_t;
typedef void * pifs_DIR;

extern int pifs_errno;

void pifs_print_fs_info(void);
void pifs_print_header_info(void);
void pifs_print_free_space_info(void);
pifs_status_t pifs_init(void);
pifs_status_t pifs_delete(void);
pifs_status_t pifs_check(void);
P_FILE * pifs_fopen(const pifs_char_t * a_filename, const pifs_char_t * a_modes);
P_FILE * pifs_tmpfile( void );
pifs_char_t * pifs_tmpnam(pifs_char_t * a_str);
pifs_char_t * pifs_tmpnamn(pifs_char_t * a_str, size_t a_size);
size_t pifs_fwrite(const void * a_data, size_t a_size, size_t a_count, P_FILE * a_file);
size_t pifs_fread(void * a_data, size_t a_size, size_t a_count, P_FILE * a_file);
int pifs_fflush(P_FILE * a_file);
int pifs_fclose(P_FILE * a_file);
int pifs_fseek(P_FILE * a_file, long int a_offset, int a_origin);
void pifs_rewind(P_FILE * a_file);
long int pifs_ftell(P_FILE * a_file);
#if PIFS_ENABLE_USER_DATA
int pifs_fgetuserdata(P_FILE * a_file, pifs_user_data_t * a_user_data);
int pifs_fsetuserdata(P_FILE * a_file, const pifs_user_data_t * a_user_data);
#endif
int pifs_remove(const pifs_char_t * a_filename);
int pifs_rename(const pifs_char_t * a_oldname, const pifs_char_t * a_newname);
int pifs_copy(const pifs_char_t * a_oldname, const pifs_char_t * a_newname);
bool_t pifs_is_file_exist(const pifs_char_t * a_filename);
int pifs_ferror(P_FILE * a_file);
int pifs_feof(P_FILE * a_file);
long int pifs_filesize(const pifs_char_t * a_filename);
pifs_DIR * pifs_opendir(const pifs_char_t * a_name);
struct pifs_dirent * pifs_readdir(pifs_DIR * a_dirp);
int pifs_closedir(pifs_DIR * const a_dirp);
#if PIFS_ENABLE_DIRECTORIES
int pifs_mkdir(const pifs_char_t * const a_filename);
int pifs_rmdir(const pifs_char_t * const a_filename);
int pifs_chdir(const pifs_char_t * const a_filename);
pifs_char_t * pifs_getcwd(pifs_char_t * a_buffer, size_t a_size);
pifs_char_t * pifs_append_path(pifs_char_t * a_path1, size_t a_size, pifs_char_t * a_path2);
#endif
#if 0 //PIFS_ENABLE_LINKS
/* Hard link */
int pifs_link(const pifs_char_t * a_oldname, const pifs_char_t * a_newname);
/* Sym link */
int pifs_symlink(const pifs_char_t * a_oldname, const pifs_char_t * a_newname);
size_t pifs_readlink (const pifs_char_t * a_filename, pifs_char_t * a_buffer, size_t a_size);
#endif

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
