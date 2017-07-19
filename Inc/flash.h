/**
 * @file        flash.h
 * @brief       Header of flash routines
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-16 11:53:05 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_FLASH_H_
#define _INCLUDE_FLASH_H_

#include <stdint.h>
#include "flash_config.h"
#include "pifs_config.h"
#include "api_pifs.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (PIFS_FLASH_BLOCK_NUM_ALL < 255)
typedef uint8_t pifs_block_address_t;
#define PIFS_BLOCK_ADDRESS_INVALID   (UINT8_MAX - 1u)
#define PIFS_BLOCK_ADDRESS_ERASED    (UINT8_MAX)
#elif (PIFS_FLASH_BLOCK_NUM_ALL < 65535)
typedef uint16_t pifs_block_address_t;
#define PIFS_BLOCK_ADDRESS_INVALID   (UINT16_MAX - 1u)
#define PIFS_BLOCK_ADDRESS_ERASED    (UINT16_MAX)
#elif (PIFS_FLASH_BLOCK_NUM_ALL < 4294967295l)
typedef uint32_t pifs_block_address_t;
#define PIFS_BLOCK_ADDRESS_INVALID   (UINT32_MAX - 1u)
#define PIFS_BLOCK_ADDRESS_ERASED    (UINT32_MAX)
#else
#error PIFS_FLASH_BLOCK_NUM_ALL is too big!
#endif

#if (PIFS_FLASH_PAGE_PER_BLOCK < 255)
typedef uint8_t pifs_page_address_t;
#define PIFS_PAGE_ADDRESS_INVALID   (UINT8_MAX - 1u)
#define PIFS_PAGE_ADDRESS_ERASED    (UINT8_MAX)
#elif (PIFS_FLASH_PAGE_PER_BLOCK < 65535)
typedef uint16_t pifs_page_address_t;
#define PIFS_PAGE_ADDRESS_INVALID   (UINT16_MAX - 1u)
#define PIFS_PAGE_ADDRESS_ERASED    (UINT16_MAX)
#elif (PIFS_FLASH_PAGE_PER_BLOCK < 4294967295l)
typedef uint32_t pifs_page_address_t;
#define PIFS_PAGE_ADDRESS_INVALID   (UINT32_MAX - 1u)
#define PIFS_PAGE_ADDRESS_ERASED    (UINT32_MAX)
#else
#error PIFS_FLASH_PAGE_PER_BLOCK is too big!
#endif

#if (PIFS_FLASH_PAGE_SIZE_BYTE < 255)
typedef uint8_t pifs_page_offset_t;
#define PIFS_PAGE_OFFSET_INVALID   (UINT8_MAX - 1u)
#define PIFS_PAGE_OFFSET_ERASED    (UINT8_MAX)
#elif (PIFS_FLASH_PAGE_SIZE_BYTE < 65535)
typedef uint16_t pifs_page_offset_t;
#define PIFS_PAGE_OFFSET_INVALID   (UINT16_MAX - 1u)
#define PIFS_PAGE_OFFSET_ERASED    (UINT16_MAX)
#elif (PIFS_FLASH_PAGE_SIZE_BYTE < 4294967295l)
typedef uint32_t pifs_page_offset_t;
#define PIFS_PAGE_OFFSET_INVALID   (UINT32_MAX - 1u)
#define PIFS_PAGE_OFFSET_ERASED    (UINT32_MAX)
#else
#error PIFS_FLASH_PAGE_SIZE_BYTE is too big!
#endif

/** Number of blocks used by the file system */
#define PIFS_FLASH_BLOCK_NUM_FS     (PIFS_FLASH_BLOCK_NUM_ALL - PIFS_FLASH_BLOCK_RESERVED_NUM)
/** Size of a block in bytes */
#define PIFS_FLASH_BLOCK_SIZE_BYTE  (PIFS_FLASH_PAGE_SIZE_BYTE * PIFS_FLASH_PAGE_PER_BLOCK)
/** Size of the whole flash memory in bytes */
#define PIFS_FLASH_SIZE_BYTE_ALL    (PIFS_FLASH_BLOCK_SIZE_BYTE * PIFS_FLASH_BLOCK_NUM_ALL)
/** Size of the file system in bytes */
#define PIFS_FLASH_SIZE_BYTE_FS     (PIFS_FLASH_BLOCK_SIZE_BYTE * PIFS_FLASH_BLOCK_NUM_FS)
/** Number of all flash pages */
#define PIFS_FLASH_PAGE_NUM_ALL     (PIFS_FLASH_BLOCK_NUM_ALL * PIFS_FLASH_PAGE_PER_BLOCK)
/** Number of flash pages used by the file system */
#define PIFS_FLASH_PAGE_NUM_FS      (PIFS_FLASH_BLOCK_NUM_FS * PIFS_FLASH_PAGE_PER_BLOCK)

pifs_status_t pifs_flash_init(void);
pifs_status_t pifs_flash_delete(void);
pifs_status_t pifs_flash_read(pifs_block_address_t a_block_address, pifs_page_address_t a_page_address, pifs_page_offset_t a_page_offset, void * const a_buf, size_t a_buf_size);
pifs_status_t pifs_flash_write(pifs_block_address_t a_block_address, pifs_page_address_t a_page_address, pifs_page_address_t a_page_offset, const void * const a_buf, size_t a_buf_size);
pifs_status_t pifs_flash_erase(pifs_block_address_t a_block_address);
void pifs_flash_print_stat(void);

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDE_FLASH_H_ */
