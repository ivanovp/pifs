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

// NOTE handle endianness?
//#define PIFS_MAGIC                        0x50494653u  /* PIFS */
#define PIFS_MAGIC                          0x53464950u  /* PIFS */
#if PIFS_MAGIC == 0
#error PIFS_MAGIC shall not be zero!
#elif PIFS_MAGIC == 0xFFFFFFFF
#error PIFS_MAGIC shall not be 0xFFFFFFFF!
#endif

#define ENABLE_PIFS_VERSION                 1
#define PIFS_MAJOR_VERSION                  1u
#define PIFS_MINOR_VERSION                  0u

#define PIFS_ENABLE_ATTRIBUTES              1u   /**< 1: Use attribute field of files, 0: don't use attribute field */
#define PIFS_ATTRIB_READONLY                0x01u
#define PIFS_ATTRIB_HIDDEN                  0x02u
#define PIFS_ATTRIB_SYSTEM                  0x04u
#define PIFS_ATTRIB_DIR                     0x10u
#define PIFS_ATTRIB_ARCHIVE                 0x20u
#define PIFS_ATTRIB_DELETED                 0x80u
#define PIFS_ATTRIB_ALL                     UINT8_MAX

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

#define PIFS_MAP_PAGES_RECOMENDED           (((PIFS_LOGICAL_PAGE_NUM_FS - PIFS_MANAGEMENT_BLOCK_NUM * PIFS_LOGICAL_PAGE_PER_BLOCK) * PIFS_MAP_ENTRY_SIZE_BYTE + PIFS_LOGICAL_PAGE_SIZE_BYTE - 1) / PIFS_LOGICAL_PAGE_SIZE_BYTE)

#define PIFS_MANAGEMENT_PAGES_MIN           (PIFS_HEADER_SIZE_PAGE + PIFS_ENTRY_LIST_SIZE_PAGE + PIFS_FREE_SPACE_BITMAP_SIZE_PAGE + PIFS_DELTA_MAP_PAGE_NUM + PIFS_WEAR_LEVEL_LIST_SIZE_PAGE)
#define PIFS_MANAGEMENT_BLOCK_NUM_MIN          ((PIFS_MANAGEMENT_PAGES_MIN + PIFS_LOGICAL_PAGE_PER_BLOCK - 1) / PIFS_LOGICAL_PAGE_PER_BLOCK)
#define PIFS_MANAGEMENT_PAGES_RECOMMENDED   (PIFS_MANAGEMENT_PAGES_MIN + PIFS_MAP_PAGES_RECOMENDED)
#define PIFS_MANAGEMENT_BLOCK_NUM_RECOMMENDED  ((PIFS_MANAGEMENT_PAGES_RECOMMENDED + PIFS_LOGICAL_PAGE_PER_BLOCK - 1) / PIFS_LOGICAL_PAGE_PER_BLOCK)
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
#define PIFS_ROOT_STR                       "/"
#else
#define PIFS_ROOT_STR                       "\\"
#endif
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

#if PIFS_CHECKSUM_SIZE == 1
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
#if PIFS_FLASH_ERASED_BYTE_VALUE == 0xFF
/* If erased value is 0xFF, invert attribute bits. */
/* Therefore bits can be updated in the existing entry, no new entry is needed. */
#define PIFS_INVERT_ATTRIBUTE_BITS      1
#else
#define PIFS_INVERT_ATTRIBUTE_BITS      0
#endif
#if PIFS_ENABLE_DIRECTORIES && !PIFS_ENABLE_ATTRIBUTES
#error PIFS_ENABLE_ATTRIBUTES shall be 1 if PIFS_ENABLE_DIRECTORIES is 1!
#endif

#if PIFS_PATH_SEPARATOR_CHAR != '/' && PIFS_PATH_SEPARATOR_CHAR != '\\'
#error Invalid PIFS_PATH_SEPARATOR_CHAR! Forward slash '/' or backslash '\\' are supported!
#endif

#define PIFS_FREE_PAGE_BUF_SIZE         (PIFS_FLASH_PAGE_NUM_FS / PIFS_BYTE_BITS)

#define PIFS_SET_ERRNO(status)          do { \
        pifs_errno = status; \
    } while (0)

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
    /** Any type of the above. */
    PIFS_BLOCK_TYPE_ANY = 0x07,
    /** Reserved block */
    PIFS_BLOCK_TYPE_RESERVED = 0x08
} pifs_block_type_t;

/**
 * Address of a page in flash memory.
 */
typedef struct PIFS_PACKED_ATTRIBUTE
{
    pifs_block_address_t    block_address;
    pifs_page_address_t     page_address;
} pifs_address_t;

typedef struct PIFS_PACKED_ATTRIBUTE
{
    pifs_block_address_t    block_address;
    pifs_wear_level_cntr_t  wear_level_cntr;
} pifs_wear_level_t;

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
#if PIFS_ENABLE_CONFIG_IN_FLASH
    /* Flash configuration */
    uint16_t                flash_block_num_all;
    uint16_t                flash_block_reserved_num;
    uint16_t                flash_page_per_block;
    uint16_t                flash_page_size_byte;
    /* File system configuration */
    uint16_t                logical_page_size_byte;
    uint16_t                filename_len_max;
    uint16_t                entry_num_max;
    uint16_t                user_data_size_byte;
    uint8_t                 management_block_num;
    uint16_t                least_weared_block_num;
    uint16_t                delta_map_page_num;
    uint8_t                 map_page_count_size;
    bool_t                  use_delta_for_entries : 1;
    bool_t                  enable_directories : 1;
#endif
    /* file system status */
    pifs_block_address_t    management_block_address;       /**< number of pifs_management_blocks */
    pifs_block_address_t    next_management_block_address;  /**< Number of PIFS_MANAGEMENT_BLOCK_NUM */
    pifs_address_t          free_space_bitmap_address;
    pifs_address_t          entry_list_address;
    pifs_address_t          delta_map_address;
    pifs_address_t          wear_level_list_address;
    /** Data blocks with lowest erase counter value */
    pifs_wear_level_t       least_weared_blocks[PIFS_LEAST_WEARED_BLOCK_NUM];
    pifs_wear_level_cntr_t  wear_level_cntr_max;
    /** Checksum shall be the last element! */
    pifs_checksum_t         checksum;
} pifs_header_t;

/**
 * File or directory entry.
 */
typedef struct PIFS_PACKED_ATTRIBUTE
{
    pifs_char_t             name[PIFS_FILENAME_LEN_MAX];
#if PIFS_ENABLE_ATTRIBUTES
    uint8_t                 attrib;
#endif
    /* TODO implement file date and time! modified, created, etc? */
#if PIFS_ENABLE_USER_DATA
    pifs_user_data_t        user_data;
#endif
    pifs_address_t          first_map_address;  /**< First map page's address */
    pifs_file_size_t        file_size;          /**< Bytes written to file */
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
    pifs_map_page_count_t   page_count;
} pifs_map_entry_t;

typedef struct PIFS_PACKED_ATTRIBUTE
{
    pifs_wear_level_cntr_t wear_level_cntr;
    pifs_wear_level_bits_t wear_level_bits;
} pifs_wear_level_entry_t;

typedef struct PIFS_PACKED_ATTRIBUTE
{
    pifs_wear_level_entry_t wear_level[PIFS_FLASH_BLOCK_NUM_FS];
} pifs_wear_level_list_t;

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
    bool_t                  is_size_changed PIFS_BOOL_SIZE;
    bool_t                  mode_create_new_file PIFS_BOOL_SIZE;
    bool_t                  mode_read PIFS_BOOL_SIZE;
    bool_t                  mode_write PIFS_BOOL_SIZE;
    bool_t                  mode_append PIFS_BOOL_SIZE;
    bool_t                  mode_file_shall_exist PIFS_BOOL_SIZE;
    bool_t                  mode_deleted PIFS_BOOL_SIZE;
    pifs_entry_t            entry;              /**< File's entry, one element of entry list */
    pifs_status_t           status;             /**< Last file operation's result */
    pifs_address_t          actual_map_address; /**< Actual map's address used for reading */
    pifs_map_header_t       map_header;         /**< Actual map's header */
    size_t                  map_entry_idx;      /**< Actual entry's index in the map */
    pifs_map_entry_t        map_entry;          /**< Actual entry in the map */
    /* TODO To think about: read_pos/write_pos could be in one variable? */
    size_t                  read_pos;           /**< Position in file after last read */
    pifs_address_t          read_address;       /**< Last read page's address */
    pifs_page_count_t       read_page_count;    /**< Page count to be read from 'read_address' */
    size_t                  write_pos;          /**< Position in file after last write */
    pifs_address_t          write_address;      /**< Last written page's address */
    pifs_page_count_t       write_page_count;   /**< Page count to be ritten to 'write_address' */
} pifs_file_t;

/**
 * Internal structure used by pifs_opendir(), pifs_listdir(), pifs_closedir().
 */
typedef struct
{
    bool_t         is_used PIFS_BOOL_SIZE;
    bool_t         find_deleted PIFS_BOOL_SIZE;
    pifs_size_t    entry_page_index;
    pifs_address_t entry_list_address;
    pifs_size_t    entry_list_index;
    pifs_dirent_t  directory_entry;
    pifs_entry_t   entry; /**< Can be large, to avoid storing on stack */
} pifs_dir_t;

/**
 * Actual status of file system.
 */
typedef struct
{
    pifs_address_t          header_address;
    bool_t                  is_header_found PIFS_BOOL_SIZE;
    bool_t                  is_merging PIFS_BOOL_SIZE;
    pifs_header_t           header;                                       /**< Actual header. */
    pifs_entry_t            entry;                                        /**< For merging */
    /* Page cache */
    pifs_address_t          cache_page_buf_address;                       /**< Address of cache_page_buf */
    uint8_t                 cache_page_buf[PIFS_LOGICAL_PAGE_SIZE_BYTE];  /**< Flash page buffer for cache */
    bool_t                  cache_page_buf_is_dirty;
    /* Opened files and directories */
    pifs_file_t             file[PIFS_OPEN_FILE_NUM_MAX];                 /**< Opened files */
    pifs_file_t             internal_file;                                /**< Internally opened files */
    pifs_dir_t              dir[PIFS_OPEN_DIR_NUM_MAX];                   /**< Opened directories */
    /* Delta pages */
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
#endif /* _INCLUDE_PIFS_H_ */
