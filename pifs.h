/**
 * @file        pifs.h
 * @brief       Internal header of Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-15 14:51:24 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_PIFS_H_
#define _INCLUDE_PIFS_H_

#include <stdint.h>
#include "flash.h"
#include "pifs_config.h"

#define PIFS_MAGIC              0x50494653u  /* PIFS */

#define ENABLE_PIFS_VERSION     1
#define PIFS_MAJOR_VERSION      1u
#define PIFS_MINOR_VERSION      0u

#define PIFS_ATTRIB_READONLY    0x01u
#define PIFS_ATTRIB_HIDDEN      0x02u
#define PIFS_ATTRIB_SYSTEM      0x04u
#define PIFS_ATTRIB_DIR         0x10u
#define PIFS_ATTRIB_ARCHIVE     0x20u
#define PIFS_ATTRIB_DELETED     0x80u

typedef uint8_t checksum_t;

typedef struct PIFS_PACKED_ATTRIBUTE
{
    uint32_t    magic;
#if ENABLE_PIFS_VERSION
    uint8_t     majorVersion;
    uint8_t     minorVersion;
#endif
    uint32_t    counter;
    checksum_t  checksum;
} pifs_header_t;

typedef struct PIFS_PACKED_ATTRIBUTE
{
    uint8_t         name[PIFS_FILENAME_LEN_MAX];
    uint8_t         attrib;
    object_id_t     object_id;
    block_address_t block_address;
    page_address_t  page_address;
} pifs_entry_t;

#endif /* _INCLUDE_PIFS_H_ */
