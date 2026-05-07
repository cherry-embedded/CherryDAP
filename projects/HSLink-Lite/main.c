#include "dap_main.h"
#include "hpm_dma_mgr.h"
#include "hpm_gpio_drv.h"
#include "hpm_gpiom_drv.h"
#include "hpm_romapi.h"
#include "hpm_gptmr_drv.h"
#include "board.h"
#include "usb2uart.h"

static void serial_number_init(void)
{
#define OTP_CHIP_UUID_IDX_START (88U)
#define OTP_CHIP_UUID_IDX_END   (91U)
    uint32_t uuid_words[4];

    uint32_t word_idx = 0;
    for (uint32_t i = OTP_CHIP_UUID_IDX_START; i <= OTP_CHIP_UUID_IDX_END; i++) {
        uuid_words[word_idx++] = ROM_API_TABLE_ROOT->otp_driver_if->read_from_shadow(i);
    }

    sprintf(serial_number_dynamic, "%08X%08X%08X%08X", uuid_words[0], uuid_words[1], uuid_words[2], uuid_words[3]);
    printf("Serial number: %s\n", serial_number_dynamic);
}

static inline void SWDIO_DIR_Init(void)
{
    HPM_IOC->PAD[SWDIO_DIR].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);

    gpiom_configure_pin_control_setting(SWDIO_DIR);
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR));
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1);
}

static inline void Port_Enable_Init(void) {
    // PA04
    HPM_IOC->PAD[IOC_PAD_PA04].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);

    gpiom_configure_pin_control_setting(IOC_PAD_PA04);
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(IOC_PAD_PA04), GPIO_GET_PIN_INDEX(IOC_PAD_PA04));
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(IOC_PAD_PA04), GPIO_GET_PIN_INDEX(IOC_PAD_PA04), 1);
}

GPTMR_Type *const USER_PWM = HPM_GPTMR0;
const clock_name_t USER_PWM_CLK = clock_gptmr0;
const uint8_t PWM_CHANNEL = 2;

ADC16_Type *const USER_ADC = HPM_ADC0;

const uint32_t DEFAULT_PWM_FREQ = 100000;
const uint8_t DEFAULT_PWM_DUTY = 85;
static uint32_t pwm_current_reload;

static void set_pwm_waveform_edge_aligned_frequency(uint32_t freq) {
    gptmr_channel_config_t config;
    uint32_t gptmr_freq;

    gptmr_channel_get_default_config(USER_PWM, &config);
    gptmr_freq = clock_get_frequency(USER_PWM_CLK);
    pwm_current_reload = gptmr_freq / freq;
    config.reload = pwm_current_reload;
    config.cmp_initial_polarity_high = true;
    gptmr_stop_counter(USER_PWM, PWM_CHANNEL);
    gptmr_channel_config(USER_PWM, PWM_CHANNEL, &config, false);
    gptmr_channel_reset_count(USER_PWM, PWM_CHANNEL);
    gptmr_start_counter(USER_PWM, PWM_CHANNEL);
}

static void set_pwm_waveform_edge_aligned_duty(uint8_t duty) {
    uint32_t cmp;
    if (duty > 100) {
        duty = 100;
    }
    cmp = ((pwm_current_reload * duty) / 100) + 1;
    gptmr_update_cmp(USER_PWM, PWM_CHANNEL, 0, cmp);
}

static void Power_PWM_Init(void) {
    // PA10 100k PWM，占空比50%
    HPM_IOC->PAD[IOC_PAD_PA10].FUNC_CTL = IOC_PA10_FUNC_CTL_GPTMR0_COMP_2;
    set_pwm_waveform_edge_aligned_frequency(DEFAULT_PWM_FREQ);
    set_pwm_waveform_edge_aligned_duty(DEFAULT_PWM_DUTY); // 以目前的控制形式来看，电压越高，输出电压就越小，先给个较高的占空比
}

static void Power_Set_TVCC_Voltage(double voltage) {
    // voltage = -DAC + (1974/395)
    // DAC = -voltage + (1974/395)

    // PWM DAC需要输出的电压
    double dac;

    dac = -voltage + (1974.0 / 395.0);

    if (dac < 0) {
        dac = 0;
    } else if (dac > 3.3) {
        dac = 3.3;
    }

    // 计算PWM占空比
    uint8_t duty = (uint8_t) (dac / 3.3 * 100);
    set_pwm_waveform_edge_aligned_duty(duty);
}

HSLink_Setting_t HSLink_Setting;
HSLink_Lazy_t HSLink_Global;

int main(void)
{
    board_init();
    board_init_usb(HPM_USB0);
    intc_set_irq_priority(IRQn_USB0, 3);

    HSLink_Global.reset_level = 0;
    HSLink_Setting.reset = RESET_NRST;
    HSLink_Setting.jtag_20pin_compatible = false;
    HSLink_Setting.boost = false;
    HSLink_Setting.swd_port_mode = PORT_MODE_GPIO;
    HSLink_Setting.jtag_port_mode = PORT_MODE_GPIO;

    serial_number_init();
    dma_mgr_init();

    SWDIO_DIR_Init();

    Port_Enable_Init();
    Power_PWM_Init();

    uartx_preinit();
    chry_dap_init(0, HPM_USB0_BASE);

    while (1) {
        chry_dap_handle();
        chry_dap_usb2uart_handle();
        usb2uart_handler();
        Power_Set_TVCC_Voltage(3.3);
    }

    return 0;
}