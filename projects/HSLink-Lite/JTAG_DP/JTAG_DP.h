#ifndef HSLINK_PRO_JTAG_DP_SPI_H
#define HSLINK_PRO_JTAG_DP_SPI_H

#include "DAP_config.h"

#if (DAP_JTAG != 0)

void IO_PORT_JTAG_SETUP(void);
void IO_JTAG_Sequence (uint32_t info, const uint8_t *tdi, uint8_t *tdo);
uint32_t IO_JTAG_ReadIDCode (void);
void IO_JTAG_WriteAbort (uint32_t data);
void IO_JTAG_IR (uint32_t ir);
uint8_t  IO_JTAG_Transfer(uint32_t request, uint32_t *data);


void SPI_PORT_JTAG_SETUP(void);
void SPI_JTAG_Sequence (uint32_t info, const uint8_t *tdi, uint8_t *tdo);
uint32_t SPI_JTAG_ReadIDCode (void);
void SPI_JTAG_WriteAbort (uint32_t data);
void SPI_JTAG_IR (uint32_t ir);
uint8_t  SPI_JTAG_Transfer(uint32_t request, uint32_t *data);

#endif

#endif //HSLINK_PRO_JTAG_DP_SPI_H
