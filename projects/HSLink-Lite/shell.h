/*
 * Copyright (c) 2022, Egahp
 * Copyright (c) 2024, HPMicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_H
#define SHELL_H

#include "hpm_uart_drv.h"
#include "csh.h"

extern int shell_init(UART_Type *uart, bool need_login);
extern int shell_main(void);
extern void shell_uart_isr(void);
extern void shell_lock(void);
extern void shell_unlock(void);

#endif
