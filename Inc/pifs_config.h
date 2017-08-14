/**
 * @file        pifs_config.h
 * @brief       Pi file system's configuration
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-27 19:41:42 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_PIFS_CONFIG_H_
#define _INCLUDE_PIFS_CONFIG_H_

#include <unistd.h>

/** PIFS_LOGICAL_PAGE_SIZE_BYTE
 * Allocation unit of file system.
 * Larger logical page needs more RAM and less pages in management area.
 * Smaller logical page needs less RAM and more pages in management area.
 * It shall be larger or equal to PIFS_FLASH_PAGE_SIZE!
 * Its size shall be power of two!
 */
#define PIFS_LOGICAL_PAGE_SIZE_BYTE     256u
#define PIFS_OPEN_FILE_NUM_MAX          4u   /**< Maximum number of opened file */
#define PIFS_OPEN_DIR_NUM_MAX           2u   /**< Maximum number of opened directories */
#define PIFS_FILENAME_LEN_MAX           32u  /**< Maximum length of file name */
#define PIFS_PATH_LEN_MAX               128u /**< Maximum length of path. Only relevent if PIFS_ENABLE_DIRECTORIES is 1. */
#define PIFS_ENTRY_NUM_MAX              32u  /**< Maximum number of files and directories in a directory */
#define PIFS_ENABLE_USER_DATA           1u   /**< 1: Add user data (pifs_user_data_t) to every file, 0: don't add user data */
#define PIFS_MANAGEMENT_BLOCK_NUM       1u   /**< Number of management blocks. Minimum: 1 (Allocated area is twice of this number.) */
#define PIFS_LEAST_WEARED_BLOCK_NUM     6u   /**< Number of stored least weared blocks */
#define PIFS_DELTA_MAP_PAGE_NUM         2u   /**< Number of delta page maps */
#define PIFS_CHECKSUM_SIZE              4u   /**< Size of checksum variable in bytes. Valid values are 1, 2 and 4. */
#define PIFS_MAP_PAGE_COUNT_SIZE        1u   /**< Size of page count variable of map entry in bytes. Valid values are 1, 2 and 4. */
#define PIFS_ENABLE_CONFIG_IN_FLASH     1u   /**< 1: Store file system's configuration in flash memory */
#define PIFS_OPTIMIZE_FOR_RAM           1u   /**< 1: Use less RAM, 0: Use more RAM, but faster code execution */
#define PIFS_CHECK_IF_PAGE_IS_ERASED    1u   /**< 1: Check if page is erased */
#define PIFS_USE_DELTA_FOR_ENTRIES      0u   /**< 1: Use delta pages for list entries, 0: don't use delta pages */
#define PIFS_ENABLE_FSEEK_BEYOND_FILE   1u   /**< 1: Enable seeking beyond file size, 0: disable seeking beyond file size */
#define PIFS_ENABLE_FSEEK_ERASED_VALUE  0u   /**< 1: Write 0xFF values when seeking beyond file size,
                                                  0: write 0 values when seeking beyond file size (default). */
#define PIFS_STATIC_WEAR_LEVEL_LIMIT   20u
#define PIFS_STATIC_WEAR_LEVEL_PERCENT 10u
#define PIFS_CALC_TBR_IN_FREE_SPACE     0u   /**< 1: Free pages and to be released pages are counted, 0: only free pages counted */
#define PIFS_FSCHECK_USE_STATIC_MEMORY  1u   /**< 1: Use static memory for file system check, 0: Use dynamic (malloc) for file system check */
#define PIFS_ENABLE_DIRECTORIES         1u   /**< 1: Support directories, 0: only support root directory */
#define PIFS_PATH_SEPARATOR_CHAR        '/'  /**< Character to separate directories in path, '/' or '\' */

#define PIFS_PACKED_ATTRIBUTE           __attribute__((packed))
#define PIFS_ALIGNED_ATTRIBUTE(align)   __attribute__((aligned(align)))
/** Task delay is used when flash initialization fails. */
#define PIFS_DELAY_MS(ms)               usleep(ms * 1000)

typedef char pifs_char_t;
#define PIFS_CHAR_MAX   255

#if PIFS_ENABLE_USER_DATA
/**
 * pifs_user_data_t is saved to every file if PIFS_ENABLE_USER_DATA is 1.
 * @see pifs_fgetuserdata pifs_fsetuserdata
 */
typedef struct
{
    uint32_t ctime;
    uint32_t cdate;
} pifs_user_data_t;
#endif

#endif /* _INCLUDE_PIFS_CONFIG_H_ */
