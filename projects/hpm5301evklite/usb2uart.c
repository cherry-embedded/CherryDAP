#include <stdio.h>
#include "board.h"
#include "hpm_uart_drv.h"
#include "hpm_debug_console.h"
#include "hpm_dma_mgr.h"
#include "hpm_sysctl_drv.h"
#include "dap_main.h"

#define UART_BASE                     BOARD_APP_UART_BASE
#define UART_IRQ                      BOARD_APP_UART_IRQ
#define UART_CLK_NAME                 BOARD_APP_UART_CLK_NAME
#define UART_RX_DMA                   BOARD_APP_UART_RX_DMA_REQ
#define UART_RX_DMA_RESOURCE_INDEX    (0U)
#define UART_RX_DMA_BUFFER_SIZE       (8192U)
#define UART_RX_DMA_BUFFER_COUNT      (5)

#define UART_TX_DMA                   BOARD_APP_UART_TX_DMA_REQ
#define UART_TX_DMA_RESOURCE_INDEX    (1U)
#define UART_TX_DMA_BUFFER_SIZE       (8192U)

ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(4) uint8_t uart_tx_buf[UART_TX_DMA_BUFFER_SIZE];
ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(4) uint8_t uart_rx_buf[UART_RX_DMA_BUFFER_COUNT][UART_RX_DMA_BUFFER_SIZE];
ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(8) dma_linked_descriptor_t rx_descriptors[UART_RX_DMA_BUFFER_COUNT - 1];

static dma_resource_t dma_resource_pools[2];
volatile uint32_t g_uart_tx_transfer_length = 0;
static hpm_stat_t board_uart_dma_config(void);
static hpm_stat_t board_uart_rx_dma_restart(void);

void dma_channel_tc_callback(DMA_Type *ptr, uint32_t channel, void *user_data)
{
    (void)ptr;
    (void)channel;
    (void)user_data;
    uint32_t i;
    dma_resource_t *rx_resource = &dma_resource_pools[UART_RX_DMA_RESOURCE_INDEX];
    dma_resource_t *tx_resource = &dma_resource_pools[UART_TX_DMA_RESOURCE_INDEX];
    uint32_t link_addr = ptr->CHCTRL[channel].LLPOINTER;
    uint32_t rx_desc_size = (sizeof(rx_descriptors) / sizeof(dma_linked_descriptor_t));
    if (rx_resource->channel == channel) {
        if (link_addr == NULL) {
            board_uart_rx_dma_restart();
            return;
        }
        for (i = 0; i < rx_desc_size; i++) {
            if (link_addr == core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&rx_descriptors[i])) {
                chry_ringbuffer_write(&g_uartrx, uart_rx_buf[i - 1], UART_RX_DMA_BUFFER_SIZE);
            }
        }
    }
    if (tx_resource->channel == channel) {
        chry_dap_usb2uart_uart_send_complete(g_uart_tx_transfer_length);
    }
}


void uart_isr(void)
{
    uint32_t uart_received_data_count;
    uint32_t i;
    dma_resource_t *rx_resource = &dma_resource_pools[UART_RX_DMA_RESOURCE_INDEX];
    uint32_t link_addr = rx_resource->base->CHCTRL[rx_resource->channel].LLPOINTER;
    uint32_t rx_desc_size = (sizeof(rx_descriptors) / sizeof(dma_linked_descriptor_t));
    if (uart_is_rxline_idle(UART_BASE)) {
        uart_clear_rxline_idle_flag(UART_BASE);
        uart_received_data_count = UART_RX_DMA_BUFFER_SIZE - dma_get_remaining_transfer_size(rx_resource->base, rx_resource->channel);
        if (uart_received_data_count > 0) {
            for (i = 0; i < rx_desc_size; i++) {
                if (link_addr == core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&rx_descriptors[i])) {
                    chry_ringbuffer_write(&g_uartrx, uart_rx_buf[i], uart_received_data_count);
                    board_uart_rx_dma_restart();
                    break;
                }
            }
        }
    }
}
SDK_DECLARE_EXT_ISR_M(UART_IRQ, uart_isr)



void uartx_preinit(void)
{
    board_init_uart(UART_BASE);
    clock_set_source_divider(UART_CLK_NAME, clk_src_pll1_clk0, 8);
    clock_add_to_group(UART_CLK_NAME, 0);
    intc_m_enable_irq_with_priority(UART_IRQ, 2);
    uart_clear_rxline_idle_flag(UART_BASE);
    dma_mgr_init();
    if (board_uart_dma_config() != status_success) {
        return;
    }
}

void chry_dap_usb2uart_uart_config_callback(struct cdc_line_coding *line_coding)
{
    uart_config_t config = {0};
    uart_default_config(UART_BASE, &config);
    config.baudrate = line_coding->dwDTERate;
    config.parity = line_coding->bParityType;
    config.word_length = line_coding->bDataBits - 5;
    config.num_of_stop_bits = line_coding->bCharFormat;
    config.fifo_enable = true;
    config.dma_enable = true;
    config.src_freq_in_hz = clock_get_frequency(UART_CLK_NAME);
    config.rx_fifo_level = uart_rx_fifo_trg_not_empty; /* this config should not change */
    config.tx_fifo_level = uart_tx_fifo_trg_not_full;
    config.rxidle_config.detect_enable = true;
    config.rxidle_config.detect_irq_enable = true;
    config.rxidle_config.idle_cond = uart_rxline_idle_cond_state_machine_idle;
    config.rxidle_config.threshold = 30U; /* 20bit */
    uart_init(UART_BASE, &config);
    uart_clear_rxline_idle_flag(UART_BASE);
    uart_reset_rx_fifo(UART_BASE);
    uart_reset_tx_fifo(UART_BASE);
}

void chry_dap_usb2uart_uart_send_bydma(uint8_t *data, uint16_t len)
{
    uint32_t buf_addr;
    dma_resource_t *tx_resource = &dma_resource_pools[UART_TX_DMA_RESOURCE_INDEX];
    if (len <= 0) {
        return;
    }
    g_uart_tx_transfer_length = len;
    buf_addr = core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)data);
    dma_mgr_set_chn_src_addr(tx_resource, buf_addr);
    dma_mgr_set_chn_transize(tx_resource, len);
    dma_mgr_enable_channel(tx_resource);
}


static hpm_stat_t board_uart_rx_dma_restart(void)
{
    dma_resource_t *resource = &dma_resource_pools[UART_RX_DMA_RESOURCE_INDEX];
    dma_mgr_disable_channel(resource);
    uint32_t addr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)uart_rx_buf[0]);
    dma_mgr_set_chn_dst_addr(resource, addr);
    addr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&rx_descriptors[0]);
    resource->base->CHCTRL[resource->channel].LLPOINTER = addr;
    dma_mgr_set_chn_transize(resource, UART_RX_DMA_BUFFER_SIZE);
    dma_mgr_enable_channel(resource);
    return  status_success;
}
static hpm_stat_t board_uart_dma_config(void)
{
    dma_mgr_chn_conf_t chg_config;
    dma_resource_t *resource = NULL;
    uint32_t i = 0;
    uint32_t rx_desc_size = (sizeof(rx_descriptors) / sizeof(dma_linked_descriptor_t));
    dma_mgr_get_default_chn_config(&chg_config);
    chg_config.src_width = DMA_MGR_TRANSFER_WIDTH_BYTE;
    chg_config.dst_width = DMA_MGR_TRANSFER_WIDTH_BYTE;
    /* uart rx dma config */
    resource = &dma_resource_pools[UART_RX_DMA_RESOURCE_INDEX];
    if (dma_mgr_request_resource(resource) == status_success) {
        chg_config.src_mode = DMA_MGR_HANDSHAKE_MODE_HANDSHAKE;
        chg_config.src_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
        chg_config.src_addr = (uint32_t)&UART_BASE->RBR;
        chg_config.dst_mode = DMA_MGR_HANDSHAKE_MODE_NORMAL;
        chg_config.dst_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_INCREMENT;
        chg_config.size_in_byte = UART_RX_DMA_BUFFER_SIZE;
        chg_config.en_dmamux = true;
        chg_config.dmamux_src = UART_RX_DMA;
        for (i = 0; i < rx_desc_size; i++) {
            if (i < (rx_desc_size - 1)) {
                chg_config.dst_addr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)uart_rx_buf[i + 1]);
                chg_config.linked_ptr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&rx_descriptors[i + 1]);
            } else {
                chg_config.dst_addr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)uart_rx_buf[1]);
                chg_config.linked_ptr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&rx_descriptors[0]);
            }
            if (dma_mgr_config_linked_descriptor(resource, &chg_config, (dma_mgr_linked_descriptor_t *)&rx_descriptors[i]) != status_success) {
                printf("generate dma desc fail\n");
                return status_fail;
            }
            rx_descriptors[i].ctrl &= ~DMA_MGR_INTERRUPT_MASK_TC;
            printf("linker_addr:0x%08x  %d\n", (uint32_t)&rx_descriptors[i], i);
        }
        chg_config.dst_addr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)uart_rx_buf[0]);
        chg_config.linked_ptr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&rx_descriptors[0]);
        dma_mgr_setup_channel(resource, &chg_config);
        dma_mgr_enable_channel(resource);
        dma_mgr_install_chn_tc_callback(resource, dma_channel_tc_callback, NULL);
        dma_mgr_enable_chn_irq(resource, DMA_MGR_INTERRUPT_MASK_TC);
        dma_mgr_enable_dma_irq_with_priority(resource, 1);
    }
     /* uart tx dma config */
    resource = &dma_resource_pools[UART_TX_DMA_RESOURCE_INDEX];
    if (dma_mgr_request_resource(resource) == status_success) {
        chg_config.src_mode = DMA_MGR_HANDSHAKE_MODE_NORMAL;
        chg_config.src_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_INCREMENT;
        chg_config.dst_mode = DMA_MGR_HANDSHAKE_MODE_HANDSHAKE;
        chg_config.dst_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
        chg_config.dst_addr =(uint32_t)&UART_BASE->THR;
        chg_config.en_dmamux = true;
        chg_config.dmamux_src = UART_TX_DMA;
        chg_config.linked_ptr = NULL;
        dma_mgr_setup_channel(resource, &chg_config);
        dma_mgr_install_chn_tc_callback(resource, dma_channel_tc_callback, NULL);
        dma_mgr_enable_chn_irq(resource, DMA_MGR_INTERRUPT_MASK_TC);
        dma_mgr_enable_dma_irq_with_priority(resource, 1);
    }
    return status_success;
}

