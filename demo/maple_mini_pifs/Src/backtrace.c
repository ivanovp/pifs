/**
 * @file        backtrace.c
 * @brief       Backtrace printer's implementation
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created      2017-08-25 17:48:53
 * Last modify: 2017-08-28 18:45:20 ivanovp {Time-stamp}
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
#include <stdint.h>
#include <unwind.h>

#include "common.h"
#include "uart.h"

_Unwind_Reason_Code trace_func(_Unwind_Context *a_context, void *a_depth)
{
    int * depth = (int*) a_depth;

    UART_printf("Depth %i: PC @0x%08X\r\n", *depth, _Unwind_GetIP(a_context));
    (*depth)++;
    return _URC_NO_REASON;
}

void print_backtrace(void)
{
    int depth = 0;

    UART_printf("Call stack:\r\n");
    _Unwind_Backtrace(&trace_func, &depth);
}

