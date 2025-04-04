#include <stdio.h>
// #include "board.h"
#include "dap_main.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "usb_log.h"

#define PATTERN_CHR_NUM    (3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/
#define EX_UART_NUM UART_NUM_1
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static const char *TAG = "usb2uart";
static QueueHandle_t uart0_queue;
static TaskHandle_t *uart_task_handle = NULL;

void chry_dap_usb2uart_uart_config_callback(struct cdc_line_coding *line_coding);

static void uart_event_task(void *pvParameters)
{
    ESP_LOGI(TAG, "uart_event_task start", EX_UART_NUM);
    uart_event_t event;
    size_t buffered_size;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
    if (!dtmp) {
        ESP_LOGE(TAG, "Failed to allocate buffer");
        vTaskDelete(NULL);
        return;
    }
    for (;;) {
        uart_get_buffered_data_len(UART_NUM_1, &buffered_size);
        if (buffered_size > 0)
        {
            bzero(dtmp, RD_BUF_SIZE);
            uart_read_bytes(EX_UART_NUM, dtmp, buffered_size, 20 / portTICK_PERIOD_MS); 
            usbd_ep_start_write(0, CDC_IN_EP, (uint8_t *)dtmp, buffered_size);
            // ESP_LOGI(TAG, "[UART DATA]: %d: '%s'", buffered_size, dtmp);
            // if (buffered_size < 256)
            //     ESP_LOG_BUFFER_HEXDUMP(TAG, dtmp, buffered_size, ESP_LOG_INFO);
        }
        vTaskDelay(1);
        //Waiting for UART event.
        if (xQueueReceive(uart0_queue, (void *)&event, (TickType_t)1)) {
            bzero(dtmp, RD_BUF_SIZE);
            // ESP_LOGI(TAG, "uart[%d] event: %d", EX_UART_NUM, event.type);
            switch (event.type) {
            //Event of UART receiving data
            /*We'd better handler data event fast, there would be much more data events than
            other types of events. If we take too much time on data event, the queue might
            be full.*/
            case UART_DATA:
                // uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                // ESP_LOGI(TAG, "[UART DATA]: %d: %s", event.size, dtmp);
                // usbd_ep_start_write(0, CDC_IN_EP, (uint8_t *)dtmp, event.size);
                // // uart_write_bytes(EX_UART_NUM, (const char*) dtmp, event.size);
                break;
            //Event of HW FIFO overflow detected
            case UART_FIFO_OVF:
                ESP_LOGI(TAG, "hw fifo overflow");
                // If fifo overflow happened, you should consider adding flow control for your application.
                // The ISR has already reset the rx FIFO,
                // As an example, we directly flush the rx buffer here in order to read more data.
                uart_flush_input(EX_UART_NUM);
                xQueueReset(uart0_queue);
                break;
            //Event of UART ring buffer full
            case UART_BUFFER_FULL:
                ESP_LOGI(TAG, "ring buffer full");
                // If buffer full happened, you should consider increasing your buffer size
                // As an example, we directly flush the rx buffer here in order to read more data.
                uart_flush_input(EX_UART_NUM);
                xQueueReset(uart0_queue);
                break;
            //Event of UART RX break detected
            case UART_BREAK:
                // struct cdc_line_coding line_coding = {
                //     dwDTERate: 115200,
                //     bCharFormat: 0,
                //     bParityType: UART_PARITY_DISABLE,
                //     bDataBits: 8
                // };
                // chry_dap_usb2uart_uart_config_callback(&line_coding);
                ESP_LOGI(TAG, "uart rx break");
                break;
            //Event of UART parity check error
            case UART_PARITY_ERR:
                ESP_LOGI(TAG, "uart parity error");
                break;
            //Event of UART frame error
            case UART_FRAME_ERR:
                ESP_LOGI(TAG, "uart frame error");
                break;
            //UART_PATTERN_DET
            case UART_PATTERN_DET:
                uart_get_buffered_data_len(EX_UART_NUM, &buffered_size);
                int pos = uart_pattern_pop_pos(EX_UART_NUM);
                ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                if (pos == -1) {
                    // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
                    // record the position. We should set a larger queue size.
                    // As an example, we directly flush the rx buffer here.
                    uart_flush_input(EX_UART_NUM);
                } else {
                    uart_read_bytes(EX_UART_NUM, dtmp, pos, 100 / portTICK_PERIOD_MS);
                    uint8_t pat[PATTERN_CHR_NUM + 1];
                    memset(pat, 0, sizeof(pat));
                    uart_read_bytes(EX_UART_NUM, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
                    ESP_LOGI(TAG, "read data: %s", dtmp);
                    ESP_LOGI(TAG, "read pat : %s", pat);
                }
                break;
            //Others
            default:
                ESP_LOGI(TAG, "uart event type: %d", event.type);
                break;
            }
        }
    }
    ESP_LOGI(TAG, "uart_event_task stop", EX_UART_NUM);
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

void uartx_preinit(void)
{
    struct cdc_line_coding line_coding = {
        dwDTERate: 115200,
        bCharFormat: 0,
        bParityType: UART_PARITY_DISABLE,
        bDataBits: 8
    };
    chry_dap_usb2uart_uart_config_callback(&line_coding);
    xTaskCreate(uart_event_task, "uart_event_task", 3072, NULL, 12, uart_task_handle);
}

static volatile bool lock = 0;

void chry_dap_usb2uart_uart_config_callback(struct cdc_line_coding *line_coding)
{
    uart_config_t uart_config;

    uart_config.flags.allow_pd = 0;
    uart_config.flags.backup_before_sleep = 0;

    uart_config.baud_rate = line_coding->dwDTERate;
    uart_config.data_bits = line_coding->bDataBits ? line_coding->bDataBits - 5 : UART_DATA_8_BITS;
    uart_config.stop_bits = line_coding->bDataBits ? line_coding->bCharFormat + 1 : UART_STOP_BITS_1;
    // uart_config.parity = line_coding->bParityType;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_APB;
    // uart_config.source_clk = UART_SCLK_DEFAULT;
    uart_config.rx_flow_ctrl_thresh = 122;

    USB_LOG_INFO("config uart, data bit %d bDataBits %d\r\n", uart_config.data_bits, line_coding->bDataBits);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, DAP_UART_TX, DAP_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (!uart_is_driver_installed(UART_NUM_1))
        uart_driver_install(UART_NUM_1, CONFIG_UARTRX_RINGBUF_SIZE, CONFIG_USBRX_RINGBUF_SIZE, 20, &uart0_queue, 0);
    //Set uart pattern detect function.
    // uart_enable_pattern_det_baud_intr(UART_NUM_1, '+', 3, 9, 0, 0);
    //Reset the pattern queue length to record at most 20 pattern positions.
    // uart_pattern_queue_reset(UART_NUM_1, 20);
}

void chry_dap_usb2uart_uart_send_bydma(uint8_t *data, uint16_t len)
{
    // USB_LOG_INFO("usb2uart send by dma\r\n");
    // if (len < 256)
    //     ESP_LOG_BUFFER_HEXDUMP(TAG, data, len, ESP_LOG_WARN);
    uart_write_bytes(UART_NUM_1, (const char *) data, len);
    chry_dap_usb2uart_uart_send_complete(len);
}
