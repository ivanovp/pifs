/**
 * @file        pifs.c
 * @brief       Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-15 15:03:34 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#include <stdio.h>
#include <string.h>

#include "api_pifs.h"
#include "flash.h"
#include "flash_config.h"
#include "pifs.h"
#include "pifs_debug.h"

object_id_t         latest_object_id = 0;
block_address_t     mgmt_block_address = 0;
page_address_t      mgmt_page_address = 0;

checksum_t calc_header_checksum(pifs_header_t * a_pifs_header)
{
    uint8_t * ptr = (uint8_t*) a_pifs_header;
    checksum_t checksum = UINT8_MAX;
    uint8_t cntr = sizeof(pifs_header_t) - 1; /* Skip checksum */
    
    while (cntr--)
    {
        checksum += *ptr;
        ptr++;
    }

    return checksum;
}

pifs_status_t pifs_init(void)
{
    pifs_status_t   ret = PIFS_FLASH_INIT_ERROR;
    block_address_t ba;
    page_address_t  pa;
    pifs_header_t   header;
    bool_t          is_mgmt_found = FALSE;
    checksum_t      checksum;

    printf("Maximum number of management pages: %i\r\n",
            (PIFS_FLASH_BLOCK_NUM - PIFS_FLASH_BLOCK_START) * PIFS_MANAGEMENT_PAGES_MAX);

    ret = flash_init();

    if (ret == PIFS_SUCCESS)
    {
        /* Find latest management block */
        for (ba = PIFS_FLASH_BLOCK_START; ba < PIFS_FLASH_BLOCK_NUM; ba++)
        {
            for (pa = 0; pa < PIFS_MANAGEMENT_PAGES_MAX; pa++)
            {
                flash_read(ba, pa, 0, &header, sizeof(header));
                if (header.magic == PIFS_MAGIC
#if ENABLE_PIFS_VERSION
                        && header.majorVersion == PIFS_MAJOR_VERSION
                        && header.minorVersion == PIFS_MINOR_VERSION
#endif
                   )
                {
                    printf("Management page found: BA%i/PA%i\r\n", ba, pa);
                    checksum = calc_header_checksum(&header);
                    if (checksum == header.checksum)
                    {
                        printf("Checksum is valid\r\n");
                        is_mgmt_found = TRUE;
                    }
                    else
                    {
                        PIFS_ERROR_MSG("Checksum is invalid! Calculated: 0x%02X, read: 0x%02X\r\n",
                                checksum, header.checksum);
                    }
                }
            }
        }
    }

    if (!is_mgmt_found)
    {
        ba = PIFS_FLASH_BLOCK_START;
        pa = 0;
        printf("Creating managamenet block BA%i/PA%i\r\n", ba, pa);
        header.magic = PIFS_MAGIC;
#if ENABLE_PIFS_VERSION
        header.majorVersion = PIFS_MAJOR_VERSION;
        header.minorVersion = PIFS_MINOR_VERSION;
#endif
        header.counter = 1;
        header.checksum = calc_header_checksum(&header);
        flash_erase(ba);
        flash_write(ba, pa, 0, &header, sizeof(header));
    }

    return ret;
}

pifs_status_t pifs_delete(void)
{
    pifs_status_t ret = PIFS_ERROR;

    ret = flash_delete();

    return ret;
}

