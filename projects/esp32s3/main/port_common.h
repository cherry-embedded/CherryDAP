#pragma once
#include <stdint.h>
#include "gpio.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/uart_select.h"

#ifndef __STATIC_INLINE
#define __STATIC_INLINE static inline
#endif

#ifndef __WEAK
#define __WEAK __attribute__((weak))
#endif

#define PIN_SWCLK_TCK               GPIO_NUM_17
#define PIN_SWDIO_TMS               GPIO_NUM_16
#define PIN_TDI                     GPIO_NUM_15
#define PIN_TDO                     GPIO_NUM_7
#define PIN_nRESET                  GPIO_NUM_6
// #define LED_CONNECTED               GPIO_NUM_41
// #define LED_RUNNING                 GPIO_NUM_42

#define DAP_UART_TX                 GPIO_NUM_4
#define DAP_UART_RX                 GPIO_NUM_5

#define USART_UX                    UART_NUM_0
#define DEV_UART0_TX                GPIO_NUM_16
#define DEV_UART0_RX                GPIO_NUM_3
#define RX_BUF_SIZE                 1024

#define DAP_CPU_CLOCK 0U

void dap_platform_init(void);
void dap_gpio_init(void);

void set_led_connect(uint32_t bit);
void set_led_running(uint32_t bit);
uint32_t get_led_connect(void);
uint32_t get_led_running(void);


static inline uint32_t dap_get_time_stamp(void)
{
    return (uint32_t)xTaskGetTickCount();
}
