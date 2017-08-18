/**
 * @file        uart.h
 * @brief       Prototype of UART functions
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created      2017-06-13 11:48:53
 * Last modify: 2017-06-16 19:07:42 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef __UART_H
#define __UART_H

#include <stdint.h>

extern void UART_putc(const char ch);
extern void UART_printf_(const char * fmt, ...);
extern void UART_printf(const char * fmt, ...);
extern uint8_t UART_getchar(void);

/* Fallback functions for exception handlers */
extern void _putchar(char c);
extern void _print (char * s);
extern void _printHex32 (uint32_t aNum);
extern void _printHex16 (uint16_t aNum);
extern void _printHex8 (uint8_t aNum);

#endif /* __UART_H */
