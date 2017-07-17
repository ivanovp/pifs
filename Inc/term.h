/**
 * @file        term.h
 * @brief       Terminal command handler
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-27 19:35:38 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_TERM_H_
#define _INCLUDE_TERM_H_

#include <stdint.h>

#include "common.h"
#include "parser.h"

extern parserCommand_t parserCommands[];

void term_init (void);
void term_task (void);

#endif /* _INCLUDE_TERM_H_ */
