/**
 * @file        pifs_os.h
 * @brief       OS functions for Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-08-26 07:10:19
 * Last modify: 2018-02-10 07:42:53 ivanovp {Time-stamp}
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
#ifndef _INCLUDE_PIFS_OS_H_
#define _INCLUDE_PIFS_OS_H_

#include <stdint.h>
#include <unistd.h>

#include "common.h"
#include "pifs_config.h"
#include "pifs.h"
#include "pifs_debug.h"

/** Task delay is used when flash initialization fails. */
#define PIFS_OS_DELAY_MS(ms)               usleep(ms * 1000)
/* For mutex debugging */
#define PIFS_OS_DEFINE_MUTEX(mutex)        extern bool_t mutex
#define PIFS_OS_CREATE_MUTEX(mutex)        FALSE
#define PIFS_OS_DELETE_MUTEX(mutex)        
#define PIFS_OS_GET_MUTEX(mutex)           do { \
      PIFS_ASSERT(mutex == FALSE); \
      mutex = TRUE; \
    } while (0)
#define PIFS_OS_PUT_MUTEX(mutex)           do { \
      PIFS_ASSERT(mutex == TRUE); \
      mutex = FALSE; \
    } while (0)
#define PIFS_OS_MUTEX_TYPE                 bool_t
#define PIFS_OS_TASK_ID_NULL               0
#define PIFS_OS_TASK_ID_TYPE               uint8_t
#define PIFS_OS_GET_TASK_ID()              0
#define PIFS_OS_TASK_ID_IS_SEQUENTIAL      0u  /**< 1: Task ID is sequential, 0: task ID is random */

#endif /* _INCLUDE_PIFS_OS_H_ */
