#ifndef HSLINK_PRO_USB2UART_H
#define HSLINK_PRO_USB2UART_H

#ifdef __cplusplus
extern "C"
{
#endif

void uartx_io_init(void);

void uartx_preinit(void);

void usb2uart_handler(void);

#ifdef __cplusplus
}
#endif

#endif //HSLINK_PRO_USB2UART_H
