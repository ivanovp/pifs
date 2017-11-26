/**
 * @file        pifs.h
 * @brief       Internal header of Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-11-16 19:09:07 ivanovp {Time-stamp}
 * Licence:     GPL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _INCLUDE_PIFS_H_
#define _INCLUDE_PIFS_H_

#include <stdint.h>

#include "common.h"
#include "api_pifs.h"
#include "flash.h"
#include "pifs_config.h"
#if PIFS_ENABLE_OS
#include "pifs_os.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// NOTE handle endianness?
//#define PIFS_MAGIC                        0x50494653u  /* PIFS */
#define PIFS_MAGIC                          0x53464950u  /* PIFS */
#if PIFS_MAGIC == 0
#error PIFS_MAGIC shall not be zero!
#elif PIFS_MAGIC == 0xFFFFFFFF
#error PIFS_MAGIC shall not be 0xFFFFFFFF!
#endif

#define PIFS_ENABLE_VERSION                 1
#define PIFS_MAJOR_VERSION                  1u
#define PIFS_MINOR_VERSION                  0u

#define PIFS_ENABLE_ATTRIBUTES              1u   /**< 1: Use attribute field of files, 0: don't use attribute field */

/** Number of bits in a byte */
#define PIFS_BYTE_BITS                      8

#define PIFS_MAP_PAGE_NUM                   1   /**< Number of map pages. Fixed to 1! */
#define PIFS_FSBM_BITS_PER_PAGE_SHIFT       1   /**< 1 => 2 bits are used per page */
#define PIFS_FSBM_BITS_PER_PAGE             (1u << PIFS_FSBM_BITS_PER_PAGE_SHIFT)

#define PIFS_FLASH_PAGE_PER_LOGICAL_PAGE    (PIFS_LOGICAL_PAGE_SIZE_BYTE / PIFS_FLASH_PAGE_SIZE_BYTE)
#define PIFS_LOGICAL_PAGE_PER_BLOCK         (PIFS_FLASH_BLOCK_SIZE_BYTE / PIFS_LOGICAL_PAGE_SIZE_BYTE)
/** Number of all logical pages */
#define PIFS_LOGICAL_PAGE_NUM_ALL           (PIFS_FLASH_BLOCK_NUM_ALL * PIFS_LOGICAL_PAGE_PER_BLOCK)
/** Number of logical pages used by the file system */
#define PIFS_LOGICAL_PAGE_NUM_FS            (PIFS_FLASH_BLOCK_NUM_FS * PIFS_LOGICAL_PAGE_PER_BLOCK)

/******************************************************************************/
/*** FILE SYSTEM HEADER                                                     ***/
/******************************************************************************/
/** Size of file system's header in bytes */
#define PIFS_HEADER_SIZE_BYTE               (sizeof(pifs_header_t))
/** Size of file system's header in pages */
#define PIFS_HEADER_SIZE_PAGE               ((PIFS_HEADER_SIZE_BYTE + PIFS_LOGICAL_PAGE_SIZE_BYTE - 1) / PIFS_LOGICAL_PAGE_SIZE_BYTE)

/******************************************************************************/
/*** ENTRY LIST                                                             ***/
/******************************************************************************/
#define PIFS_ENTRY_SIZE_BYTE                (sizeof(pifs_entry_t))

/** Number of entries can fit in one page */
#define PIFS_ENTRY_PER_PAGE                 (PIFS_LOGICAL_PAGE_SIZE_BYTE / PIFS_ENTRY_SIZE_BYTE)

/** Size of entry list in pages */
#define PIFS_ENTRY_LIST_SIZE_PAGE           ((PIFS_ENTRY_NUM_MAX + PIFS_ENTRY_PER_PAGE - 1) / PIFS_ENTRY_PER_PAGE)
/** Size of entry list in bytes */
#define PIFS_ENTRY_LIST_SIZE_BYTE           (PIFS_ENTRY_LIST_SIZE_PAGE * PIFS_LOGICAL_PAGE_SIZE_BYTE)

/******************************************************************************/
/*** MAP ENTRY                                                              ***/
/******************************************************************************/
#define PIFS_MAP_HEADER_SIZE_BYTE           (sizeof(pifs_map_header_t))
#define PIFS_MAP_ENTRY_SIZE_BYTE            (sizeof(pifs_map_entry_t))

#define PIFS_MAP_ENTRY_PER_PAGE             ((PIFS_LOGICAL_PAGE_SIZE_BYTE - PIFS_MAP_HEADER_SIZE_BYTE) / PIFS_MAP_ENTRY_SIZE_BYTE)


/******************************************************************************/
/*** FREE SPACE BITMAP                                                      ***/
/******************************************************************************/
/** Size of free space bitmap in bytes.
 * Multiplied by two, because two bits are used per page */
#define PIFS_FREE_SPACE_BITMAP_SIZE_BYTE    (PIFS_FSBM_BITS_PER_PAGE * ((PIFS_LOGICAL_PAGE_NUM_FS + PIFS_BYTE_BITS - 1) / PIFS_BYTE_BITS))
/** Size of free space bitmap in pages */
#define PIFS_FREE_SPACE_BITMAP_SIZE_PAGE    ((PIFS_FREE_SPACE_BITMAP_SIZE_BYTE + PIFS_LOGICAL_PAGE_SIZE_BYTE - 1) / PIFS_LOGICAL_PAGE_SIZE_BYTE)

/******************************************************************************/
/*** DELTA PAGES                                                            ***/
/******************************************************************************/
#define PIFS_DELTA_ENTRY_SIZE_BYTE          (sizeof(pifs_delta_entry_t))
#define PIFS_DELTA_ENTRY_PER_PAGE           (PIFS_LOGICAL_PAGE_SIZE_BYTE / PIFS_DELTA_ENTRY_SIZE_BYTE)

/******************************************************************************/
/*** WEAR LEVEL LIST                                                        ***/
/******************************************************************************/
#define PIFS_WEAR_LEVEL_ENTRY_SIZE_BYTE     (sizeof(pifs_wear_level_entry_t))
#define PIFS_WEAR_LEVEL_ENTRY_PER_PAGE      (PIFS_LOGICAL_PAGE_SIZE_BYTE / PIFS_WEAR_LEVEL_ENTRY_SIZE_BYTE)
#define PIFS_WEAR_LEVEL_LIST_SIZE_BYTE      (PIFS_WEAR_LEVEL_ENTRY_SIZE_BYTE * PIFS_FLASH_BLOCK_NUM_FS)
#define PIFS_WEAR_LEVEL_LIST_SIZE_PAGE      ((PIFS_FLASH_BLOCK_NUM_FS + PIFS_WEAR_LEVEL_ENTRY_PER_PAGE - 1)/ PIFS_WEAR_LEVEL_ENTRY_PER_PAGE)

#define PIFS_MAP_PAGE_NUM_RECOMM            (((PIFS_LOGICAL_PAGE_NUM_FS - PIFS_MANAGEMENT_BLOCK_NUM * PIFS_LOGICAL_PAGE_PER_BLOCK) * PIFS_MAP_ENTRY_SIZE_BYTE + PIFS_LOGICAL_PAGE_SIZE_BYTE - 1) / PIFS_LOGICAL_PAGE_SIZE_BYTE)

#define PIFS_MANAGEMENT_PAGE_NUM_MIN        (PIFS_HEADER_SIZE_PAGE + PIFS_ENTRY_LIST_SIZE_PAGE + PIFS_FREE_SPACE_BITMAP_SIZE_PAGE + PIFS_DELTA_MAP_PAGE_NUM + PIFS_WEAR_LEVEL_LIST_SIZE_PAGE)
#define PIFS_MANAGEMENT_BLOCK_NUM_MIN       ((PIFS_MANAGEMENT_PAGE_NUM_MIN + PIFS_LOGICAL_PAGE_PER_BLOCK - 1) / PIFS_LOGICAL_PAGE_PER_BLOCK)
#define PIFS_MANAGEMENT_PAGE_NUM_RECOMM     (PIFS_MANAGEMENT_PAGE_NUM_MIN + PIFS_MAP_PAGE_NUM_RECOMM)
#define PIFS_MANAGEMENT_BLOCK_NUM_RECOMM    ((PIFS_MANAGEMENT_PAGE_NUM_RECOMM + PIFS_LOGICAL_PAGE_PER_BLOCK - 1) / PIFS_LOGICAL_PAGE_PER_BLOCK)
/******************************************************************************/

#if PIFS_ENABLE_USER_DATA
#define PIFS_USER_DATA_SIZE_BYTE            (sizeof(pifs_user_data_t))
#else
#define PIFS_USER_DATA_SIZE_BYTE            0
#endif

#if PIFS_OPTIMIZE_FOR_RAM
#define PIFS_BOOL_SIZE                      : 1
#else
#define PIFS_BOOL_SIZE
#endif

#define PIFS_MIN(a,b)                       ((a) < (b) ? (a) : (b))
#define PIFS_MAX(a,b)                       ((a) > (b) ? (a) : (b))

#define PIFS_FILENAME_TEMP_CHAR             '%'
#define PIFS_FILENAME_TEMP_STR              "%"
#define PIFS_ROOT_CHAR                      PIFS_PATH_SEPARATOR_CHAR
#if PIFS_PATH_SEPARATOR_CHAR == '/'
#define PIFS_PATH_SEPARATOR_STR             "/"
#define PIFS_ROOT_STR                       "/"
#else
#define PIFS_PATH_SEPARATOR_STR             "\\"
#define PIFS_ROOT_STR                       "\\"
#endif
#define PIFS_DOT_CHAR                       '.'
#define PIFS_DOT_STR                        "."
#define PIFS_DOUBLE_DOT_STR                 ".."
#define PIFS_EOS                            0

#if PIFS_LOGICAL_PAGE_SIZE_BYTE < PIFS_FLASH_PAGE_SIZE_BYTE
#error PIFS_LOGICAL_PAGE_SIZE_BYTE shall be larger or equatl to PIFS_FLASH_PAGE_SIZE_BYTE!
#endif

#if PIFS_LOGICAL_PAGE_SIZE_BYTE & (PIFS_LOGICAL_PAGE_SIZE_BYTE - 1) != 0
#error PIFS_LOGICAL_PAGE_SIZE_BYTE shall be power of two!
#endif

/* Check if logical page size is equal to flash (physical) page size */
#if PIFS_LOGICAL_PAGE_SIZE_BYTE != PIFS_FLASH_PAGE_SIZE_BYTE
#define PIFS_LOGICAL_PAGE_ENABLED   1
#else
#define PIFS_LOGICAL_PAGE_ENABLED   0
#endif

#if PIFS_LOGICAL_PAGE_ENABLED
#define PIFS_LOGICAL_PAGE_IDX(idx)  (idx)
#else
#define PIFS_LOGICAL_PAGE_IDX(idx)  (0)
#endif

/** Convert from logical page to flash page */
#define PIFS_LP2FP(lp)              ((lp) * PIFS_FLASH_PAGE_PER_LOGICAL_PAGE)
/** Convert from flash page to logical page */
#define PIFS_FP2LP(fp)              ((fp) / PIFS_FLASH_PAGE_PER_LOGICAL_PAGE)

#if PIFS_CHECKSUM_SIZE == 1 || PIFS_ENABLE_CRC
typedef uint8_t pifs_checksum_t;
#define PIFS_CHECKSUM_ERASED    (UINT8_MAX)
#elif PIFS_CHECKSUM_SIZE == 2
typedef uint16_t pifs_checksum_t;
#define PIFS_CHECKSUM_ERASED    (UINT16_MAX)
#elif PIFS_CHECKSUM_SIZE == 4
typedef uint32_t pifs_checksum_t;
#define PIFS_CHECKSUM_ERASED    (UINT32_MAX)
#else
#error PIFS_CHECKSUM_SIZE is invalid! Valid values are 1, 2 or 4.
#endif

#define PIFS_CHECKSUM_SIZE_BYTE     (sizeof(pifs_checksum_t))

#if PIFS_FLASH_PAGE_NUM_FS < 256
typedef uint8_t pifs_bit_pos_t;
#elif PIFS_FLASH_PAGE_NUM_FS < 65536
typedef uint16_t pifs_bit_pos_t;
#elif PIFS_FLASH_PAGE_NUM_FS < 4294967296l
typedef uint32_t pifs_bit_pos_t;
#endif

typedef size_t pifs_size_t;
typedef uint32_t pifs_file_size_t;
#define PIFS_FILE_SIZE_ERASED       (UINT32_MAX)

/**
 * This type affects the map entry size and the page count/map entry.
 */
#if PIFS_MAP_PAGE_COUNT_SIZE == 1
typedef uint8_t pifs_map_page_count_t;
#define PIFS_MAP_PAGE_COUNT_INVALID (UINT8_MAX - 1)
#elif PIFS_MAP_PAGE_COUNT_SIZE == 2
typedef uint16_t pifs_map_page_count_t;
#define PIFS_MAP_PAGE_COUNT_INVALID (UINT16_MAX - 1)
#elif PIFS_MAP_PAGE_COUNT_SIZE == 4
typedef uint32_t pifs_map_page_count_t;
#define PIFS_MAP_PAGE_COUNT_INVALID (UINT32_MAX - 1)
#else
#error PIFS_MAP_PAGE_COUNT_SIZE is invalid! Valid values are 1, 2 or 4.
#endif

#if PIFS_FLASH_PAGE_NUM_FS < 255
typedef uint8_t pifs_page_count_t;
#define PIFS_PAGE_COUNT_INVALID (UINT8_MAX - 1)
typedef uint16_t pifs_page_count_t;
#define PIFS_PAGE_COUNT_INVALID (UINT16_MAX - 1)
#elif PIFS_FLASH_PAGE_NUM_FS < 4294967295l
typedef uint32_t pifs_page_count_t;
#define PIFS_PAGE_COUNT_INVALID (UINT32_MAX - 1)
#endif

typedef uint16_t pifs_wear_level_cntr_t;
#define PIFS_WEAR_LEVEL_CNTR_MAX    (UINT16_MAX)

typedef uint8_t pifs_wear_level_bits_t;
#define PIFS_WEAR_LEVEL_BITS_ERASED (UINT8_MAX)

#if PIFS_MANAGEMENT_BLOCK_NUM >= PIFS_FLASH_BLOCK_NUM_FS
#error PIFS_MANAGEMENT_BLOCK_NUM is greater than PIFS_FLASH_BLOCK_NUM_FS!
#endif
#if PIFS_MANAGEMENT_BLOCK_NUM > PIFS_FLASH_BLOCK_NUM_FS / 4
#warning PIFS_MANAGEMENT_BLOCK_NUM is larger than PIFS_FLASH_BLOCK_NUM_FS / 4!
#endif
#if PIFS_MANAGEMENT_BLOCK_NUM < 1
#error PIFS_MANAGEMENT_BLOCK_NUM shall be 1 at minimum!
#endif
#if PIFS_LEAST_WEARED_BLOCK_NUM < 1
#error PIFS_LEAST_WEARED_BLOCK_NUM shall be 1 at minimum!
#endif
#if PIFS_LEAST_WEARED_BLOCK_NUM > PIFS_FLASH_BLOCK_NUM_FS - PIFS_MANAGEMENT_BLOCK_NUM * 2
#error PIFS_LEAST_WEARED_BLOCK_NUM shall not be greater than PIFS_FLASH_BLOCK_NUM_FS - PIFS_MANAGEMENT_BLOCK_NUM * 2!
#endif
#if PIFS_MOST_WEARED_BLOCK_NUM < 1
#error PIFS_MOST_WEARED_BLOCK_NUM shall be 1 at minimum!
#endif
#if PIFS_MOST_WEARED_BLOCK_NUM > PIFS_FLASH_BLOCK_NUM_FS - PIFS_MANAGEMENT_BLOCK_NUM * 2
#error PIFS_MOST_WEARED_BLOCK_NUM shall not be greater than PIFS_FLASH_BLOCK_NUM_FS - PIFS_MANAGEMENT_BLOCK_NUM * 2!
#endif
#if PIFS_ENABLE_DIRECTORIES && !PIFS_ENABLE_ATTRIBUTES
#error PIFS_ENABLE_ATTRIBUTES shall be 1 if PIFS_ENABLE_DIRECTORIES is 1!
#endif

#define PIFS_INIT_ATTRIB(attrib)    do { \
        (attrib) = PIFS_FLASH_ERASED_BYTE_VALUE; \
    } while (0)

#if PIFS_PATH_SEPARATOR_CHAR != '/' && PIFS_PATH_SEPARATOR_CHAR != '\\'
#error Invalid PIFS_PATH_SEPARATOR_CHAR! Forward slash '/' or backslash '\\' are supported!
#endif

#define PIFS_FREE_PAGE_BUF_SIZE         (PIFS_FLASH_PAGE_NUM_FS / PIFS_BYTE_BITS)

#define PIFS_SET_ERRNO(status)          do { \
        pifs_errno = status; \
    } while (0)

#if PIFS_ENABLE_OS
extern PIFS_OS_MUTEX_TYPE pifs_mutex;
#define PIFS_GET_MUTEX()     PIFS_OS_GET_MUTEX(pifs_mutex)
#define PIFS_PUT_MUTEX()     PIFS_OS_PUT_MUTEX(pifs_mutex)
#else
#define PIFS_GET_MUTEX()
#define PIFS_PUT_MUTEX()
#endif

/** This is the only place, where Pi number can be found in the source code
 * of Pi file system and it is not used at all!
 */
#define PIFS_PI              3.14159265358979323846

/**
 * Each block has specified role in the file system:
 * management area, data area or reserved.
 * Reserved blocks are not used by the file system, they can store code for
 * boot or other type of data.
 */
typedef enum
{
    /**
     * Management information stored in the block. Actual management block.
     */
    PIFS_BLOCK_TYPE_PRIMARY_MANAGEMENT = 0x01,
    /**
     * Management information stored in the block. Next management block.
     * Secondary management pages will be used when primary management area is
     * full.
     */
    PIFS_BLOCK_TYPE_SECONDARY_MANAGEMENT = 0x02,
    /** Data stored in the block. */
    PIFS_BLOCK_TYPE_DATA = 0x04,
    /** Reserved block */
    PIFS_BLOCK_TYPE_RESERVED = 0x08,
} pifs_block_type_t;

/**
 * Address of a page in flash memory.
 * This structure is used in RAM and flash memory as well.
 */
typedef struct PIFS_PACKED_ATTRIBUTE
{
    pifs_block_address_t    block_address;
    pifs_page_address_t     page_address;
} pifs_address_t;

#define PIFS_ADDRESS_SIZE_BYTE      (sizeof(pifs_address_t))

/**
 * Type to store least weared blocks in the file system's header.
 * It is used to speed up finding least weared blocks.
 * This structure is used in RAM and flash memory as well.
 */
typedef struct PIFS_PACKED_ATTRIBUTE
{
    pifs_block_address_t    block_address;
    pifs_wear_level_cntr_t  wear_level_cntr;
} pifs_wear_level_t;

/**
 * File system's header.
 * This structure is used in RAM and flash memory as well.
 */
typedef struct PIFS_PACKED_ATTRIBUTE
{
    uint32_t                magic;                      /**< Magic number to help finding the header at startup */
#if PIFS_ENABLE_VERSION
    uint8_t                 majorVersion;               /**< Major version of file system */
    uint8_t                 minorVersion;               /**< Minor version of file system */
#endif
    uint32_t                counter;
#if PIFS_ENABLE_CONFIG_IN_FLASH
    /* Flash configuration */
    uint16_t                flash_block_num_all;        /**< Number of all blocks in the flash memory */
    uint16_t                flash_block_reserved_num;   /**< Reserved blocks at the beginning of the flash memory */
    uint16_t                flash_page_per_block;       /**< Number of flash (physical) pages in a block */
    uint16_t                flash_page_size_byte;       /**< Size of flash (physical) page in bytes */
    /* File system configuration */
    uint16_t                logical_page_size_byte;     /**< Size of logical page in bytes */
    uint16_t                filename_len_max;           /**< Maximum length of file name */
    uint16_t                entry_num_max;              /**< Maximum number of entries in entry list (directory) */
    uint16_t                user_data_size_byte;        /**< Number of data bytes per file */
    uint8_t                 management_block_num;       /**< Number of management blocks */
    uint16_t                least_weared_block_num;     /**< Number of least weared blocks in the list */
    uint16_t                most_weared_block_num;      /**< Number of most weared blocks in the list */
    uint16_t                delta_map_page_num;         /**< Number of delta map pages */
    uint8_t                 map_page_count_size;        /**< Size of map page count's type in bytes */
    bool_t                  use_delta_for_entries : 1;  /**< TRUE: delta pages used for entries */
    bool_t                  enable_directories : 1;     /**< TRUE: directories can be create, read */
    bool_t                  enable_crc : 1;             /**< TRUE: CRC is calculate, FALSE: checksum is calculated */
#endif
    /* file system status */
    pifs_block_address_t    management_block_address;       /**< Address of primary (active) management block */
    pifs_block_address_t    next_management_block_address;  /**< Address of secondary (next) management block */
    pifs_address_t          free_space_bitmap_address;      /**< Address of free space bitmap (FSBM) */
    pifs_address_t          root_entry_list_address;        /**< Root directory */
    pifs_address_t          delta_map_address;              /**< Address of delta map */
    pifs_address_t          wear_level_list_address;        /**< Adress of wear level list */
    /** Data blocks with lowest erase counter value */
    pifs_wear_level_t       least_weared_blocks[PIFS_LEAST_WEARED_BLOCK_NUM];   /**< List of least weared blocks */
    pifs_wear_level_t       most_weared_blocks[PIFS_MOST_WEARED_BLOCK_NUM];     /**< List of most weared blocks */
    pifs_wear_level_cntr_t  wear_level_cntr_max;            /**< Maximum number of wear level */
    /** Checksum shall be the last element! */
    pifs_checksum_t         checksum;                       /**< Checksum of file system's header */
} pifs_header_t;

/**
 * File or directory entry.
 * This structure is used in RAM and flash memory as well.
 */
typedef struct PIFS_PACKED_ATTRIBUTE
{
    pifs_char_t             name[PIFS_FILENAME_LEN_MAX];    /**< Name of file or directory */
#if PIFS_ENABLE_ATTRIBUTES
    uint8_t                 attrib;                         /**< Attribute's of file */
#endif
#if PIFS_ENABLE_USER_DATA
    pifs_user_data_t        user_data;          /**< User defined data */
#endif
    pifs_address_t          first_map_address;  /**< First map page's address */
    pifs_file_size_t        file_size;          /**< Bytes written to file */
    /** Checksum shall be the last element! */
    pifs_checksum_t         checksum;
} pifs_entry_t;

/**
 * Map's header.
 * This structure is used in RAM and flash memory as well.
 */
typedef struct PIFS_PACKED_ATTRIBUTE
{
    pifs_address_t          prev_map_address;   /**< Address of previous map */
    pifs_checksum_t         prev_map_checksum;
    pifs_address_t          next_map_address;   /**< Address of next map */
    pifs_checksum_t         next_map_checksum;
} pifs_map_header_t;

/**
 * Map of file's page.
 * This structure is used in RAM and flash memory as well.
 */
typedef struct PIFS_PACKED_ATTRIBUTE
{
    pifs_address_t          address;            /**< Address of file's page */
    pifs_map_page_count_t   page_count;         /**< Number of pages */
    /** Checksum shall be the last element! */
    pifs_checksum_t         checksum;
} pifs_map_entry_t;

/**
 * Wear level (erase count) of a block.
 * This structure is used in RAM and flash memory as well.
 */
typedef struct PIFS_PACKED_ATTRIBUTE
{
    pifs_wear_level_cntr_t wear_level_cntr;     /**< Wear level count base */
    /**
     * When a bit is programmed it shall be added to wear level count to get
     * the real wear level count */
    pifs_wear_level_bits_t wear_level_bits;
    /** Checksum shall be the last element! */
    pifs_checksum_t         checksum;
} pifs_wear_level_entry_t;

/**
 * Delta page.
 * This structure is used in RAM and flash memory as well.
 */
typedef struct PIFS_PACKED_ATTRIBUTE
{
    pifs_address_t          delta_address;
    pifs_address_t          orig_address;
    /** Checksum shall be the last element! */
    pifs_checksum_t         checksum;
} pifs_delta_entry_t;

/**
 * Actual status and parameters of an opened file.
 * This structure is used only in RAM.
 */
typedef struct
{
    bool_t                  is_used PIFS_BOOL_SIZE;
    bool_t                  is_opened PIFS_BOOL_SIZE;
    bool_t                  is_entry_changed PIFS_BOOL_SIZE;
    bool_t                  mode_create_new_file PIFS_BOOL_SIZE;
    bool_t                  mode_read PIFS_BOOL_SIZE;
    bool_t                  mode_write PIFS_BOOL_SIZE;
    bool_t                  mode_append PIFS_BOOL_SIZE;
    bool_t                  mode_file_shall_exist PIFS_BOOL_SIZE;
    bool_t                  mode_deleted PIFS_BOOL_SIZE;
    /* TODO entry_list_address shall be updated for every opened file after pifs_merge() */
    pifs_address_t          entry_list_address; /**< Entry list (directory) where the file belongs to */
    pifs_entry_t            entry;              /**< File's entry, one element of entry list */
    pifs_status_t           status;             /**< Last file operation's result */
    pifs_address_t          actual_map_address; /**< Actual map's address used for reading */
    pifs_map_header_t       map_header;         /**< Actual map's header */
    size_t                  map_entry_idx;      /**< Actual entry's index in the map */
    pifs_map_entry_t        map_entry;          /**< Actual entry in the map */
    size_t                  rw_pos;             /**< Position in file after last read/write */
    pifs_address_t          rw_address;         /**< Last read/write page's address */
    pifs_page_count_t       rw_page_count;      /**< Page count to be read/write from 'rw_address' */
} pifs_file_t;

/**
 * Internal structure used by pifs_opendir(), pifs_readdir(), pifs_closedir().
 * This structure is used only in RAM.
 */
typedef struct
{
    bool_t         is_used PIFS_BOOL_SIZE;      /**< TRUE: directory structure is opened, FALSE: directory structure is available */
    bool_t         find_deleted PIFS_BOOL_SIZE; /**< For internal use. TRUE: find deleted entries as well */
    pifs_size_t    entry_page_index;            /**< Actual index of entry page */
    pifs_address_t entry_list_address;          /**< Address of entry list */
    pifs_size_t    entry_list_index;            /**< */
    pifs_dirent_t  directory_entry;
    pifs_entry_t   entry; /**< Can be large, to avoid storing on stack */
} pifs_dir_t;

/**
 * Actual status of file system.
 * This structure is used only in RAM.
 */
typedef struct
{
    pifs_address_t          header_address;                               /**< Address of actual file system's header */
    bool_t                  is_header_found PIFS_BOOL_SIZE;               /**< TRUE: file system's header found */
    bool_t                  is_merging PIFS_BOOL_SIZE;                    /**< TRUE: merging is in progress */
    /* TODO what if is_wear_leveling is 1 and user creates a file, not the static wear leveling's copy? */
    bool_t                  is_wear_leveling PIFS_BOOL_SIZE;              /**< TRUE: wear leveling is in progress */
    pifs_header_t           header;                                       /**< Actual header. */
    pifs_entry_t            entry;                                        /**< For merging */
    /* Page cache */
    pifs_address_t          cache_page_buf_address;                       /**< Address of cache_page_buf */
    uint8_t                 cache_page_buf[PIFS_LOGICAL_PAGE_SIZE_BYTE];  /**< Flash page buffer for cache */
    bool_t                  cache_page_buf_is_dirty;                      /**< TRUE: cache page was changed and needs to be written to flash memory */
    /* Opened files and directories */
    pifs_file_t             file[PIFS_OPEN_FILE_NUM_MAX];                 /**< Opened files */
    pifs_file_t             internal_file;                                /**< Internally opened files */
    pifs_dir_t              dir[PIFS_OPEN_DIR_NUM_MAX];                   /**< Opened directories */
    /** Page buffer of delta map */
    uint8_t                 delta_map_page_buf[PIFS_DELTA_MAP_PAGE_NUM][PIFS_LOGICAL_PAGE_SIZE_BYTE];
    /** TRUE: delta_map_page_buf's content is valid */
    bool_t                  delta_map_page_is_read PIFS_BOOL_SIZE;
    /** TRUE: delta_map_page_buf is inconsistent, it shall be written to the flash memory */
    bool_t                  delta_map_page_is_dirty PIFS_BOOL_SIZE;
    /** General page buffer used by pifs_write_delta(),
     * pifs_copy_fsbm(), pifs_wear_level_list_init(), dmw=delta, merge, wear */
    uint8_t                 dmw_page_buf[PIFS_LOGICAL_PAGE_SIZE_BYTE];
    /** General page buffer use by pifs_fseek, pifs_copy buffer, sc=seek, copy */
    uint8_t                 sc_page_buf[PIFS_LOGICAL_PAGE_SIZE_BYTE];
    uint32_t                error_cntr;         /**< File system's integrity check uses it */
    pifs_size_t             free_data_page_num;
    pifs_size_t             last_static_wear_block_idx; /**< Block index used for last static wear leveling. */
    uint32_t                auto_static_wear_cntr;      /**< Counter to call less often static wear leveling */
#if PIFS_ENABLE_DIRECTORIES
    pifs_char_t             cwd[PIFS_TASK_COUNT_MAX][PIFS_PATH_LEN_MAX];  /**< Current working directory */
    /* TODO current_entry_list_address shall be removed and cwd shall be used instead! */
    pifs_address_t          current_entry_list_address[PIFS_TASK_COUNT_MAX]; /**< Entry list of current working directory */
#endif
#if PIFS_FSCHECK_USE_STATIC_MEMORY
    uint8_t                 free_pages_buf[PIFS_FLASH_PAGE_NUM_FS / PIFS_BYTE_BITS];
#endif
} pifs_t;

extern pifs_t pifs;

pifs_status_t pifs_flush(void);
pifs_status_t pifs_read(pifs_block_address_t a_block_address,
                        pifs_page_address_t a_page_address,
                        pifs_page_offset_t a_page_offset,
                        void * const a_buf,
                        pifs_size_t a_buf_size);
pifs_status_t pifs_write(pifs_block_address_t a_block_address,
                         pifs_page_address_t a_page_address,
                         pifs_page_offset_t a_page_offset,
                         const void * const a_buf,
                         pifs_size_t a_buf_size);
pifs_status_t pifs_erase(pifs_block_address_t a_block_address, pifs_header_t *a_old_header, pifs_header_t *a_new_header);
pifs_status_t pifs_merge(void);
pifs_status_t pifs_header_init(pifs_block_address_t a_block_address,
                               pifs_page_address_t a_page_address,
                               pifs_block_address_t a_next_mgmt_block_address,
                               pifs_header_t * a_header);
pifs_status_t pifs_header_write(pifs_block_address_t a_block_address,
                                pifs_page_address_t a_page_address,
                                pifs_header_t * a_header, bool_t a_mark_pages);

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDE_PIFS_H_ */
