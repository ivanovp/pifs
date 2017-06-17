/**
 * @file        pifs.h
 * @brief       Internal header of Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-16 12:44:31 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_PIFS_H_
#define _INCLUDE_PIFS_H_

#include <stdint.h>

#include "common.h"
#include "flash.h"
#include "pifs_config.h"

// FIXME handle endianness?
//#define PIFS_MAGIC              0x50494653u  /* PIFS */
#define PIFS_MAGIC              0x53464950u  /* PIFS */
#if PIFS_MAGIC == 0
#error PIFS_MAGIC shall not be zero!
#elif PIFS_MAGIC == 0xFFFFFFFF
#error PIFS_MAGIC shall not be 0xFFFFFFFF!
#endif

#define ENABLE_PIFS_VERSION     1
#define PIFS_MAJOR_VERSION      1u
#define PIFS_MINOR_VERSION      0u

#define PIFS_ATTRIB_READONLY    0x01u
#define PIFS_ATTRIB_HIDDEN      0x02u
#define PIFS_ATTRIB_SYSTEM      0x04u
#define PIFS_ATTRIB_DIR         0x10u
#define PIFS_ATTRIB_ARCHIVE     0x20u
#define PIFS_ATTRIB_DELETED     0x80u

/** Two bits are used per page */
#define PIFS_FREE_SPACE_BITMAP_SIZE_BYTE    (2 * ((PIFS_FLASH_PAGE_FS_NUM + 7) / 8))
/** Size of file system's header + entry list in bytes */
#define PIFS_HEADER_ENTRY_LIST_SIZE_BYTE    (sizeof(pifs_header_t) + (PIFS_OBJECT_NUM_MAX * sizeof(pifs_entry_t)))
/** Size of file system's header + entry list in pages */
#define PIFS_HEADER_ENTRY_LIST_SIZE_PAGE    ((PIFS_HEADER_ENTRY_LIST_SIZE_BYTE + PIFS_FLASH_PAGE_SIZE_BYTE - 1u) / PIFS_FLASH_PAGE_SIZE_BYTE)

#define PIFS_ERASED_VALUE       0xFFu

#if PIFS_CHECKSUM_SIZE == 1
typedef uint8_t pifs_checksum_t;
#elif PIFS_CHECKSUM_SIZE == 2
typedef uint16_t checksum_t;
#elif PIFS_CHECKSUM_SIZE == 4
typedef uint32_t checksum_t;
#else
#error PIFS_CHECKSUM_SIZE is invalid! Valid values are 1, 2 or 4.
#endif

typedef enum
{
    /** Only management information stored in the block. */
    PIFS_BLOCK_TYPE_MANAGEMENT_BLOCK = 1,
    /** Data and management information stored as well. */
    PIFS_BLOCK_TYPE_HEADER_MIXED_BLOCK
} pifs_block_type_t;

typedef struct PIFS_PACKED_ATTRIBUTE
{
    uint32_t                magic;
#if ENABLE_PIFS_VERSION
    uint8_t                 majorVersion;
    uint8_t                 minorVersion;
#endif
    uint32_t                counter;
    pifs_block_type_t       block_type;
    pifs_block_address_t    free_space_bitmap_block_address;
    pifs_page_address_t     free_space_bitmap_page_address;
    /** Checksum shall be the last element! */
    pifs_checksum_t         checksum;
} pifs_header_t;

typedef struct PIFS_PACKED_ATTRIBUTE
{
    uint8_t                 bitmap[PIFS_FREE_SPACE_BITMAP_SIZE_BYTE];
} pifs_free_space_bitmap_t;

typedef struct PIFS_PACKED_ATTRIBUTE
{
    uint8_t                 name[PIFS_FILENAME_LEN_MAX];
    uint8_t                 attrib;
    pifs_object_id_t        object_id;
    pifs_block_address_t    block_address;
    pifs_page_address_t     page_address;
} pifs_entry_t;

typedef struct
{
    pifs_object_id_t        latest_object_id;
    pifs_block_address_t    mgmt_block_address;
    pifs_page_address_t     mgmt_page_address;
    bool_t                  is_mgmt_found;
    pifs_header_t           header;
    uint8_t                 page_buf[PIFS_FLASH_PAGE_SIZE_BYTE];
} pifs_t;

#endif /* _INCLUDE_PIFS_H_ */
