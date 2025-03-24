//
// Created by HalfSweet on 25-3-23.
//

#ifndef HSLINK_PRO_LED_H
#define HSLINK_PRO_LED_H

#include "HSLink_Pro_expansion.h"
#include "board.h"
#include "neopixel.h"
#include <cmath>
#include <cstdio>

constexpr Color COLOR_BLUE = {0, 0, 255};
constexpr Color COLOR_WHITE = {255, 255, 255};
constexpr Color COLOR_PINK = {255, 105, 180};
constexpr Color COLOR_GREEN = {0, 255, 0};

enum class LEDMode {
    IDLE,
    CONNECTED,
    RUNNING,
};

class LED {
private:
    NeoPixel *neopixel;
    LEDMode mode = LEDMode::IDLE;
    uint8_t brightness = 0;
    bool boost = false;
    bool enable = false;

    uint8_t lerp(uint8_t a, uint8_t b, float t)
    {
        return (uint8_t) (a + (b - a) * t);
    }

    Color lerpColor(const Color &a, const Color &b, float t)
    {
        Color result{};
        result.r = lerp(a.r, b.r, t);
        result.g = lerp(a.g, b.g, t);
        result.b = lerp(a.b, b.b, t);
        return result;
    }

    Color scaleColor(const Color &color, uint8_t factor)
    {
        Color result{};
        result.r = (uint8_t) ((uint32_t) color.r * factor / 100);
        result.g = (uint8_t) ((uint32_t) color.g * factor / 100);
        result.b = (uint8_t) ((uint32_t) color.b * factor / 100);
        return result;
    }

    void updateIdleEffect()
    {
        if (!neopixel) {
            return;
        }
        const uint32_t totalCycle = 3000; // 总周期 6000ms（6秒）
        auto now = millis();
        uint32_t elapsed = now % totalCycle;
        uint32_t segmentTime = totalCycle / 3;  // 每个阶段约 2000ms

        auto segment = elapsed / segmentTime;
        float t = ((float) elapsed - segment * segmentTime) / segmentTime;
        size_t next = (segment + 1) % 3;

        // 用 const 数组定义 Idle 状态下的颜色序列
        const Color idleColors[3] = {
                COLOR_BLUE,
                COLOR_PINK,
                COLOR_WHITE
        };

        auto interpolated = lerpColor(idleColors[segment], idleColors[next], t);
        auto finalColor = scaleColor(interpolated, this->brightness);

        this->neopixel->SetPixel(0, finalColor);
        this->neopixel->Flush();
    }

    void updateConnectedEffect()
    {
        if (!neopixel) {
            return;
        }
        auto finalColor = scaleColor(COLOR_GREEN, this->brightness);
        this->neopixel->SetPixel(0, finalColor);
        this->neopixel->Flush();
    }

    void updateRunningEffect()
    {
        if (!neopixel) {
            return;
        }
        const uint64_t breathingPeriod = 2000; // 呼吸周期 2000ms
        uint64_t now = millis();
        uint64_t elapsed = now % breathingPeriod;
        float t = (float) elapsed / (float) breathingPeriod;

        // 利用正弦函数计算呼吸波动因子，范围 0~1
        float breathFactor = (sinf(2 * M_PI * t - M_PI / 2) + 1.0f) / 2.0f;

        Color baseColor{};
        if (this->boost) {
            baseColor = COLOR_PINK;
        } else {
            baseColor = COLOR_BLUE;
        }

        auto finalColor = scaleColor(baseColor, breathFactor * this->brightness);

        this->neopixel->SetPixel(0, finalColor);
        this->neopixel->Flush();
    }

    void TurnOff()
    {
        if (!neopixel) {
            return;
        }
        this->neopixel->SetPixel(0, 0, 0, 0);
        this->neopixel->Flush();
    }

public:
    LED() = default;

    void SetNeoPixel(NeoPixel *neopixel)
    {
        this->neopixel = neopixel;
    }

    void SetMode(LEDMode mode)
    {
        this->mode = mode;
    }

    void SetBrightness(uint8_t brightness)
    {
        this->brightness = brightness;
    }

    void SetBoost(bool boost)
    {
        this->boost = boost;
    }

    void SetEnable(bool enable)
    {
        this->enable = enable;
        if (!enable)
        {
            TurnOff();
        }
    }

    void Handle()
    {
        // 10ms 更新一次
        static uint64_t lastUpdateTime = 0;
        uint64_t now = millis();
        if (now - lastUpdateTime < 10) {
            return;
        }
        lastUpdateTime = now;
        if (!enable) {
            return;
        }
        switch (this->mode) {
            case LEDMode::IDLE:
                updateIdleEffect();
                break;
            case LEDMode::CONNECTED:
                updateConnectedEffect();
                break;
            case LEDMode::RUNNING:
                updateRunningEffect();
                break;
        }
    }
};

#endif //HSLINK_PRO_LED_H
