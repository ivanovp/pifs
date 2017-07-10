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

#ifdef __cplusplus
extern "C" {
#endif

#define pfopen      pifs_open
#define pfwrite     pifs_write
#define pfread      pifs_read
#define pfclose     pifs_close

#define PIFS_EOF    (-1)

typedef enum
{
    PIFS_SUCCESS,
    PIFS_ERROR,
    PIFS_ERROR_NOT_INITIALIZED,
    PIFS_ERROR_INVALID_OPEN_MODE,
    PIFS_ERROR_NO_MORE_FILE_HANDLE,
    PIFS_ERROR_INVALID_FILE_NAME,
    PIFS_ERROR_FILE_NOT_FOUND,
    PIFS_ERROR_NO_MORE_SPACE,
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

pifs_status_t pifs_init(void);
pifs_status_t pifs_delete(void);
P_FILE * pifs_fopen(const char * a_filename, const char * a_modes);
size_t pifs_fwrite(const void * a_data, size_t a_size, size_t a_count, P_FILE * a_file);
size_t pifs_fread(void * a_data, size_t a_size, size_t a_count, P_FILE * a_file);
int pifs_fclose(P_FILE * a_file);
pifs_status_t pifs_get_free_space(size_t * a_free_management_bytes,
                                  size_t * a_free_data_bytes,
                                  size_t * a_free_management_page_count,
                                  size_t * a_free_data_page_count);
int pifs_remove(const char * a_filename);
int pifs_ferror(P_FILE * a_file);
#ifdef __cplusplus
}
#endif

#endif /* _INCLUDE_API_PIFS_H_ */
