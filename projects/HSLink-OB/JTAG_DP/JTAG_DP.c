#include "JTAG_DP.h"
#include "setting.h"

void PORT_JTAG_SETUP (void)
{
    if (HSLink_Setting.jtag_port_mode == PORT_MODE_SPI) {
        SPI_PORT_JTAG_SETUP();
    } else {
        IO_PORT_JTAG_SETUP();
    }
}

void JTAG_Sequence (uint32_t info, const uint8_t *tdi, uint8_t *tdo)
{
    if (HSLink_Setting.jtag_port_mode == PORT_MODE_SPI) {
        SPI_JTAG_Sequence(info, tdi, tdo);
    } else {
        IO_JTAG_Sequence(info, tdi, tdo);
    }
}

uint32_t JTAG_ReadIDCode (void)
{
    if (HSLink_Setting.jtag_port_mode == PORT_MODE_SPI) {
        return SPI_JTAG_ReadIDCode();
    } else {
        return IO_JTAG_ReadIDCode();
    }
}

void JTAG_WriteAbort (uint32_t data)
{
    if (HSLink_Setting.jtag_port_mode == PORT_MODE_SPI) {
        SPI_JTAG_WriteAbort(data);
    } else {
        IO_JTAG_WriteAbort(data);
    }
}

void JTAG_IR (uint32_t ir)
{
    if (HSLink_Setting.jtag_port_mode == PORT_MODE_SPI) {
        SPI_JTAG_IR(ir);
    } else {
        IO_JTAG_IR(ir);
    }
}

uint8_t JTAG_Transfer(uint32_t request, uint32_t *data)
{
    if (HSLink_Setting.jtag_port_mode == PORT_MODE_SPI)
    {
        return SPI_JTAG_Transfer(request, data);
    }
    else
    {
        return IO_JTAG_Transfer(request, data);
    }
}

