/**
 * @file        term.h
 * @brief       Terminal command handler
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-27 19:35:38 ivanovp {Time-stamp}
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
#ifndef _INCLUDE_TERM_H_
#define _INCLUDE_TERM_H_

#include <stdint.h>

#include "api_pifs.h"
#include "common.h"
#include "parser.h"

extern parserCommand_t parserCommands[];
extern pifs_status_t pifs_status;

void term_init (void);
void term_task (void);

#endif /* _INCLUDE_TERM_H_ */
