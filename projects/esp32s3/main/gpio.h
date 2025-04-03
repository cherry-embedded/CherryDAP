#pragma once
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

enum
{
    GPIO_INPUT_MODE = 0, /*!< INPUT NO PULL */
    GPIO_OUTPUT_MODE,    /*!< OUTPUT MODE NO PULL */
    GPIO_INPUT_PP_MODE,  /*!< INPUT PULL UP */
};

static inline void gpio_set_mode(uint32_t pin, uint32_t mode)
{
    if (mode == GPIO_INPUT_MODE)
    {
        gpio_set_direction(pin,  GPIO_MODE_INPUT);
    }
    else if (mode == GPIO_OUTPUT_MODE)
    {
        gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    }
    else if (mode == GPIO_INPUT_PP_MODE)
    {
        gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
        gpio_set_direction(pin,  GPIO_MODE_INPUT);
        
    }


}

static inline void gpio_write(uint32_t pin, uint32_t level)
{
    gpio_set_level(pin, level);
}

static inline int gpio_read(uint32_t pin)
{
    return gpio_get_level(pin);
}
