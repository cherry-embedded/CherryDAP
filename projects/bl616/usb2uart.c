#include "bflb_dma.h"
#include "bflb_uart.h"
#include "bflb_gpio.h"
#include "usb2uart.h"
#include "bl616_glb.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "dap_main.h"

static ATTR_NOCACHE_NOINIT_RAM_SECTION uint8_t uart_rx_dma_buf[UART_RX_DMA_BUF_SIZE];
static __ALIGNED(32) struct bflb_dma_channel_lli_pool_s rx_llipool[20];
static __ALIGNED(32) struct bflb_dma_channel_lli_pool_s tx_llipool[20];

struct bflb_device_s *g_gpio = NULL;
static struct bflb_device_s *uartx = NULL;
static struct bflb_device_s *dma0_rx_chx = NULL;
static struct bflb_device_s *dma0_tx_chx = NULL;

static struct bflb_rx_cycle_dma g_uart_rx_dma;

volatile uint32_t g_uart_tx_transfer_length = 0;

ATTR_TCM_SECTION void uart_rx_dma_copy(uint8_t *data, uint32_t len)
{
    chry_ringbuffer_write(&g_uartrx, data, len);
}

ATTR_TCM_SECTION static void uartx_isr(int irq, void *arg)
{
    uint32_t intstatus = bflb_uart_get_intstatus(uartx);
    if (intstatus & UART_INTSTS_RTO) {
        bflb_rx_cycle_dma_process(&g_uart_rx_dma, 0);
        bflb_uart_int_clear(uartx, UART_INTCLR_RTO);
    }
}

ATTR_TCM_SECTION static void uart_rx_dma_complete(void *arg)
{
    bflb_rx_cycle_dma_process(&g_uart_rx_dma, 1);
}

ATTR_TCM_SECTION void uart_tx_dma_complete(void *arg)
{
    chry_dap_usb2uart_uart_send_complete(g_uart_tx_transfer_length);
}

void uartx_preinit(void)
{
    uartx = bflb_device_get_by_name("uart1");
    dma0_rx_chx = bflb_device_get_by_name("dma0_ch0");
    dma0_tx_chx = bflb_device_get_by_name("dma0_ch1");

    g_gpio = bflb_device_get_by_name("gpio");

    bflb_gpio_uart_init(g_gpio, UART_TX_PIN, GPIO_UART_FUNC_UART1_TX);
    bflb_gpio_uart_init(g_gpio, UART_RX_PIN, GPIO_UART_FUNC_UART1_RX);
}

void chry_dap_usb2uart_uart_config_callback(struct cdc_line_coding *line_coding)
{
    GLB_AHB_MCU_Software_Reset(GLB_AHB_MCU_SW_UART1);

    bflb_irq_disable(uartx->irq_num);
    bflb_dma_channel_stop(dma0_rx_chx);
    bflb_dma_channel_stop(dma0_tx_chx);

    struct bflb_uart_config_s cfg;

    cfg.baudrate = line_coding->dwDTERate;
    cfg.data_bits = line_coding->bDataBits - 5;
    cfg.stop_bits = line_coding->bCharFormat + 1;
    cfg.parity = line_coding->bParityType;
    cfg.flow_ctrl = 0;
    cfg.tx_fifo_threshold = 7;
    cfg.rx_fifo_threshold = 0;
    bflb_uart_init(uartx, &cfg);
    bflb_uart_feature_control(uartx, UART_CMD_SET_RTO_VALUE, 0x80);

    bflb_irq_attach(uartx->irq_num, uartx_isr, NULL);
    bflb_irq_enable(uartx->irq_num);
    bflb_uart_link_txdma(uartx, true);
    bflb_uart_link_rxdma(uartx, true);

    struct bflb_dma_channel_config_s config;

    config.direction = DMA_MEMORY_TO_PERIPH;
    config.src_req = DMA_REQUEST_NONE;
    config.dst_req = DMA_REQUEST_UART1_TX;
    config.src_addr_inc = DMA_ADDR_INCREMENT_ENABLE;
    config.dst_addr_inc = DMA_ADDR_INCREMENT_DISABLE;
    config.src_burst_count = DMA_BURST_INCR1;
    config.dst_burst_count = DMA_BURST_INCR1;
    config.src_width = DMA_DATA_WIDTH_8BIT;
    config.dst_width = DMA_DATA_WIDTH_8BIT;

    bflb_dma_channel_init(dma0_tx_chx, &config);
    bflb_dma_channel_irq_attach(dma0_tx_chx, uart_tx_dma_complete, NULL);

    struct bflb_dma_channel_config_s rxconfig;

    rxconfig.direction = DMA_PERIPH_TO_MEMORY;
    rxconfig.src_req = DMA_REQUEST_UART1_RX;
    rxconfig.dst_req = DMA_REQUEST_NONE;
    rxconfig.src_addr_inc = DMA_ADDR_INCREMENT_DISABLE;
    rxconfig.dst_addr_inc = DMA_ADDR_INCREMENT_ENABLE;
    rxconfig.src_burst_count = DMA_BURST_INCR1;
    rxconfig.dst_burst_count = DMA_BURST_INCR1;
    rxconfig.src_width = DMA_DATA_WIDTH_8BIT;
    rxconfig.dst_width = DMA_DATA_WIDTH_8BIT;

    bflb_dma_channel_init(dma0_rx_chx, &rxconfig);
    bflb_dma_channel_irq_attach(dma0_rx_chx, uart_rx_dma_complete, NULL);

    bflb_rx_cycle_dma_init(&g_uart_rx_dma,
                           dma0_rx_chx,
                           rx_llipool,
                           20,
                           DMA_ADDR_UART1_RDR,
                           uart_rx_dma_buf,
                           UART_RX_DMA_BUF_SIZE,
                           uart_rx_dma_copy);

    bflb_dma_channel_start(dma0_rx_chx);
}

void chry_dap_usb2uart_uart_send_bydma(uint8_t *data, uint16_t len)
{
    struct bflb_dma_channel_lli_transfer_s tx_transfers;

    tx_transfers.src_addr = (uint32_t)data;
    tx_transfers.dst_addr = (uint32_t)DMA_ADDR_UART1_TDR;
    tx_transfers.nbytes = len;
    g_uart_tx_transfer_length = len;

    // tx_llipool[0].control.WORD = bflb_dma_feature_control(dma0_tx_chx, DMA_CMD_GET_LLI_CONTROL, 0);
    // tx_llipool[0].src_addr = (uint32_t)data;
    // if (id == 0) {
    //     tx_llipool[0].dst_addr = (uint32_t)DMA_ADDR_UART0_TDR;
    // } else {
    //     tx_llipool[0].dst_addr = (uint32_t)DMA_ADDR_UART1_TDR;
    // }
    // tx_llipool[0].nextlli = 0;
    // tx_llipool[0].control.bits.TransferSize = len;
    // tx_llipool[0].control.bits.I = 1;

    bflb_dma_channel_stop(dma0_tx_chx);
    //bflb_dma_feature_control(dma0_tx_chx, DMA_CMD_SET_LLI_CONFIG, (uint32_t)&tx_llipool[0]);
    bflb_dma_channel_lli_reload(dma0_tx_chx, tx_llipool, 20, &tx_transfers, 1);
    bflb_dma_channel_start(dma0_tx_chx);
}