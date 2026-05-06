#ifndef HSLINK_PRO_SW_DP_H
#define HSLINK_PRO_SW_DP_H

#include "DAP_config.h"

#if ((DAP_SWD != 0) || (DAP_JTAG != 0))
void IO_SWJ_Sequence(uint32_t count, const uint8_t *data);
void SPI_SWJ_Sequence(uint32_t count, const uint8_t *data);
#endif

#if (DAP_SWD != 0)
void IO_PORT_SWD_SETUP (void);
void IO_SWD_Sequence(uint32_t info, const uint8_t *swdo, uint8_t *swdi);
uint8_t IO_SWD_Transfer(uint32_t request, uint32_t *data);

void SPI_PORT_SWD_SETUP(void);
void SPI_SWD_Sequence(uint32_t info, const uint8_t *swdo, uint8_t *swdi);
uint8_t SPI_SWD_Transfer(uint32_t request, uint32_t *data);
#endif

#endif //HSLINK_PRO_SW_DP_H
