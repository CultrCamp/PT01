//
// Created by HanWool Lee on 24. 12. 31.
//

#ifndef PIPE_TEMP_SENSOR_THERMISTER_H
#define PIPE_TEMP_SENSOR_THERMISTER_H

#include "math.h"
#include "Arduino.h"
#include "single_moving_average.h"

class Thermister{
public:
    Thermister(uint8_t pin, uint32_t seriesResister = 10000, uint8_t bufferSize = 16,
               uint16_t supplyVoltage = 3300, uint16_t beta = 3950, float t0 = 298.15, uint16_t r0 = 10000):
            _pin(pin),
            _seriesResister(seriesResister),
            _supplyVoltage(supplyVoltage),
            _beta(beta), _t0(t0), _r0(r0)
    {
        _pin = pin;
        _buffer = new MovingAverage<float>(bufferSize);
        analogReadResolution(12);
    }

    ~Thermister();

    float temperature() {
        float smaVoltage = 0.0;
        for(uint8_t i = 0; i < 10; i++) {
            smaVoltage = _buffer->next((float)analogReadMilliVolts(_pin));
            delay(10);
        }
        float voltage = smaVoltage / 1000;
        float R_NTC = _seriesResister * (3.3 - voltage) / voltage;
        float tempK = _beta / (log(R_NTC / _r0) + (_beta / _t0));
        return tempK - 273.15;
    }

private:
    MovingAverage<float>* _buffer;
    uint8_t _pin;
    uint64_t _lastReadMillis;
    uint32_t _seriesResister;
    uint32_t _supplyVoltage;
    uint16_t _beta;
    float _t0;
    uint16_t _r0;
};


#endif //PIPE_TEMP_SENSOR_THERMISTER_H
