#include "board.h"
#include "hpm_clock_drv.h"
#include "hpm_uart_drv.h"
#ifdef CONFIG_HAS_HPMSDK_DMAV2
#include "hpm_dmav2_drv.h"
#else
#include "hpm_dma_drv.h"
#endif
#include "hpm_dmamux_drv.h"
#include "hpm_l1c_drv.h"
#include "hpm_common.h"
#include "dap_main.h"

#define APP_UART                   HPM_UART3
#define APP_UART_IRQ               IRQn_UART3
#define APP_UART_CLK_NAME          clock_uart3
#define APP_UART_TX_DMA_REQ        HPM_DMA_SRC_UART3_TX
#define APP_UART_RX_DMA_REQ        HPM_DMA_SRC_UART3_RX

#define APP_UART_DMA_CONTROLLER    HPM_HDMA
#define APP_UART_DMAMUX_CONTROLLER HPM_DMAMUX
#define APP_UART_TX_DMA_CHN        (0U)
#define APP_UART_RX_DMA_CHN        (1U)
#define APP_UART_RX_DMAMUX_CHN     DMA_SOC_CHN_TO_DMAMUX_CHN(APP_UART_DMA_CONTROLLER, APP_UART_RX_DMA_CHN)
#define APP_UART_TX_DMAMUX_CHN     DMA_SOC_CHN_TO_DMAMUX_CHN(APP_UART_DMA_CONTROLLER, APP_UART_TX_DMA_CHN)
#define APP_UART_DMA_IRQ           IRQn_HDMA

#define BUFF_SIZE                  (10 * 1024)
static hpm_stat_t uart_tx_trigger_dma(DMA_Type *dma_ptr, uint8_t ch_num,
                                      UART_Type *uart_ptr, uint32_t src,
                                      uint32_t size);

static hpm_stat_t uart_rx_trigger_dma(DMA_Type *dma_ptr, uint8_t ch_num,
                                      UART_Type *uart_ptr, uint32_t dst,
                                      uint32_t size);

static void configure_uart_dma_transfer(void);

ATTR_PLACE_AT_NONCACHEABLE uint8_t dma_rx_buff[BUFF_SIZE];
volatile uint32_t g_uart_tx_transfer_length = 0;

void uart_isr(void)
{
    uint32_t uart_received_data_count;
    if (uart_is_rxline_idle(APP_UART)) {
        uart_clear_rxline_idle_flag(APP_UART);
        uart_received_data_count = BUFF_SIZE - dma_get_remaining_transfer_size(APP_UART_DMA_CONTROLLER, APP_UART_RX_DMA_CHN);
        if (uart_received_data_count > 0) {
            dma_disable_channel(APP_UART_DMA_CONTROLLER, APP_UART_RX_DMA_CHN);
            chry_ringbuffer_write(&g_uartrx, dma_rx_buff, uart_received_data_count);
            configure_uart_dma_transfer();
        }
    }
}
SDK_DECLARE_EXT_ISR_M(APP_UART_IRQ, uart_isr)

void dma_isr(void)
{
    volatile hpm_stat_t stat_tx_chn;
    volatile hpm_stat_t stat_rx_chn;
    stat_tx_chn = dma_check_transfer_status(APP_UART_DMA_CONTROLLER, APP_UART_TX_DMA_CHN);
    stat_rx_chn = dma_check_transfer_status(APP_UART_DMA_CONTROLLER, APP_UART_RX_DMA_CHN);
    if (stat_tx_chn & DMA_CHANNEL_STATUS_TC) {
        chry_dap_usb2uart_uart_send_complete(g_uart_tx_transfer_length);
    }
    if (stat_rx_chn & DMA_CHANNEL_STATUS_TC) {
        chry_ringbuffer_write(&g_uartrx, dma_rx_buff, BUFF_SIZE);
        configure_uart_dma_transfer();
    }
}
SDK_DECLARE_EXT_ISR_M(APP_UART_DMA_IRQ, dma_isr)

static hpm_stat_t uart_tx_trigger_dma(DMA_Type *dma_ptr,
                    uint8_t ch_num,
                    UART_Type *uart_ptr,
                    uint32_t src,
                    uint32_t size)
{
    dma_handshake_config_t config;

    dma_default_handshake_config(dma_ptr, &config);
    config.ch_index = ch_num;
    config.dst = (uint32_t)&uart_ptr->THR;
    config.dst_fixed = true;
    config.src = src;
    config.src_fixed = false;
    config.data_width = DMA_TRANSFER_WIDTH_BYTE;
    config.size_in_byte = size;

    return dma_setup_handshake(dma_ptr, &config, true);
}

static hpm_stat_t uart_rx_trigger_dma(DMA_Type *dma_ptr,
                                uint8_t ch_num,
                                UART_Type *uart_ptr,
                                uint32_t dst,
                                uint32_t size)
{
    dma_handshake_config_t config;

    dma_default_handshake_config(dma_ptr, &config);
    config.ch_index = ch_num;
    config.dst = dst;
    config.dst_fixed = false;
    config.src = (uint32_t)&uart_ptr->RBR;
    config.src_fixed = true;
    config.data_width = DMA_TRANSFER_WIDTH_BYTE;
    config.size_in_byte = size;

    return dma_setup_handshake(dma_ptr, &config, true);
}

/* config dma to receive uart data */
static void configure_uart_dma_transfer(void)
{
    uart_rx_trigger_dma(APP_UART_DMA_CONTROLLER,
                            APP_UART_RX_DMA_CHN,
                            APP_UART,
                            core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)dma_rx_buff),
                            BUFF_SIZE);
}

void uartx_preinit(void)
{
    hpm_stat_t stat;
    board_init_uart(APP_UART);
    clock_set_source_divider(APP_UART_CLK_NAME, clk_src_pll1_clk0, 10);
    clock_add_to_group(APP_UART_CLK_NAME, 0);
    intc_m_enable_irq_with_priority(APP_UART_IRQ, 2);
    intc_m_enable_irq_with_priority(APP_UART_DMA_IRQ, 1);
    dmamux_config(APP_UART_DMAMUX_CONTROLLER, APP_UART_TX_DMAMUX_CHN, APP_UART_TX_DMA_REQ, true);
    dmamux_config(APP_UART_DMAMUX_CONTROLLER, APP_UART_RX_DMAMUX_CHN, APP_UART_RX_DMA_REQ, true);
    uart_clear_rxline_idle_flag(APP_UART);
}

void chry_dap_usb2uart_uart_config_callback(struct cdc_line_coding *line_coding)
{
    uart_config_t config = {0};
    uart_default_config(APP_UART, &config);
    config.baudrate = line_coding->dwDTERate;
    config.parity = line_coding->bParityType;
    config.word_length = line_coding->bDataBits - 5;
    config.num_of_stop_bits = line_coding->bCharFormat;
    config.fifo_enable = true;
    config.dma_enable = true;
    config.src_freq_in_hz = clock_get_frequency(APP_UART_CLK_NAME);
    config.rx_fifo_level = uart_rx_fifo_trg_not_empty; /* this config should not change */
    config.tx_fifo_level = uart_tx_fifo_trg_not_full;
    config.rxidle_config.detect_enable = true;
    config.rxidle_config.detect_irq_enable = true;
    config.rxidle_config.idle_cond = uart_rxline_idle_cond_state_machine_idle;
    config.rxidle_config.threshold = 10U; /* 20bit */
    uart_init(APP_UART, &config);
    uart_clear_rxline_idle_flag(APP_UART);
    configure_uart_dma_transfer();
}

void chry_dap_usb2uart_uart_send_bydma(uint8_t *data, uint16_t len)
{
    if (len <= 0) {
        return;
    }
    g_uart_tx_transfer_length = len;
    dma_disable_channel(APP_UART_DMA_CONTROLLER, APP_UART_TX_DMA_CHN);
    uart_tx_trigger_dma(APP_UART_DMA_CONTROLLER,
                                    APP_UART_TX_DMA_CHN,
                                    APP_UART,
                                    core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)data),
                                    len);
}
