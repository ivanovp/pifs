/**
 * @file        pifs.h
 * @brief       Internal header of Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-27 19:35:38 ivanovp {Time-stamp}
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

/******************************************************************************/
/*** FILE SYSTEM HEADER                                                     ***/
/******************************************************************************/
/** Size of file system's header in bytes */
#define PIFS_HEADER_SIZE_BYTE               (sizeof(pifs_header_t))
/** Size of file system's header in pages */
#define PIFS_HEADER_SIZE_PAGE               ((PIFS_HEADER_SIZE_BYTE + PIFS_FLASH_PAGE_SIZE_BYTE - 1) / PIFS_FLASH_PAGE_SIZE_BYTE)

/******************************************************************************/
/*** ENTRY LIST                                                             ***/
/******************************************************************************/
#define PIFS_ENTRY_SIZE_BYTE                (sizeof(pifs_entry_t))

/** Number of entries can fit in one page */
#define PIFS_ENTRY_PER_PAGE                 (PIFS_FLASH_PAGE_SIZE_BYTE / PIFS_ENTRY_SIZE_BYTE)

/** Size of entry list in pages */
#define PIFS_ENTRY_LIST_SIZE_PAGE           ((PIFS_ENTRY_NUM_MAX + PIFS_ENTRY_PER_PAGE - 1) / PIFS_ENTRY_PER_PAGE)
/** Size of entry list in bytes */
#define PIFS_ENTRY_LIST_SIZE_BYTE           (PIFS_ENTRY_LIST_SIZE_PAGE * PIFS_FLASH_PAGE_SIZE_BYTE)

/******************************************************************************/
/*** MAP ENTRY                                                              ***/
/******************************************************************************/
#define PIFS_MAP_HEADER_SIZE_BYTE           (sizeof(pifs_map_header_t))
#define PIFS_MAP_ENTRY_SIZE_BYTE            (sizeof(pifs_map_entry_t))

#define PIFS_MAP_ENTRY_PER_PAGE             ((PIFS_FLASH_PAGE_SIZE_BYTE - PIFS_MAP_HEADER_SIZE_BYTE) / PIFS_MAP_ENTRY_SIZE_BYTE)


/******************************************************************************/
/*** FREE SPACE BITMAP                                                      ***/
/******************************************************************************/
/** Size of free space bitmap in bytes.
 * Multiplied by two, because two bits are used per page */
#define PIFS_FREE_SPACE_BITMAP_SIZE_BYTE    (2 * ((PIFS_FLASH_PAGE_NUM_FS + 7) / 8))
/** Size of free space bitmap in pages */
#define PIFS_FREE_SPACE_BITMAP_SIZE_PAGE    ((PIFS_FREE_SPACE_BITMAP_SIZE_BYTE + PIFS_FLASH_PAGE_SIZE_BYTE - 1) / PIFS_FLASH_PAGE_SIZE_BYTE)

/******************************************************************************/
/*** DELTA PAGES                                                            ***/
/******************************************************************************/
//#define PIFS_DELTA_HEADER_SIZE_BYTE         (sizeof(pifs_delta_header_t))
#define PIFS_DELTA_ENTRY_SIZE_BYTE          (sizeof(pifs_delta_entry_t))

//#define PIFS_DELTA_ENTRY_PER_PAGE           ((PIFS_FLASH_PAGE_SIZE_BYTE - PIFS_DELTA_HEADER_SIZE_BYTE) / PIFS_DELTA_ENTRY_SIZE_BYTE)
#define PIFS_DELTA_ENTRY_PER_PAGE           (PIFS_FLASH_PAGE_SIZE_BYTE / PIFS_DELTA_ENTRY_SIZE_BYTE)

/******************************************************************************/

/** Number of bits in a byte */
#define PIFS_BYTE_BITS              8

#if PIFS_OPTIMIZE_FOR_RAM
#define PIFS_BOOL_SIZE              : 1
#else
#define PIFS_BOOL_SIZE
#endif

#define PIFS_MIN(a,b)               ((a) < (b) ? (a) : (b))
#define PIFS_MAX(a,b)               ((a) > (b) ? (a) : (b))

#if PIFS_CHECKSUM_SIZE == 1
typedef uint8_t pifs_checksum_t;
#elif PIFS_CHECKSUM_SIZE == 2
typedef uint16_t pifs_checksum_t;
#elif PIFS_CHECKSUM_SIZE == 4
typedef uint32_t pifs_checksum_t;
#else
#error PIFS_CHECKSUM_SIZE is invalid! Valid values are 1, 2 or 4.
#endif

#if PIFS_FLASH_PAGE_NUM_FS < 256
typedef uint8_t pifs_bit_pos_t;
#elif PIFS_FLASH_PAGE_NUM_FS < 65536
typedef uint16_t pifs_bit_pos_t;
#elif PIFS_FLASH_PAGE_NUM_FS < 4294967296l
typedef uint32_t pifs_bit_pos_t;
#endif

#ifndef PIFS_PAGE_COUNT_SIZE
#if PIFS_FLASH_PAGE_PER_BLOCK < 256
#define PIFS_PAGE_COUNT_SIZE        1
#elif PIFS_FLASH_PAGE_PER_BLOCK < 65536
#define PIFS_PAGE_COUNT_SIZE        2
#elif PIFS_FLASH_PAGE_PER_BLOCK < 4294967296l
#define PIFS_PAGE_COUNT_SIZE        4
#endif
#endif

/**
 * This type affects the map entry size and the page count/map entry.
 */
#if PIFS_PAGE_COUNT_SIZE == 1
typedef uint8_t pifs_page_count_t;
#define PIFS_PAGE_COUNT_INVALID (UINT8_MAX - 1)
#elif PIFS_PAGE_COUNT_SIZE == 2
typedef uint16_t pifs_page_count_t;
#define PIFS_PAGE_COUNT_INVALID (UINT16_MAX - 1)
#elif PIFS_PAGE_COUNT_SIZE == 4
typedef uint32_t pifs_page_count_t;
#define PIFS_PAGE_COUNT_INVALID (UINT32_MAX - 1)
#else
#error PIFS_PAGE_COUNT_SIZE is invalid! Valid values are 1, 2 or 4.
#endif

typedef enum
{
    /**
     * Management information stored in the block. Actual management block.
     */
    PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT = 1,
    /**
     * Management information stored in the block. Next management block.
     * Secondary management pages will be used when primary management area is
     * full.
     */
    PIFS_BLOCK_TYPE_SECONDARY_MANAGEMENT,
    /** Data stored in the block. */
    PIFS_BLOCK_TYPE_DATA,
    /** Reserved block */
    PIFS_BLOCK_TYPE_RESERVED
} pifs_block_type_t;

/**
 * Address of a page in flash memory.
 */
typedef struct PIFS_PACKED_ATTRIBUTE
{
    pifs_block_address_t    block_address;
    pifs_page_address_t     page_address;
} pifs_address_t;

/**
 * File system's header.
 */
typedef struct PIFS_PACKED_ATTRIBUTE
{
    uint32_t                magic;
#if ENABLE_PIFS_VERSION
    uint8_t                 majorVersion;
    uint8_t                 minorVersion;
#endif
    uint32_t                counter;
    pifs_block_address_t    management_blocks[PIFS_MANAGEMENT_BLOCKS];
    pifs_block_address_t    next_management_blocks[PIFS_MANAGEMENT_BLOCKS];
    pifs_address_t          free_space_bitmap_address;
    pifs_address_t          entry_list_address;
    pifs_address_t          delta_map_address;
    /** Checksum shall be the last element! */
    pifs_checksum_t         checksum;
} pifs_header_t;

/**
 * File or directory entry.
 */
typedef struct PIFS_PACKED_ATTRIBUTE
{
    uint8_t                 name[PIFS_FILENAME_LEN_MAX];
    uint8_t                 attrib;
//    pifs_object_id_t        object_id;          /***< FIXME not used */
    pifs_address_t          first_map_address;  /**< First map page's address */
    pifs_page_offset_t      last_page_size;     /**< Bytes written into last page */
} pifs_entry_t;

/**
 * Map's header.
 */
typedef struct PIFS_PACKED_ATTRIBUTE
{
    pifs_address_t          prev_map_address;
    pifs_address_t          next_map_address;
} pifs_map_header_t;

/**
 * Map of file's page.
 */
typedef struct PIFS_PACKED_ATTRIBUTE
{
    pifs_address_t          address;
    pifs_page_count_t       page_count;
} pifs_map_entry_t;

#if 0
/**
 * Delta pages' header.
 */
typedef struct PIFS_PACKED_ATTRIBUTE
{
    pifs_address_t          next_delta_address;
} pifs_delta_header_t;
#endif

/**
 * Delta page.
 */
typedef struct PIFS_PACKED_ATTRIBUTE
{
    pifs_address_t          delta_address;
    pifs_address_t          orig_address;
} pifs_delta_entry_t;

/**
 * Actual status and parameters of an opened file.
 */
typedef struct
{
    bool_t                  is_used PIFS_BOOL_SIZE;
    bool_t                  is_opened PIFS_BOOL_SIZE;
    bool_t                  mode_create_new_file PIFS_BOOL_SIZE;
    bool_t                  mode_read PIFS_BOOL_SIZE;
    bool_t                  mode_write PIFS_BOOL_SIZE;
    bool_t                  mode_append PIFS_BOOL_SIZE;
    bool_t                  mode_file_shall_exist PIFS_BOOL_SIZE;
    pifs_entry_t            entry;              /**< One element of entry list */
    pifs_status_t           status;             /**< Last file operation's result */
    pifs_address_t          actual_map_address; /**< Actual map's address used for reading */
    pifs_map_header_t       map_header;         /**< Actual map's header */
    size_t                  map_entry_idx;      /**< Actual entry's index in the map */
    pifs_map_entry_t        map_entry;          /**< Actual entry in the map */
    size_t                  read_pos;           /**< Position in file after last read */
    pifs_address_t          read_address;       /**< Last read page's address */
    pifs_page_count_t       read_page_count;    /**< Page count to be read from 'read_address' */
    size_t                  write_pos;          /**< Position in file after last write */
    pifs_address_t          write_address;      /**< Last written page's address */
} pifs_file_t;

/**
 * Actual status of file system.
 */
typedef struct
{
    pifs_address_t          header_address;
    bool_t                  is_header_found;
    pifs_header_t           header;
//    pifs_object_id_t        latest_object_id;
    pifs_address_t          cache_page_buf_address;                       /**< Address of cache_page_buf */
    uint8_t                 cache_page_buf[PIFS_FLASH_PAGE_SIZE_BYTE];    /**< Flash page buffer for cache */
    bool_t                  cache_page_buf_is_dirty;
    pifs_file_t             file[PIFS_OPEN_FILE_NUM_MAX];                 /**< Opened files */
    uint8_t                 delta_map_page_buf[PIFS_DELTA_MAP_PAGE_NUM][PIFS_FLASH_PAGE_SIZE_BYTE];
    bool_t                  delta_map_page_is_read PIFS_BOOL_SIZE;
    bool_t                  delta_map_page_is_dirty PIFS_BOOL_SIZE;
    size_t                  delta_map_entry_count;
    uint8_t                 page_buf[PIFS_FLASH_PAGE_SIZE_BYTE];           /**< Flash page buffer */
} pifs_t;

#endif /* _INCLUDE_PIFS_H_ */
