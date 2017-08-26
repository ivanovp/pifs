/**
 * @file        pifs_os.h
 * @brief       OS functions for Pi file system
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-08-26 07:10:19
 * Last modify: 2017-08-26 08:01:38 ivanovp {Time-stamp}
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
#include "cmsis_os.h"

#include "common.h"
#include "pifs_config.h"
#include "pifs.h"

/** Task delay is used when flash initialization fails. */
#define PIFS_OS_DELAY_MS(ms)               osDelay(ms)
#define PIFS_OS_DEFINE_MUTEX(mutex)        osMutexDef(mutex)
#define PIFS_OS_CREATE_MUTEX(mutex)        osMutexCreate(mutex)
#define PIFS_OS_DELETE_MUTEX(mutex)        osMutexDelete(mutex)
#define PIFS_OS_GET_MUTEX(mutex)           osMutexWait(mutex,osWaitForever)
#define PIFS_OS_PUT_MUTEX(mutex)           osMutexRelease(mutex)
#define PIFS_OS_MUTEX_TYPE                 osMutexId

#endif /* _INCLUDE_PIFS_OS_H_ */
