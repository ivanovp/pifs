/**
 * @file        flash_config.h
 * @brief       Flash configuration
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-15 14:06:42 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_FLASH_CONFIG_H_
#define _INCLUDE_FLASH_CONFIG_H_

/* Geometry of M25P40 */
#define PIFS_FLASH_BLOCK_NUM        8       /**< Number of blocks used by the file system */
#define PIFS_FLASH_BLOCK_START      0       /**< Index of first block to use by the file system */
#define PIFS_FLASH_PAGE_PER_BLOCK   256     /**< Number of pages in a block */
#define PIFS_FLASH_PAGE_SIZE_BYTE   256     /**< Size of a page in bytes */
#define PIFS_FLASH_PAGE_SIZE_SPARE  0       /**< Number of spare bytes in a page */

/* Configuration of filesystem */
#define PIFS_MANAGEMENT_PAGES_MAX   8       /**< Maximum number of management pages per block */

#if PIFS_MANAGEMENT_PAGES_MAX >= PIFS_FLASH_PAGE_PER_BLOCK / 2
#error PIFS_MANAGEMENT_PAGES_MAX is too big.
#endif

#endif /* _INCLUDE_FLASH_CONFIG_H_ */
