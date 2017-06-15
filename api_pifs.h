/**
 * @file        api_pifs.h
 * @brief       Pi file system API
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-15 14:30:11 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_API_PIFS_H_
#define _INCLUDE_API_PIFS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    PIFS_SUCCESS,
    PIFS_ERROR,
    PIFS_FLASH_INIT_ERROR,
} pifs_status_t;

typedef void P_FILE;

pifs_status_t pifs_init(void);
pifs_status_t pifs_delete(void);
P_FILE * pifs_fopen(const char * filename, const char modes);

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDE_API_PIFS_H_ */
