/**
 * @file        flash.h
 * @brief       Header of flash routines
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-15 14:29:50 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_FLASH_H_
#define _INCLUDE_FLASH_H_

#include <stdint.h>
#include "flash_config.h"
#include "api_pifs.h"

#ifdef __cplusplus
extern "C" {
#endif

#if ( PIFS_FLASH_BLOCK_NUM < 256 )
typedef uint8_t block_address_t;
#elif ( PIFS_FLASH_BLOCK_NUM < 65536 )
typedef uint16_t block_address_t;
#elif ( PIFS_FLASH_BLOCK_NUM < 4294967296l )
typedef uint32_t block_address_t;
#else
#error PIFS_FLASH_BLOCK_NUM is too big!
#endif

#if ( PIFS_FLASH_PAGE_PER_BLOCK < 256 )
typedef uint8_t page_address_t;
#elif ( PIFS_FLASH_PAGE_PER_BLOCK < 65536 )
typedef uint16_t page_address_t;
#elif ( PIFS_FLASH_PAGE_PER_BLOCK < 4294967296l )
typedef uint32_t page_address_t;
#else
#error PIFS_FLASH_PAGE_PER_BLOCK is too big!
#endif

#if ( PIFS_FLASH_PAGE_SIZE_BYTE < 256 )
typedef uint8_t page_offset_t;
#elif ( PIFS_FLASH_PAGE_SIZE_BYTE < 65536 )
typedef uint16_t page_offset_t;
#elif ( PIFS_FLASH_PAGE_SIZE_BYTE < 4294967296l )
typedef uint32_t page_offset_t;
#else
#error PIFS_FLASH_PAGE_SIZE_BYTE is too big!
#endif

#if ( PIFS_OBJECT_NUM_MAX < 255 )
typedef uint8_t object_id_t;
#elif ( PIFS_OBJECT_NUM_MAX < 65535 )
typedef uint16_t object_id_t;
#elif ( PIFS_OBJECT_NUM_MAX < 4294967295l )
typedef uint32_t object_id_t;
#else
#error PIFS_OBJECT_NUM_MAX is too big!
#endif

#define PIFS_FLASH_BLOCK_SIZE_BYTE  ( PIFS_FLASH_PAGE_SIZE_BYTE * PIFS_FLASH_PAGE_PER_BLOCK )
#define PIFS_FLASH_SIZE_BYTE        ( PIFS_FLASH_BLOCK_SIZE_BYTE * PIFS_FLASH_BLOCK_NUM )
#define PIFS_FLASH_PAGE_NUM         ( PIFS_FLASH_BLOCK_NUM * PIFS_FLASH_PAGE_PER_BLOCK )

pifs_status_t flash_init(void);
pifs_status_t flash_delete(void);
pifs_status_t flash_read(block_address_t a_block_address, page_address_t a_page_address, page_offset_t a_page_offset, void * const a_buf, size_t a_buf_size);
pifs_status_t flash_write(block_address_t a_block_address, page_address_t a_page_address, page_address_t a_page_offset, const void * a_buf, size_t a_buf_size);
pifs_status_t flash_erase(block_address_t a_block_address);

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDE_FLASH_H_ */
