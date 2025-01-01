//
// Created by HanWool Lee on 25. 1. 1.
//

#ifndef PIPE_TEMP_SENSOR_BATTERY_H
#define PIPE_TEMP_SENSOR_BATTERY_H

#include "esp_log.h"
#include "stdint.h"
#include "Arduino.h"
#include "single_moving_average.h"

#define ASSUME_ZERO_PERCENT_MV 3500
#define ASSUME_100_PERCENT_MV 4200
#define VOLTAGE_DIVIDER_MULTIPLY 2

class Battery{
public:
    Battery(uint8_t pin, uint8_t bufferSize = 16):
            _pin(pin)
    {
        _pin = pin;
        _buffer = new MovingAverage<uint32_t>(bufferSize);
        analogReadResolution(12);
    }

    ~Battery();

    void measure() {
        for(uint8_t i = 0; i < 10; i++) {
            _voltage = _buffer->next((analogReadMilliVolts(_pin) * VOLTAGE_DIVIDER_MULTIPLY));
            delay(10);
        }
    }

    uint32_t mv() {
        return _voltage;
    }

    uint8_t percent() {
        ESP_LOGI("Bettery", "(%ld - %d) / (%d - %d) * 100) = %d", _voltage, ASSUME_ZERO_PERCENT_MV,
                 ASSUME_100_PERCENT_MV, ASSUME_ZERO_PERCENT_MV,
                 (uint8_t)((_voltage - ASSUME_ZERO_PERCENT_MV) / (ASSUME_100_PERCENT_MV - ASSUME_ZERO_PERCENT_MV) * 100)
        );
        return (uint8_t)((_voltage - ASSUME_ZERO_PERCENT_MV) / (ASSUME_100_PERCENT_MV - ASSUME_ZERO_PERCENT_MV) * 100);
    }
private:
    MovingAverage<uint32_t>* _buffer;
    uint8_t _pin;
    uint32_t _voltage = 0;
};
#endif //PIPE_TEMP_SENSOR_BATTERY_H
