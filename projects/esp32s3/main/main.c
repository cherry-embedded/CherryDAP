#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "usb_config.h"
#include "driver/usb_serial_jtag.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_check.h"
#include "dap_main.h"
#include "usb2uart.h"
#include "usbd_core.h"
#include "usbd_cdc.h"

void app_main() {
    uartx_preinit();
    chry_dap_init(0, ESP_USBD_BASE);
    while (1) {
        chry_dap_handle();
        chry_dap_usb2uart_handle();
    }
}