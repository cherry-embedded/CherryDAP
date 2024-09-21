#include "SW_DP.h"

PORT_Mode_t SWD_Port_Mode = PORT_MODE_SPI;

void PORT_SWD_SETUP(void)
{
    if (SWD_Port_Mode == PORT_MODE_SPI) {
        SPI_PORT_SWD_SETUP();
    } else {
        IO_PORT_SWD_SETUP();
    }
}

void SWJ_Sequence(uint32_t count, const uint8_t *data)
{
    if (SWD_Port_Mode == PORT_MODE_SPI) {
        SPI_SWJ_Sequence(count, data);
    } else {
        IO_SWJ_Sequence(count, data);
    }
}

void SWD_Sequence(uint32_t info, const uint8_t *swdo, uint8_t *swdi)
{
    if (SWD_Port_Mode == PORT_MODE_SPI) {
        SPI_SWD_Sequence(info, swdo, swdi);
    } else {
        IO_SWD_Sequence(info, swdo, swdi);
    }
}

uint8_t SWD_Transfer(uint32_t request, uint32_t *data)
{
    if (SWD_Port_Mode == PORT_MODE_SPI) {
        return SPI_SWD_Transfer(request, data);
    } else {
        return IO_SWD_Transfer(request, data);
    }
}
