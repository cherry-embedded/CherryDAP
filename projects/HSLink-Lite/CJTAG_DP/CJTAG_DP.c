/*
 * Copyright (c) 2025 runcheng,lu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "CJTAG_DP.h"

// #define PIN_CJTAG_TCK_SET PIN_SWCLK_TCK_SET
// #define PIN_CJTAG_TCK_CLR PIN_SWCLK_TCK_CLR
// #define PIN_CJTAG_TMSC_IN PIN_SWDIO_IN
// #define PIN_CJTAG_TMSC_SET PIN_SWDIO_TMS_SET
// #define PIN_CJTAG_TMSC_CLR PIN_SWDIO_TMS_CLR
// #define PIN_CJTAG_TMSC_OUT PIN_SWDIO_OUT

static void CJTAG_hpm_user_connect_seq(void);
static void CJTAG_hpm_jlink_standard_connect_seq(void);

volatile uint32_t id_core_value = 0;

#define CJTAG_CYCLE_TCK()                                                     \
    PIN_CJTAG_TCK_CLR();                                                      \
    PIN_DELAY();                                                              \
    PIN_CJTAG_TCK_SET();                                                      \
    PIN_DELAY()

/* nTDI means that inverted TDI is being transmitted */
#define CJTAG_TAP_CYCLE(tms, ntdi, tdo)                                         \
    PIN_CJTAG_TCK_CLR();                                                       \
    PIN_CJTAG_TMSC_OUT(ntdi);                                                   \
    PIN_DELAY();                                                               \
    PIN_CJTAG_TCK_SET();                                                       \
    PIN_DELAY();                                                               \
                                                                               \
    PIN_CJTAG_TCK_CLR();                                                       \
    PIN_CJTAG_TMSC_OUT(tms);                                                   \
    PIN_DELAY();                                                               \
    PIN_CJTAG_TCK_SET();                                                       \
    PIN_DELAY();                                                               \
                                                                               \
    PIN_CJTAG_TMSC_DIR_IN();                                                   \
    PIN_CJTAG_TCK_CLR();                                                       \
    PIN_DELAY();                                                               \
    tdo = PIN_CJTAG_TMSC_IN();                                                 \
    PIN_CJTAG_TCK_SET();                                                       \
    PIN_DELAY();                                                               \
    PIN_CJTAG_TMSC_DIR_OUT();                                                  \

/* In TAP.7, only TMS and CLK represent the TAP state; TDI and TDO are disabled. */
#define CJTAG_TAP7_CYCLE(tmsc)                                                 \
    PIN_CJTAG_TCK_CLR();                                                       \
    PIN_CJTAG_TMSC_OUT(tmsc);                                                  \
    PIN_DELAY();                                                               \
    PIN_CJTAG_TCK_SET();                                                       \
    PIN_DELAY()


#define TAP1_TEST_LOGIC_RESET       0
#define TAP1_RUN_TEST_IDLE          1
#define TAP1_SELECT_DR_SCAN         2
#define TAP1_CAPTURE_DR             3
#define TAP1_SHIFT_DR               4
#define TAP1_EXIT1_DR               5
#define TAP1_PAUSE_DR               6
#define TAP1_EXIT2_DR               7
#define TAP1_UPDATE_DR              8
#define TAP1_SELECT_IR_SCAN         9
#define TAP1_CAPTURE_IR            10
#define TAP1_SHIFT_IR              11
#define TAP1_EXIT1_IR              12
#define TAP1_PAUSE_IR              13
#define TAP1_EXIT2_IR              14
#define TAP1_UPDATE_IR             15

/* TAP.7 Control Levels */
#define TAP7_CTRL_LVL_ZBS_STL            1  /* ZBSs used by STL or other function */
#define TAP7_CTRL_LVL_CMD                2  /* TAP.7 command generation */
#define TAP7_CTRL_LVL_RESERVED           3  /* Reserved */
#define TAP7_CTRL_LVL_SCAN_PATHS_4       4  /* Optional Controller Scan Paths */
#define TAP7_CTRL_LVL_SCAN_PATHS_5       5  /* Optional Controller Scan Paths */
#define TAP7_CTRL_LVL_FORCE_OFFLINE      6  /* Force Offline (optional) */
#define TAP7_CTRL_LVL_DTS                7  /* DTS uses */

/* store command type*/
#define CMD_STORE_STMC                   0  /* Store Miscellaneous Control */
#define CMD_STORE_STC1                   1  /* Store Conditional one-bit */
#define CMD_STORE_STC2                   2  /* Store Conditional two-bit */
#define CMD_STORE_STFMT                  3  /* Store Format */
#define CMD_STORE_STTPST                 4  /* Store Transport state */
#define CMD_STORE_STTESTM                5  /* Store Test Mode */
#define CMD_SELECT_MCM                   6  /* Make Cond. Group Member */
#define CMD_SELECT_MSC                   7  /* Make Scan Group Candidate*/

#define PIN_DELAY() PIN_DELAY_SLOW(DAP_Data.clock_delay)

#ifndef ENABLE_CJTAG_READ_IDCORE
#define ENABLE_CJTAG_READ_IDCORE         0 /* Enable CJTAG Read IDCODE */
#endif

#ifndef ENABLE_CJTAG_READ_USERCODE
#define ENABLE_CJTAG_READ_USERCODE       0 /* Enable CJTAG Read USERCODE */
#endif

#ifndef ENABLE_CJTAG_DOBYPASS
#define ENABLE_CJTAG_DOBYPASS            0 /* Enable CJTAG Do Bypass */
#endif

volatile static uint8_t current_tap_state = TAP1_TEST_LOGIC_RESET;

static void cjtag_pin_init(void)
{
    HPM_IOC->PAD[PIN_TCK].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0); /* as gpio*/
    HPM_IOC->PAD[PIN_TMS].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0); /* as gpio*/
    HPM_IOC->PAD[PIN_JTAG_TRST].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[PIN_SRST].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[PIN_TCK].PAD_CTL = IOC_PAD_PAD_CTL_PRS_SET(2) | IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1) | IOC_PAD_PAD_CTL_SPD_SET(3);
    HPM_IOC->PAD[PIN_TMS].PAD_CTL = IOC_PAD_PAD_CTL_PRS_SET(2) | IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1) | IOC_PAD_PAD_CTL_SPD_SET(3);
    HPM_IOC->PAD[PIN_JTAG_TRST].PAD_CTL = IOC_PAD_PAD_CTL_PRS_SET(2) | IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1) | IOC_PAD_PAD_CTL_SPD_SET(3);
    HPM_IOC->PAD[PIN_SRST].PAD_CTL = IOC_PAD_PAD_CTL_PRS_SET(2) | IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1) | IOC_PAD_PAD_CTL_SPD_SET(3);

    gpiom_configure_pin_control_setting_reset(IOC_PAD_PA26);
    gpiom_configure_pin_control_setting_reset(IOC_PAD_PA27);
    gpiom_configure_pin_control_setting_reset(IOC_PAD_PA28);
    gpiom_configure_pin_control_setting(PIN_TCK);
    gpiom_configure_pin_control_setting(PIN_TMS);
    gpiom_configure_pin_control_setting(PIN_JTAG_TRST);
    gpiom_configure_pin_control_setting(PIN_SRST);

    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TCK), GPIO_GET_PIN_INDEX(PIN_TCK));
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TMS), GPIO_GET_PIN_INDEX(PIN_TMS));
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_JTAG_TRST), GPIO_GET_PIN_INDEX(PIN_JTAG_TRST));
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_SRST), GPIO_GET_PIN_INDEX(PIN_SRST));

    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_SRST), GPIO_GET_PIN_INDEX(PIN_SRST), !HSLink_Global.reset_level);

    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1); // 默认SWDIO为输出
}

#if ENABLE_CJTAG_READ_USERCODE
uint32_t CJTAG_ReadUsercode(void)
{
    uint32_t val = 0;
    uint32_t n;
    volatile uint32_t tdo_bit;

    CJTAG_TAP_CYCLE(0, 1, tdo_bit); // RUN-TEST-IDLE, nTDI=1
    CJTAG_TAP_CYCLE(1, 1, tdo_bit); // Select-DR-Scan
    CJTAG_TAP_CYCLE(1, 1, tdo_bit); // Select-IR-Scan
    CJTAG_TAP_CYCLE(0, 1, tdo_bit); // Capture-IR
    CJTAG_TAP_CYCLE(0, 1, tdo_bit); // Shift-IR

    // IR=0x00
    CJTAG_TAP_CYCLE(0, 1, tdo_bit); // bit0=0 (nTDI=1)
    CJTAG_TAP_CYCLE(0, 1, tdo_bit); // bit1=0 (nTDI=1)
    CJTAG_TAP_CYCLE(0, 1, tdo_bit); // bit2=0 (nTDI=1)
    CJTAG_TAP_CYCLE(0, 1, tdo_bit); // bit3=0 (nTDI=1)
    CJTAG_TAP_CYCLE(1, 1, tdo_bit); // bit4=0 (nTDI=1), TMS=1, exit Shift-IR

    CJTAG_TAP_CYCLE(1, 1, tdo_bit); // Update-IR
    CJTAG_TAP_CYCLE(0, 1, tdo_bit); // Idle

    CJTAG_TAP_CYCLE(1, 1, tdo_bit); // Select-DR-Scan
    CJTAG_TAP_CYCLE(0, 1, tdo_bit); // Capture-DR
    CJTAG_TAP_CYCLE(0, 1, tdo_bit); // Shift-DR
    val = 0;
    for (n = 0; n < 31; n++) {
        CJTAG_TAP_CYCLE(0, 1, tdo_bit);
        val |= (tdo_bit & 0x1) << n;
    }
    CJTAG_TAP_CYCLE(1, 1, tdo_bit);
    val |= (tdo_bit & 0x1) << 31;

    CJTAG_TAP_CYCLE(1, 1, tdo_bit); // Update-DR
    CJTAG_TAP_CYCLE(0, 1, tdo_bit); // Idle

    return val;
}
#endif

#if ENABLE_CJTAG_READ_IDCORE
uint32_t CJTAG_IDcode(void)
{
    uint32_t val = 0;
    uint32_t n;
    volatile uint32_t tdo_bit;
    CJTAG_TAP_CYCLE(0, 1, tdo_bit); // Idle

    CJTAG_TAP_CYCLE(1, 1, tdo_bit); // Select-DR-Scan
    CJTAG_TAP_CYCLE(0, 1, tdo_bit); // Capture-DR
    CJTAG_TAP_CYCLE(0, 1, tdo_bit); // Shift-DR
    val = 0;
    for (n = 0; n < 31; n++) {
        CJTAG_TAP_CYCLE(0, 1, tdo_bit);
        val |= (tdo_bit & 0x1) << n;
    }
    CJTAG_TAP_CYCLE(1, 1, tdo_bit);
    val |= (tdo_bit & 0x1) << 31;

    CJTAG_TAP_CYCLE(1, 1, tdo_bit); // Update-DR
    CJTAG_TAP_CYCLE(0, 1, tdo_bit); // Idle

    return val;
}
#endif

#if ENABLE_CJTAG_DOBYPASS
void CJTAG_DoBypass(void)
{
    volatile uint32_t tdo_bit __attribute__((unused));
    uint32_t ir_len = 5;
    uint32_t dr_len = 672;
    CJTAG_TAP_CYCLE(0, 0, tdo_bit); // RUN-TEST-IDLE
    CJTAG_TAP_CYCLE(1, 0, tdo_bit); // Select-DR-Scan
    CJTAG_TAP_CYCLE(1, 0, tdo_bit); // Select-IR-Scan
    CJTAG_TAP_CYCLE(0, 0, tdo_bit); // Capture-IR
    CJTAG_TAP_CYCLE(0, 0, tdo_bit); // Shift-IR

    for (uint32_t i = 0; i < ir_len - 1; i++) {
        CJTAG_TAP_CYCLE(0, 0, tdo_bit);
    }
    CJTAG_TAP_CYCLE(1, 0, tdo_bit);

    CJTAG_TAP_CYCLE(1, 0, tdo_bit); // Update-IR
    CJTAG_TAP_CYCLE(0, 0, tdo_bit); // Idle

    CJTAG_TAP_CYCLE(0, 0, tdo_bit); // RUN-TEST-IDLE
    CJTAG_TAP_CYCLE(1, 0, tdo_bit); // Select-DR-Scan
    CJTAG_TAP_CYCLE(0, 0, tdo_bit); // Capture-DR
    CJTAG_TAP_CYCLE(0, 0, tdo_bit); // Shift-DR

    for (uint32_t i = 0; i < dr_len - 1; i++) {
        CJTAG_TAP_CYCLE(0, 0, tdo_bit);
    }
    CJTAG_TAP_CYCLE(1, 0, tdo_bit);

    CJTAG_TAP_CYCLE(1, 0, tdo_bit); // Update-DR
    CJTAG_TAP_CYCLE(0, 0, tdo_bit); // Idle
}
#endif

static void TAP1_change_state(bool tms_sta)
{
    switch (current_tap_state) {
        case TAP1_TEST_LOGIC_RESET:
            current_tap_state = tms_sta ? TAP1_TEST_LOGIC_RESET : TAP1_RUN_TEST_IDLE;
            break;
        case TAP1_RUN_TEST_IDLE:
            current_tap_state = tms_sta ? TAP1_SELECT_DR_SCAN : TAP1_RUN_TEST_IDLE;
            break;
        case TAP1_SELECT_DR_SCAN:
            current_tap_state = tms_sta ? TAP1_SELECT_IR_SCAN : TAP1_CAPTURE_DR;
            break;
        case TAP1_CAPTURE_DR:
            current_tap_state = tms_sta ? TAP1_EXIT1_DR : TAP1_SHIFT_DR;
            break;
        case TAP1_SHIFT_DR:
            current_tap_state = tms_sta ? TAP1_EXIT1_DR : TAP1_SHIFT_DR;
            break;
        case TAP1_EXIT1_DR:
            current_tap_state = tms_sta ? TAP1_UPDATE_DR : TAP1_PAUSE_DR;
            break;
        case TAP1_PAUSE_DR:
            current_tap_state = tms_sta ? TAP1_EXIT2_DR : TAP1_PAUSE_DR;
            break;
        case TAP1_EXIT2_DR:
            current_tap_state = tms_sta ? TAP1_UPDATE_DR : TAP1_SHIFT_DR;
            break;
        case TAP1_UPDATE_DR:
            current_tap_state = tms_sta ? TAP1_SELECT_DR_SCAN : TAP1_RUN_TEST_IDLE;
            break;
        case TAP1_SELECT_IR_SCAN:
            current_tap_state = tms_sta ? TAP1_TEST_LOGIC_RESET : TAP1_CAPTURE_IR;
            break;
        case TAP1_CAPTURE_IR:
            current_tap_state = tms_sta ? TAP1_EXIT1_IR : TAP1_SHIFT_IR;
            break;
        case TAP1_SHIFT_IR:
            current_tap_state = tms_sta ? TAP1_EXIT1_IR : TAP1_SHIFT_IR;
            break;
        case TAP1_EXIT1_IR:
            current_tap_state = tms_sta ? TAP1_UPDATE_IR : TAP1_PAUSE_IR;
            break;
        case TAP1_PAUSE_IR:
            current_tap_state = tms_sta ? TAP1_EXIT2_IR : TAP1_PAUSE_IR;
            break;
        case TAP1_EXIT2_IR:
            current_tap_state = tms_sta ? TAP1_UPDATE_IR : TAP1_SHIFT_IR;
            break;
        case TAP1_UPDATE_IR:
            current_tap_state = tms_sta ? TAP1_SELECT_DR_SCAN : TAP1_RUN_TEST_IDLE;
            break;
        default:
            current_tap_state = TAP1_TEST_LOGIC_RESET;
            break;
    }
}

void PORT_CJTAG_SETUP(void)
{
    uint32_t usercode __attribute__((unused));
    uint32_t tdo_bit __attribute__((unused));
    cjtag_pin_init();
#if USE_SHORT_CONNECT_SEQ
    CJTAG_short_connect_seq();
#if ENABLE_CJTAG_READ_IDCORE
    board_delay_us(50);
    id_core_value = CJTAG_IDcode();
#endif
#else
#if USE_JLINK_STANDARD_SEQUENCE
    CJTAG_hpm_jlink_standard_connect_seq();
#else
    CJTAG_hpm_user_connect_seq();
#endif
#if ENABLE_CJTAG_READ_IDCORE
    board_delay_us(50);
    id_core_value = CJTAG_IDcode();
#endif
#endif
    current_tap_state = TAP1_RUN_TEST_IDLE;
}


// Generate JTAG Sequence
//   info:   sequence information
//   tdi:    pointer to TDI generated data
//   tdo:    pointer to TDO captured data
//   return: none
void CJTAG_Sequence(uint32_t info, const uint8_t *tdi, uint8_t *tdo)
{
    static uint8_t tms_reset_count = 0;
#if USE_OSCAN1_NOT_LOGIC_RESET
    static uint8_t tms_buffer[3];
    static uint8_t tdi_buffer[3];
    static bool tdo_buffer[3];
    static uint32_t tck_count[3];
#endif
    uint32_t i_val;
    uint32_t o_val;
    uint32_t bit;
    uint32_t n, k;
#if USE_OSCAN1_NOT_LOGIC_RESET
    uint32_t m;
#endif
    bool is_tms_high;
    uint8_t ntdi;

    n = info & JTAG_SEQUENCE_TCK;
    if (n == 0U) {
      n = 64U;
    }
    TAP1_change_state((info & JTAG_SEQUENCE_TMS) >> 6);
    if (info & JTAG_SEQUENCE_TMS) {
        is_tms_high = true;
#if USE_OSCAN1_NOT_LOGIC_RESET
        tms_reset_count++;
        tck_count[tms_reset_count - 1] = n;
        tms_buffer[tms_reset_count - 1] = 1;
        tdi_buffer[tms_reset_count - 1] = *tdi++;
        if (info & JTAG_SEQUENCE_TDO) {
            tdo_buffer[tms_reset_count - 1] = true;
        } else {
            tdo_buffer[tms_reset_count - 1] = false;
        }
        if (tms_reset_count >= 3) {
            if (current_tap_state == TAP1_TEST_LOGIC_RESET) {
                /* Move to Test-Logic-Reset state  no use*/
                memset(tms_buffer, 0, sizeof(tms_buffer));
                tms_reset_count = 0;
#if ENABLE_CJTAG_READ_IDCORE
                CJTAG_IDcode(); // Read IDCODE
#endif
                return;
            } else {
                goto tms_handle;
            }
        } else {
            return;
        }
#else
        tms_reset_count++;
#endif
    } else {
        tms_reset_count = 0;
        is_tms_high = false;
#if USE_OSCAN1_NOT_LOGIC_RESET
        if (tms_reset_count) {
            for (m = 0; m < sizeof(tdo_buffer); m++) {
                if (tdo_buffer[m] == true) {
                    tdo --;
                }
            }
            for (m = 0; m < tms_reset_count; m++) {
                while (tck_count[m]) {
                    i_val = tdi_buffer[m];
                    o_val = 0U;
                    for (k = 8U; k && tck_count[m]; k--, tck_count[m]--) {
                        ntdi = (i_val & 1) ? 0 : 1;
                        CJTAG_TAP_CYCLE(tms_buffer[m], ntdi, bit);
                        i_val >>= 1;
                        o_val >>= 1;
                        o_val  |= bit << 7;
                    }
                    o_val >>= k;
                    if (tdo_buffer[m] == true) {
                        *tdo++ = (uint8_t)o_val;
                    }
                }
            }
            tms_reset_count = 0;
        }
#endif
    }

    while (n) {
        i_val = *tdi++;
        o_val = 0U;
        for (k = 8U; k && n; k--, n--) {
            /* nTDI means that inverted TDI is being transmitted */
            ntdi = (i_val & 1) ? 0 : 1;
            CJTAG_TAP_CYCLE(is_tms_high, ntdi, bit);
            i_val >>= 1;
            o_val >>= 1;
            o_val  |= bit << 7;
        }
        o_val >>= k;
        if (info & JTAG_SEQUENCE_TDO) {
            *tdo++ = (uint8_t)o_val;
        }
      }
#if USE_OSCAN1_NOT_LOGIC_RESET
    return;
tms_handle:
    if (tms_reset_count) {
        for (m = 0; m < sizeof(tdo_buffer); m++) {
            if (tdo_buffer[m] == true) {
                tdo --;
            }
        }
        for (m = 0; m < tms_reset_count; m++) {
            while (tck_count[m]) {
                i_val = tdi_buffer[m];
                o_val = 0U;
                for (k = 8U; k && tck_count[m]; k--, tck_count[m]--) {
                    ntdi = (i_val & 1) ? 0 : 1;
                    CJTAG_TAP_CYCLE(tms_buffer[m], ntdi, bit);
                    i_val >>= 1;
                    o_val >>= 1;
                    o_val  |= bit << 7;
                }
                o_val >>= k;
                if (tdo_buffer[m] == true) {
                    *tdo++ = (uint8_t)o_val;
                }
            }
        }
        tms_reset_count = 0;
    }
#else
    if (tms_reset_count >= 3) {
#if !USE_SHORT_CONNECT_SEQ
        if (current_tap_state == TAP1_TEST_LOGIC_RESET) {
#if USE_JLINK_STANDARD_SEQUENCE
            CJTAG_hpm_jlink_standard_connect_seq();
#else
            CJTAG_hpm_user_connect_seq();
#endif
        }
#else
    CJTAG_short_connect_seq();
#endif
        tms_reset_count = 0;
    }
#endif
}

// JTAG Set IR
//   ir:     IR value
//   return: none
#define CJTAG_IR_Function(speed) /**/                                                                 \
static void CJTAG_IR_##speed(uint32_t ir)                                                             \
{                                                                                                    \
    uint32_t n;                                                                                      \
    volatile uint32_t tdo_bit __attribute__((unused));                                               \
    volatile uint32_t ntdi_bit = 1;                                                                  \
    CJTAG_TAP_CYCLE(true, ntdi_bit, tdo_bit);   /* Select-DR-Scan */                                 \
    CJTAG_TAP_CYCLE(true, ntdi_bit, tdo_bit);   /* Select-IR-Scan */                                 \
    CJTAG_TAP_CYCLE(false, ntdi_bit, tdo_bit);   /* Capture-IR */                                    \
    CJTAG_TAP_CYCLE(false, ntdi_bit, tdo_bit);   /* Shift-IR */                                      \
    ntdi_bit = 0;                                                                                    \
    for (n = DAP_Data.jtag_dev.ir_before[DAP_Data.jtag_dev.index]; n; n--) {                         \
        CJTAG_TAP_CYCLE(false, ntdi_bit, tdo_bit);                /* Bypass before data */           \
    }                                                                                                \
    for (n = DAP_Data.jtag_dev.ir_length[DAP_Data.jtag_dev.index] - 1U; n; n--) {                    \
        ntdi_bit = (ir & 1) ? 0 : 1;                                                                 \
        CJTAG_TAP_CYCLE(false, ntdi_bit, tdo_bit);               /* Set IR bits (except last) */     \
        ir >>= 1;                                                                                    \
    }                                                                                                \
    n = DAP_Data.jtag_dev.ir_after[DAP_Data.jtag_dev.index];                                         \
    if (n) {                                                                                         \
        ntdi_bit = (ir & 1) ? 0 : 1;                                                                 \
        CJTAG_TAP_CYCLE(false, ntdi_bit, tdo_bit);                     /* Set last IR bit */         \
        ntdi_bit = 0;                                                                                \
        for (--n; n; n--) {                                                                          \
            CJTAG_TAP_CYCLE(false, ntdi_bit, tdo_bit);  /* Bypass after data */                      \
        }                                                                                            \
        CJTAG_TAP_CYCLE(true, ntdi_bit, tdo_bit);      /* Bypass & Exit1-IR */                       \
    } else {                                                                                         \
        ntdi_bit = (ir & 1) ? 0 : 1;                                                                 \
        CJTAG_TAP_CYCLE(true, ntdi_bit, tdo_bit);      /* Set last IR bit & Exit1-IR */              \
    }                                                                                                \
    CJTAG_TAP_CYCLE(true, ntdi_bit, tdo_bit);    /* Update-IR */                                     \
    CJTAG_TAP_CYCLE(false, ntdi_bit, tdo_bit);                                                       \
    CJTAG_TAP_CYCLE(false, 0, tdo_bit);         /* Idle */                                           \
}


// JTAG Transfer I/O
//   request: A[3:2] RnW APnDP
//   data:    DATA[31:0]
//   return:  ACK[2:0]
#define CJTAG_TransferFunction(speed)        /**/                                                     \
static uint8_t CJTAG_Transfer##speed (uint32_t request, uint32_t *data)                               \
{                                                                                                    \
    uint32_t ack;                                                                                    \
    uint32_t bit;                                                                                    \
    uint32_t val;                                                                                    \
    uint32_t n;                                                                                      \
    volatile uint32_t tdo_bit __attribute__((unused));                                               \
    volatile uint32_t ntdi_bit = 1;                                                                  \
                                                                                                     \
    CJTAG_TAP_CYCLE(true, ntdi_bit, tdo_bit);                         /* Select-DR-Scan */           \
    CJTAG_TAP_CYCLE(false, ntdi_bit, tdo_bit);                         /* Capture-DR */              \
    CJTAG_TAP_CYCLE(false, ntdi_bit, tdo_bit);                         /* Shift-DR */                \
                                                                                                     \
    for (n = DAP_Data.jtag_dev.index; n; n--) {                                                      \
        CJTAG_TAP_CYCLE(false, ntdi_bit, tdo_bit);                   /* Bypass before data */        \
    }                                                                                                \
                                                                                                     \
    ntdi_bit = ((request >> 1) & 0x01) ? 0 : 1;                                                      \
    CJTAG_TAP_CYCLE(false, ntdi_bit, bit);       /* Set RnW, Get ACK.0 */                            \
    ack  = bit << 1;                                                                                 \
    ntdi_bit = ((request >> 2) & 0x01) ? 0 : 1;                                                      \
    CJTAG_TAP_CYCLE(false, ntdi_bit, bit);       /* Set A2,  Get ACK.1 */                            \
    ack |= bit << 0;                                                                                 \
    ntdi_bit = ((request >> 3) & 0x01) ? 0 : 1;                                                      \
    CJTAG_TAP_CYCLE(false, ntdi_bit, bit);       /* Set A3,  Get ACK.2 */                            \
    ack |= bit << 2;                                                                                 \
                                                                                                     \
    if (ack != DAP_TRANSFER_OK) {                                                                    \
        /* Exit on error */                                                                          \
        CJTAG_TAP_CYCLE(true, 1, tdo_bit);                       /* Exit1-DR */                      \
        goto exit;                                                                                   \
    }                                                                                                \
                                                                                                     \
    if (request & DAP_TRANSFER_RnW) {                                                                \
        /* Read Transfer */                                                                          \
        val = 0U;                                                                                    \
        for (n = 31U; n; n--) {                                                                      \
        CJTAG_TAP_CYCLE(false, 1, bit);                  /* Get D0..D30 */                           \
        val  |= bit << 31;                                                                           \
        val >>= 1;                                                                                   \
        }                                                                                            \
        n = DAP_Data.jtag_dev.count - DAP_Data.jtag_dev.index - 1U;                                  \
        if (n) {                                                                                     \
            CJTAG_TAP_CYCLE(false, 1, val);                  /* Get D31 */                           \
        for (--n; n; n--) {                                                                          \
            CJTAG_TAP_CYCLE(false, 1, tdo_bit);                   /* Bypass after data */            \
        }                                                                                            \
            CJTAG_TAP_CYCLE(true, 1, tdo_bit);                     /* Bypass & Exit1-DR */           \
        } else {                                                                                     \
            CJTAG_TAP_CYCLE(true, 1, bit);                  /* Get D31 & Exit1-DR */                 \
        }                                                                                            \
        val |= bit << 31;                                                                            \
        if (data) { *data = val; }                                                                   \
    } else {                                                                                         \
        /* Write Transfer */                                                                         \
        val = *data;                                                                                 \
        for (n = 31U; n; n--) {                                                                      \
            ntdi_bit = (val & 1) ? 0 : 1;                                                            \
            CJTAG_TAP_CYCLE(false, ntdi_bit, tdo_bit);                  /* Set D0..D30 */            \
            val >>= 1;                                                                               \
        }                                                                                            \
        n = DAP_Data.jtag_dev.count - DAP_Data.jtag_dev.index - 1U;                                  \
        if (n) {                                                                                     \
            ntdi_bit = (val & 1) ? 0 : 1;                                                            \
            CJTAG_TAP_CYCLE(false, ntdi_bit, tdo_bit);                  /* Set D31 */                \
        for (--n; n; n--) {                                                                          \
            CJTAG_TAP_CYCLE(false, 1, tdo_bit);                   /* Bypass after data */            \
        }                                                                                            \
        CJTAG_TAP_CYCLE(true, 1, tdo_bit);                     /* Bypass & Exit1-DR */               \
        } else {                                                                                     \
            ntdi_bit = (val & 1) ? 0 : 1;                                                            \
            CJTAG_TAP_CYCLE(true, ntdi_bit, tdo_bit);                  /* Set D31 & Exit1-DR */      \
        }                                                                                            \
    }                                                                                                \
                                                                                                     \
exit:                                                                                                \
    CJTAG_TAP_CYCLE(true, 1, tdo_bit);                         /* Update-DR */                       \
    CJTAG_TAP_CYCLE(false, 1, tdo_bit);                         /* Idle */                           \
                                                                                                     \
    /* Capture Timestamp */                                                                          \
    if (request & DAP_TRANSFER_TIMESTAMP) {                                                          \
        DAP_Data.timestamp = TIMESTAMP_GET();                                                        \
    }                                                                                                \
                                                                                                     \
    /* Idle cycles */                                                                                \
    n = DAP_Data.transfer.idle_cycles;                                                               \
    while (n--) {                                                                                    \
        CJTAG_TAP_CYCLE(false, 0, tdo_bit);                       /* Idle */                         \
    }                                                                                                \
                                                                                                     \
    return ((uint8_t)ack);                                                                           \
}

#undef  PIN_DELAY
#define PIN_DELAY() PIN_DELAY_FAST()
CJTAG_IR_Function(Fast)
CJTAG_TransferFunction(Fast)

#undef  PIN_DELAY
#define PIN_DELAY() PIN_DELAY_SLOW(DAP_Data.clock_delay)
CJTAG_IR_Function(Slow)
CJTAG_TransferFunction(Slow)

// JTAG Read IDCODE register
//   return: value read
uint32_t CJTAG_ReadIDCode (void)
{
    uint32_t val;
    uint32_t n;
    volatile uint32_t tdo_bit __attribute__((unused));
    CJTAG_TAP_CYCLE(true, 1, tdo_bit);                          /* Select-DR-Scan */
    CJTAG_TAP_CYCLE(false, 1, tdo_bit);                         /* Capture-DR */
    CJTAG_TAP_CYCLE(false, 1, tdo_bit);                         /* Shift-DR */

    for (n = DAP_Data.jtag_dev.index; n; n--) {
        CJTAG_TAP_CYCLE(false, 1, tdo_bit);                     /* Bypass before data */
    }

    val = 0U;
    for (n = 31U; n; n--) {
        CJTAG_TAP_CYCLE(false, 0, tdo_bit);                     /* Get D0..D30 */
        val  |= tdo_bit << 31;
        val >>= 1;
    }
    CJTAG_TAP_CYCLE(true, 0, tdo_bit);                      /* Get D31 & Exit1-DR */
    val |= tdo_bit << 31;

    CJTAG_TAP_CYCLE(true, 0, tdo_bit);                         /* Update-DR */
    CJTAG_TAP_CYCLE(false, 0, tdo_bit);                         /* Idle */

    return (val);
}

// JTAG Write ABORT register
//   data:   value to write
//   return: none
void CJTAG_WriteAbort (uint32_t data)
{
    uint32_t n;
    volatile uint32_t tdo_bit __attribute__((unused));
    CJTAG_TAP_CYCLE(true, 1, tdo_bit);                          /* Select-DR-Scan */
    CJTAG_TAP_CYCLE(false, 1, tdo_bit);                         /* Capture-DR */
    CJTAG_TAP_CYCLE(false, 1, tdo_bit);                         /* Shift-DR */

    for (n = DAP_Data.jtag_dev.index; n; n--) {
        CJTAG_TAP_CYCLE(false, 1, tdo_bit);                     /* Bypass before data */
    }

    CJTAG_TAP_CYCLE(false, 1, tdo_bit);                         /* Set RnW=0 (Write) */
    CJTAG_TAP_CYCLE(false, 1, tdo_bit);                         /* Set A2=0 */
    CJTAG_TAP_CYCLE(false, 1, tdo_bit);                         /* Set A3=0 */

    for (n = 31U; n; n--) {
        CJTAG_TAP_CYCLE(false, data, tdo_bit);                   /* Set D0..D30 */
        data >>= 1;
    }
    n = DAP_Data.jtag_dev.count - DAP_Data.jtag_dev.index - 1U;
    if (n) {
        CJTAG_TAP_CYCLE(false, data, tdo_bit);;                   /* Set D31 */
        for (--n; n; n--) {
            CJTAG_TAP_CYCLE(false, 0, tdo_bit);                  /* Bypass after data */
        }
        CJTAG_TAP_CYCLE(true, 0, tdo_bit);                       /* Bypass & Exit1-DR */
    } else {
        CJTAG_TAP_CYCLE(true, 0, tdo_bit);                       /* Set D31 & Exit1-DR */
    }

    CJTAG_TAP_CYCLE(true, 0, tdo_bit);                         /* Update-DR */
    CJTAG_TAP_CYCLE(false, 0, tdo_bit);                         /* Idle */
    CJTAG_TAP_CYCLE(false, 0, tdo_bit);
  }


// JTAG Set IR
//   ir:     IR value
//   return: none
void CJTAG_IR (uint32_t ir)
{
    if (DAP_Data.fast_clock) {
      CJTAG_IR_Fast(ir);
    } else {
      CJTAG_IR_Slow(ir);
    }
}
// JTAG Transfer I/O
//   request: A[3:2] RnW APnDP
//   data:    DATA[31:0]
//   return:  ACK[2:0]
uint8_t  CJTAG_Transfer(uint32_t request, uint32_t *data)
{
    if (DAP_Data.fast_clock) {
      return CJTAG_TransferFast(request, data);
    } else {
      return CJTAG_TransferSlow(request, data);
    }
}


/* Toggle TMS <NumToggle> times while TCK == HIGH */
void CJTAG_write_escape_seq(uint32_t num_toggle)
{
    // PIN_CJTAG_TCK_CLR();
    // PIN_CJTAG_TCK_SET();
    // PIN_CJTAG_TMSC_CLR();
    for (uint32_t i = 0; i < num_toggle; i++)
    {
        PIN_CJTAG_TMSC_OUT_TOGGLE();
        PIN_DELAY();
    }
}

/* Outputs exactly <NumBits> bits on the TMS line, no matter what protocol is currently active */
void CJTAG_write_tmsc(uint32_t data, uint32_t num_bits)
{
    uint8_t j = 0;
    uint32_t _data = data;
    for (uint32_t i = 0; i < num_bits; i++) {
        if (_data & 0x1) {
            PIN_CJTAG_TMSC_SET();
        } else {
            PIN_CJTAG_TMSC_CLR();
        }
        _data >>= 1;
        j++;
        if (j == 32) {
            j = 0;
            _data = data;
        }
        PIN_CJTAG_TCK_CLR();
        PIN_DELAY();
        PIN_CJTAG_TCK_SET();
        PIN_DELAY();
    }
}

/* ZBS
 * A zero-bit DR Scan is a TAPC state sequence that begins with the Select-DR-Scan state and ends with an
 * exit from the Update-DR state without an intervening Shift-DR state
 */
void CJTAG_zero_bit_DR_scans(void)
{
    /* Select-DR-Scan */
    CJTAG_TAP7_CYCLE(1);
    /* Capture-DR */
    CJTAG_TAP7_CYCLE(0);
    /* Exit1-DR */
    CJTAG_TAP7_CYCLE(1);
    /* Update-DR */
    CJTAG_TAP7_CYCLE(1);
    /* Run-Test/Idle */
    CJTAG_TAP7_CYCLE(0);
}

/* Locking the ZBS count
 * When a DR Scan containing a Shift-DR state occurs and the ZBS count is greater than zero, the ZBS count
 * is locked at its current value. ZBSs occurring while the ZBS count is locked do not affect the locked ZBS
 * count. Locking the ZBS count is the equivalent of storing the count for subsequent use
 */
void CJTAG_lock_zbs(void)
{
    /* Select-DR-Scan */
    CJTAG_TAP7_CYCLE(1);
    /* Capture-DR */
    CJTAG_TAP7_CYCLE(0);
    /* Shift-DR */
    CJTAG_TAP7_CYCLE(0);
    /* Exit1-DR */
    CJTAG_TAP7_CYCLE(1);
    /* Update-DR */
    CJTAG_TAP7_CYCLE(1);
    /* Run-Test/Idle */
    CJTAG_TAP7_CYCLE(0);
}

/* Control levels
 * Locking the ZBS count activates a control level equal to the locked ZBS count (1–7) when ZBSs are being
 * used to perform TAP.7 Controller functions and merely locks the ZBS count otherwise
 */
void CJTAG_set_cmd_level(uint8_t level)
{
    if (level < TAP7_CTRL_LVL_ZBS_STL || level > TAP7_CTRL_LVL_DTS) {
        return;
    }
    /* Ensure that the ZBS count is unlocked */
    for (uint8_t i = 0; i < level; i++) {
        CJTAG_zero_bit_DR_scans();
    }
    /* Lock the ZBS count to set the control level */
    CJTAG_lock_zbs();
}

void CJTAG_write_TAP7_cmd_param(uint8_t cmd, uint8_t param)
{
    uint8_t i = 0;
    /* CP1: Generate the MSBs of the command (op-code) using Shift-DR TAPC states */
    /* Select-DR-Scan */
    CJTAG_TAP7_CYCLE(1);
    /* Capture-DR */
    CJTAG_TAP7_CYCLE(0);
    /* Shift-DR */
    for (i = 0; i < cmd; i++) {
        CJTAG_TAP7_CYCLE(0);
    }
    /* On transition to EXIT-DR, the DR shifts once. */
    /* Exit1-DR */
    CJTAG_TAP7_CYCLE(1);
    /* Update-DR */
    CJTAG_TAP7_CYCLE(1);
    /* Run-Test/Idle */
    CJTAG_TAP7_CYCLE(0);
    /* CP2: Generate the LSBs of the command (operand) using Shift-DR TAPC states */
    /* Select-DR-Scan */
    CJTAG_TAP7_CYCLE(1);
    /* Capture-DR */
    CJTAG_TAP7_CYCLE(0);
    /* Shift-DR */
    for (i = 0; i < param; i++) {
        CJTAG_TAP7_CYCLE(0); // Shift-DR
    }
    /* On transition to EXIT-DR, the DR shifts once. */
    /* Exit1-DR */
    CJTAG_TAP7_CYCLE(1);
    /* Update-DR */
    CJTAG_TAP7_CYCLE(1);
    /* Run-Test/Idle */
    CJTAG_TAP7_CYCLE(0);
}


/**
 * @brief Outputs <NumBits> bits via OScan1.
 * For each bit: 1 clock nTDI, 1 clock TMS, 1 clock TDO.
 * Example:
 *   WriteOScan1(0x1B, 0, 6); // Exit command level 2: Idle -> DR-Scan -> IR-Scan -> Capture-IR -> Exit1-IR -> Update-IR -> Idle
 *   WriteOScan1(0, 0, 1);    // Needed for SPA to CPA transition
 */
void CJTAG_write_OScan1(uint32_t TMSData, uint32_t TDIData, uint32_t NumBits)
{
    uint32_t tms = TMSData;
    uint32_t tdi = TDIData;
    volatile uint32_t tdo_bit __attribute__((unused));;

    for (uint32_t i = 0; i < NumBits; i++) {
        // 1 clock nTDI
        // 1 clock TMS
        // 1 clock TDO
        CJTAG_TAP_CYCLE((tms & 1) ? 1 : 0, (tdi & 1) ? 0 : 1, tdo_bit);
        tms >>= 1;
        tdi >>= 1;
    }
}

void CJTAG_write_OScan1_cmd_param(uint8_t cmd, uint8_t param)
{
    uint8_t i;
    uint32_t tdo_bit __attribute__((unused));
    /* CP1: Generate the MSBs of the command (op-code) using Shift-DR TAPC states */
    /*  in all packets, the nTDI bits are equal to one */
    uint32_t tdi_bit = 1;
    /* Select-DR-Scan */
    CJTAG_TAP_CYCLE(1, tdi_bit, tdo_bit);
    /* Capture-DR */
    CJTAG_TAP_CYCLE(0, tdi_bit, tdo_bit);
    /* Shift-DR */
    for (i = 0; i < cmd; i++) {
        CJTAG_TAP_CYCLE(0, tdi_bit, tdo_bit);
    }
    /* Exit1-DR */
    CJTAG_TAP_CYCLE(1, tdi_bit, tdo_bit);
    /* Update-DR */
    CJTAG_TAP_CYCLE(1, tdi_bit, tdo_bit);
    /* Run-Test/Idle */
    CJTAG_TAP_CYCLE(0, tdi_bit, tdo_bit);
    /* CP2: Generate the LSBs of the command (operand) using Shift-DR TAPC states */
    /* Select-DR-Scan */
    CJTAG_TAP_CYCLE(1, tdi_bit, tdo_bit);
    /* Capture-DR */
    CJTAG_TAP_CYCLE(0, tdi_bit, tdo_bit);
    /* Shift-DR */
    for (i = 0; i < param; i++) {
        CJTAG_TAP_CYCLE(0, tdi_bit, tdo_bit); // Shift-DR
    }
    /* On transition to EXIT-DR, the DR shifts once. */
    /* Exit1-DR */
    CJTAG_TAP_CYCLE(1, tdi_bit, tdo_bit);
    /* Update-DR */
    CJTAG_TAP_CYCLE(1, tdi_bit, tdo_bit);
    /* Run-Test/Idle */
    CJTAG_TAP_CYCLE(0, tdi_bit, tdo_bit);
}

void CJTAG_standard_connect_seq(void)
{
    /* scape sequence "Reset": >= 8 TMS line state changes while TCK == HIGH */
    /* cJTAG TAP7 is in JScan0 mode now (TCK + TMS are transmitted) */
    CJTAG_write_escape_seq(10);
    /* >= 22 dummy clocks with TMS == HIGH. TAP: ??? => Reset */
    CJTAG_write_tmsc(0xFFFFFFFF, 25); /* 25 bits of TMS=1 */
    /* 1 clocks with TMS == LOW. TAP: Reset => Run-Test/Idle */
    CJTAG_write_tmsc(0x0, 1); /* 1 bits of TMS=0 */
    /* Escape sequence "Selection" */
    CJTAG_write_escape_seq(7);
    /* Forms of Selection Sequence page277 in ieee11497-2022 */
    /* There are two forms of the TAP.7 Selection Sequence, short and long, as shown in Figure 11-8 */
    /* short: online activation code(OAC)-4bit + extension code(EC)-4bit + check packet-4bit
     * long: OAC-4bit + EC-4bit + 24bit global register init values + check packet-4bit
     * The short form is used to select a specific technology and activate the TAP.7 controller.
     * The long form is used to select a specific technology, activate the TAP.7 controller, and initialize the global registers.
     * The long form is required when switching from one technology to another, as the global registers may need to be reinitialized.
     */

    /* 4-bit OAC sequence: Wake-up TAP7 of all technologies */
    CJTAG_write_tmsc(0x00, 4);
    /* -bit EC sequence: Use long-form selection sequence that also allows to specify the active format in detail */
    CJTAG_write_tmsc(0x00, 4);
    /* Write all 0s for 24-bit global register init values: SCNFMT, DLYC, RDYC, TPST, TPPREV, TP_DELN */
    CJTAG_write_tmsc(0x00000000, 24);
    /* Check packet */
    CJTAG_write_tmsc(0x00, 4);
    /* Set command level to 2 and lock it */
    CJTAG_set_cmd_level(TAP7_CTRL_LVL_CMD);
    /* TC1.SREDGE = 1 */
    CJTAG_write_TAP7_cmd_param(CMD_STORE_STC1, 1);
    /* STFMT[4:0] == 9 => OScan1 */
    CJTAG_write_TAP7_cmd_param(CMD_STORE_STFMT, 9);
    /* Check packet */
    CJTAG_write_tmsc(0x00, 4);
    /* Scan1 protocol is active from now on */
    /* MSC = 0*/
    CJTAG_write_OScan1_cmd_param(CMD_SELECT_MSC, 0);
    /* Needed for SPA to CPA transition */
    CJTAG_write_OScan1(0, 0, 1);
    /* Check packet */
    CJTAG_write_tmsc(0x00, 4);
    /* Exit command level 2: Idle -> DR-Scan -> IR-Scan -> Capture-IR -> Exit1-IR -> Update-IR -> Idle */
    /* JLink will start communicating with the target core itself. OScan1 is used all the time from now on */
    CJTAG_write_OScan1(0x1B, 0, 6);
}

void CJTAG_short_connect_seq(void)
{
    uint32_t tdo_bit __attribute__((unused));
    /* scape sequence "Reset": >= 8 TMS line state changes while TCK == HIGH */
    /* cJTAG TAP7 is in JScan0 mode now (TCK + TMS are transmitted) */
    CJTAG_write_escape_seq(10);
    /* >= 22 dummy clocks with TMS == HIGH. TAP: ??? => Reset */
    CJTAG_write_tmsc(0xFFFFFFFF, 25); /* 25 bits of TMS=1 */
    /* 1 clocks with TMS == LOW. TAP: Reset => Run-Test/Idle */
    CJTAG_write_tmsc(0x0, 1); /* 1 bits of TMS=0 */
    /* Escape sequence "Selection" */
    CJTAG_write_escape_seq(6);
    /* 4-bit OAC sequence: Wake-up 2-wire TAP7 only. Boot with OScan format being active */
    CJTAG_write_tmsc(0x0C, 4);
    /*  4-bit EC sequence */
    CJTAG_write_tmsc(0x08, 4);
    /* Check packet */
    CJTAG_write_tmsc(0x00, 4);
    board_delay_us(3);
    /* OScan1 protocol is active from now on */
    /* J-Link will start communicating with the target core itself. OScan1 is
     * used all the time from now on */
    CJTAG_TAP_CYCLE(0,1,tdo_bit); // Idle
}

#if !USE_JLINK_STANDARD_SEQUENCE && !USE_SHORT_CONNECT_SEQ
static void CJTAG_hpm_user_connect_seq(void)
{
    uint32_t tdo_bit __attribute__((unused));
    CJTAG_write_escape_seq(10);
    CJTAG_write_tmsc(0xFFFFFFFF, 25);
    CJTAG_write_tmsc(0x0, 16);
    CJTAG_set_cmd_level(TAP7_CTRL_LVL_CMD);
    CJTAG_write_TAP7_cmd_param(CMD_STORE_STFMT, 9);
    CJTAG_TAP7_CYCLE(0);
    CJTAG_TAP7_CYCLE(0);
    CJTAG_TAP7_CYCLE(0);
    CJTAG_TAP7_CYCLE(0);
    CJTAG_TAP7_CYCLE(0);

    CJTAG_TAP_CYCLE(1, 1, tdo_bit); // Select-DR-Scan
    CJTAG_TAP_CYCLE(1, 1, tdo_bit); // Select-IR-Scan
    CJTAG_TAP_CYCLE(0, 1, tdo_bit); // Capture-IR
    CJTAG_TAP_CYCLE(1, 1, tdo_bit); // Exit1-IR
    CJTAG_TAP_CYCLE(1, 1, tdo_bit); // Update-IR
    CJTAG_TAP_CYCLE(0, 1, tdo_bit); // Idle
}
#endif

static void CJTAG_hpm_jlink_standard_connect_seq(void)
{
     /* scape sequence "Reset": >= 8 TMS line state changes while TCK == HIGH */
    /* cJTAG TAP7 is in JScan0 mode now (TCK + TMS are transmitted) */
    CJTAG_write_escape_seq(10);
    /* >= 22 dummy clocks with TMS == HIGH. TAP: ??? => Reset */
    CJTAG_write_tmsc(0xFFFFFFFF, 22); /* 25 bits of TMS=1 */
    /* 1 clocks with TMS == LOW. TAP: Reset => Run-Test/Idle */
    CJTAG_write_tmsc(0x0, 2); /* 1 bits of TMS=0 */
    /* Escape sequence "Selection" */
    CJTAG_write_escape_seq(7);
    /* Forms of Selection Sequence page277 in ieee11497-2022 */
    /* There are two forms of the TAP.7 Selection Sequence, short and long, as shown in Figure 11-8 */
    /* short: online activation code(OAC)-4bit + extension code(EC)-4bit + check packet-4bit
     * long: OAC-4bit + EC-4bit + 24bit global register init values + check packet-4bit
     * The short form is used to select a specific technology and activate the TAP.7 controller.
     * The long form is used to select a specific technology, activate the TAP.7 controller, and initialize the global registers.
     * The long form is required when switching from one technology to another, as the global registers may need to be reinitialized.
     */

    /* 4-bit OAC sequence: Wake-up TAP7 of all technologies */
    CJTAG_write_tmsc(0x00, 4);
    /* -bit EC sequence: Use long-form selection sequence that also allows to specify the active format in detail */
    CJTAG_write_tmsc(0x00, 4);
    /* Write all 0s for 24-bit global register init values: SCNFMT, DLYC, RDYC, TPST, TPPREV, TP_DELN */
    CJTAG_write_tmsc(0x00000000, 24);
    /* Check packet */
    CJTAG_write_tmsc(0x00, 4);
    /* Set command level to 2 and lock it */
    CJTAG_set_cmd_level(TAP7_CTRL_LVL_CMD);
    /* TC1.SREDGE = 1 */
    CJTAG_write_TAP7_cmd_param(CMD_STORE_STC1, 1);
    /* STFMT[4:0] == 9 => OScan1 */
    CJTAG_write_TAP7_cmd_param(CMD_STORE_STFMT, 9);
    /* Check packet */
    // CJTAG_write_OScan1(0x00, 0, 4);
    CJTAG_write_tmsc(0x00, 4);
    // CJTAG_TAP7_CYCLE(0);
    /* Scan1 protocol is active from now on */
    /* MSC = 0*/
    CJTAG_write_OScan1_cmd_param(CMD_SELECT_MSC, 0);
    /* Needed for SPA to CPA transition */
    CJTAG_write_OScan1(0, 0, 1);
    /* Check packet */
    // CJTAG_write_OScan1(0x00, 0, 4);
    CJTAG_write_tmsc(0x00, 4);
    /* Exit command level 2: Idle -> DR-Scan -> IR-Scan -> Capture-IR -> Exit1-IR -> Update-IR -> Idle */
    /* JLink will start communicating with the target core itself. OScan1 is used all the time from now on */
    CJTAG_write_OScan1(0x1B, 0, 6);
}
