/**
 * @file        flash_config.h
 * @brief       Flash configuration
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-27 19:32:33 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_FLASH_CONFIG_H_
#define _INCLUDE_FLASH_CONFIG_H_

#define FLASH_TYPE_M25P40   0
#define FLASH_TYPE_M25P80   1
#define FLASH_TYPE_N25Q128A 2

/** Type of emulated flash memory */
#define FLASH_TYPE          FLASH_TYPE_M25P40

#if FLASH_TYPE == FLASH_TYPE_M25P40
/* Geometry of M25P40 */
#define PIFS_FLASH_BLOCK_NUM_ALL            8u      /**< Number of blocks in flash memory */
#define PIFS_FLASH_BLOCK_RESERVED_NUM       0u      /**< Index of first block to use by the file system */
#define PIFS_FLASH_PAGE_PER_BLOCK           256u    /**< Number of pages in a block */
#define PIFS_FLASH_PAGE_SIZE_BYTE           256u    /**< Size of a page in bytes */
#define PIFS_FLASH_PAGE_SIZE_SPARE          0u      /**< Number of spare bytes in a page */
#elif FLASH_TYPE == FLASH_TYPE_M25P80
/* Geometry of M25P40 */
#define PIFS_FLASH_BLOCK_NUM_ALL            16u     /**< Number of blocks in flash memory */
#define PIFS_FLASH_BLOCK_RESERVED_NUM       0u      /**< Index of first block to use by the file system */
#define PIFS_FLASH_PAGE_PER_BLOCK           256u    /**< Number of pages in a block */
#define PIFS_FLASH_PAGE_SIZE_BYTE           256u    /**< Size of a page in bytes */
#define PIFS_FLASH_PAGE_SIZE_SPARE          0u      /**< Number of spare bytes in a page */
#elif FLASH_TYPE == FLASH_TYPE_N25Q128A
/* Geometry of N25Q128A */
#define PIFS_FLASH_BLOCK_NUM_ALL            4096u   /**< Number of blocks in flash memory */
#define PIFS_FLASH_BLOCK_RESERVED_NUM       4u      /**< Index of first block to use by the file system */
#define PIFS_FLASH_PAGE_PER_BLOCK           16u     /**< Number of pages in a block */
#define PIFS_FLASH_PAGE_SIZE_BYTE           256u    /**< Size of a page in bytes */
#define PIFS_FLASH_PAGE_SIZE_SPARE          0u      /**< Number of spare bytes in a page */
#endif

#define PIFS_FLASH_ERASED_VALUE       0xFFu
#define PIFS_FLASH_PROGRAMMED_VALUE   0x00u

#endif /* _INCLUDE_FLASH_CONFIG_H_ */
