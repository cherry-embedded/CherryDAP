#ifndef _USB2UART_H
#define _USB2UART_H

#define UART_TX_PIN 11
#define UART_RX_PIN 13

#define UART_RX_DMA_BUF_SIZE (16 * 1024)

void uartx_preinit(void);
void uart_write_bydma(uint8_t id, uint8_t *data, uint32_t len);

#endif