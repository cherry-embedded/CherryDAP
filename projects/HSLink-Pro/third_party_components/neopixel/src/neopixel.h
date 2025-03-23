#pragma once

#include <functional>
#include <cstring>
#include "cstdint"

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class NeoPixel {
public:
    enum class interface_type_t : uint8_t {
        GPIO_POLLING,
        SPI_POLLING,
        SPI_DMA
    };

    enum class color_type_t : uint8_t {
        COLOR_R,
        COLOR_G,
        COLOR_B
    };

    NeoPixel(interface_type_t interfaceType, uint32_t pixel_cnt, uint8_t *buffer = nullptr);

    virtual ~NeoPixel();

    virtual void SetInterfaceConfig(void *args) = 0;

    void SetPixel(uint8_t pixel_idx, uint32_t color);

    void SetPixel(uint8_t pixel_idx, uint8_t r, uint8_t g, uint8_t b);

    void SetPixel(uint8_t pixel_idx, const Color &color);

    void ModifyPixel(uint8_t pixel_idx, color_type_t color, uint8_t value);

    virtual void Flush() = 0;

protected:
    interface_type_t interfaceType;
    uint32_t pixel_cnt;
    uint8_t *buffer;
    bool buffer_is_from_malloc;
};

class NeoPixel_GPIO_Polling : public NeoPixel {
public:
    struct interface_config_t {
        std::function<void(void *)> init;
        std::function<void(uint8_t, void *)> set_level;
        std::function<void(void *)> lock;
        std::function<void(void *)> unlock;
        uint32_t high_nop_cnt{};
        uint32_t low_nop_cnt{};
        void *user_data{};
    };

    explicit NeoPixel_GPIO_Polling(uint32_t pixel_cnt, uint8_t *buffer = nullptr) :
            NeoPixel(interface_type_t::GPIO_POLLING, pixel_cnt, buffer) {}

    void SetInterfaceConfig(interface_config_t *config) {
        this->config = *config;
        this->config.init(config->user_data);
    }

    __attribute__((section(".fast")))
    void Flush() override;

private:
    void SetInterfaceConfig(void *args) override {
        SetInterfaceConfig(reinterpret_cast<interface_config_t *>(args));
    }

    interface_config_t config;
};

class NeoPixel_SPI_Polling : public NeoPixel {
public:
    struct interface_config_t {
        std::function<void(void *)> init;
        std::function<void(uint8_t *, uint32_t, void *)> trans;
        void *user_data;
    };

    explicit NeoPixel_SPI_Polling(uint32_t pixel_cnt, uint8_t *buffer = nullptr, uint8_t *spi_buffer = nullptr);

    ~NeoPixel_SPI_Polling() override ;

    void SetInterfaceConfig(interface_config_t *config) {
        this->config = *config;
        this->config.init(config->user_data);
    }

    void Flush() override;

private:
    void SetInterfaceConfig(void *args) override {
        SetInterfaceConfig(reinterpret_cast<interface_config_t *>(args));
    }

    interface_config_t config;

    uint8_t *spi_buffer;
    bool spi_buffer_is_from_malloc;

    static constexpr uint8_t _bit0_pulse_width = 0xe0;  /* 0b11100000 40% high */
    static constexpr uint8_t _bit1_pulse_width = 0xF8;  /* 0b11111000 60% high */
};

