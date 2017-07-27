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

#define PIFS_OPEN_FILE_NUM_MAX          4u   /**< Maximum number of opened file */
#define PIFS_OPEN_DIR_NUM_MAX           2u   /**< Maximum number of opened directories */
#define PIFS_FILENAME_LEN_MAX           32u  /**< Maximum length of file name */
#define PIFS_ENTRY_NUM_MAX              254u /**< Maximum number of files and directories */
#define PIFS_ENABLE_ATTRIBUTE           1u   /**< 1: Use attribute field of files, 0: don't use attribute field */
#define PIFS_MANAGEMENT_BLOCKS          1u   /**< Number of management blocks. Minimum: 1 (Allocated area is twice of this number.) */
#define PIFS_CHECKSUM_SIZE              4u   /**< Size of checksum variable in bytes. Valid values are 1, 2 and 4. */
#define PIFS_PAGE_COUNT_SIZE            2u   /**< Size of page count variable of map entry in bytes. Valid values are 1, 2 and 4. */
#define PIFS_OPTIMIZE_FOR_RAM           1u   /**< 1: Use less RAM, 0: Use more RAM, but faster code execution */
#define PIFS_CHECK_IF_PAGE_IS_ERASED    0u   /**< 1: Check if page is erased */
#define PIFS_DELTA_MAP_PAGE_NUM         2u   /**< Number of delta page maps */
#define PIFS_USE_DELTA_FOR_ENTRIES      0u   /**< 1: Use delta pages for list entries, 0: don't use delta pages */
#define PIFS_ENABLE_FSEEK_BEYOND_FILE   1u   /**< 1: Enable seeking beyond file size, 0: disable seeking beyond file size */
#define PIFS_ENABLE_FSEEK_ERASED_VALUE  0u   /**< 1: Write 0xFF values when seeking beyond file size,
                                                  0: write 0 values when seeking beyond file size (default). */

#define PIFS_PACKED_ATTRIBUTE           __attribute__((packed))

typedef char pifs_char_t;

#endif /* _INCLUDE_PIFS_CONFIG_H_ */
