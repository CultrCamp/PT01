/* Common Class for Zigbee End point */

#pragma once

#include "ZigbeeCore.h"
#if SOC_IEEE802154_SUPPORTED && CONFIG_ZB_ENABLED

#include <Arduino.h>
#include "ZigbeeEP.h"

/* Zigbee End Device Class */
class ZigbeeEP_M: public ZigbeeEP{
public:
    ZigbeeEP_M(uint8_t endpoint = 10): ZigbeeEP(endpoint) {

    }
    ~ZigbeeEP_M();

    void setPowerSource(zb_power_source_t power_source, uint8_t percentage = 255, uint8_t battery_centivolt = 33);
    void setBatteryVoltage(uint16_t milliVolt);
    void reportBatteryVoltage();
    void reportBatteryStatus();
    void setReport(uint16_t cluster_id, uint16_t attr_id);
};

#endif  //SOC_IEEE802154_SUPPORTED && CONFIG_ZB_ENABLED
