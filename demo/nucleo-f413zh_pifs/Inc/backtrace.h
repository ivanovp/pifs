/**
 * @file        backtrace.h
 * @brief       Backtrace printer's prototypes
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created      2017-08-25 17:48:53
 * Last modify: 2017-08-28 18:45:17 ivanovp {Time-stamp}
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
#ifndef INCLUDE_BACKTRACE_H
#define INCLUDE_BACKTRACE_H

#ifdef __cplusplus
extern "C" {
#endif

void print_backtrace(void);

#ifdef __cplusplus
}
#endif

#endif // INCLUDE_BACKTRACE_H
