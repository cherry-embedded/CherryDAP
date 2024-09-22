#ifndef HSLINK_PRO_USB2UART_H
#define HSLINK_PRO_USB2UART_H

#define PIN_UART_TX IOC_PAD_PA08
#define PIN_UART_RX IOC_PAD_PA09
#define PIN_UART_RTS IOC_PAD_PA07
#define PIN_UART_DTR IOC_PAD_PA06

void uartx_preinit(void);

void usb2uart_handler(void);

#endif //HSLINK_PRO_USB2UART_H
