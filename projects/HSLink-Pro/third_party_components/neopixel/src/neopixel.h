#pragma once

#include <functional>
#include <cstring>
#include "cstdint"

class NeoPixel {
public:
    enum class interface_type_t : uint8_t {
        GPIO_POLLING,
        SPI_POLLING,
        SPI_DMA
    };

    NeoPixel(interface_type_t interfaceType, uint32_t pixel_cnt, uint8_t *buffer = nullptr);

    virtual ~NeoPixel();

    virtual void SetInterfaceConfig(void *args) = 0;

    void SetPixel(uint8_t pixel_idx, uint32_t color);

    void SetPixel(uint8_t pixel_idx, uint8_t r, uint8_t g, uint8_t b);

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
        uint32_t high_nop_cnt;
        uint32_t low_nop_cnt;
        void *user_data;
    };

    NeoPixel_GPIO_Polling(uint32_t pixel_cnt, uint8_t *buffer = nullptr) :
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

