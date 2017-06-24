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

#define pfopen  pifs_open
#define pfwrite pifs_write
#define pfread  pifs_read
#define pfclose pifs_close

typedef enum
{
    PIFS_SUCCESS,
    PIFS_ERROR,
    PIFS_ERROR_INVALID_OPEN_MODE,
    PIFS_ERROR_ENTRY_NOT_FOUND,
    PIFS_ERROR_NO_MORE_SPACE,
    PIFS_ERROR_END_OF_FILE,
    PIFS_ERROR_CONFIGURATION,
    PIFS_ERROR_FLASH_INIT,
    PIFS_ERROR_FLASH_WRITE,
    PIFS_ERROR_FLASH_READ,
    PIFS_ERROR_FLASH_ERASE,
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

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDE_API_PIFS_H_ */
