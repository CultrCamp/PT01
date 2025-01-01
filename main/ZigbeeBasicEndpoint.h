//
// Created by HanWool Lee on 24. 12. 31.
//

#ifndef PIPE_TEMP_SENSOR_ZIGBEEBASICENDPOINT_H
#define PIPE_TEMP_SENSOR_ZIGBEEBASICENDPOINT_H

#include "ZigbeeEP.h"

class ZigbeeBasicEndpoint: public ZigbeeEP {
public:
    ZigbeeBasicEndpoint(uint8_t endpoint): ZigbeeEP(endpoint) {
        _device_id = ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID;
        esp_zb_basic_cluster_cfg_t basicClusterCfg = {
                .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,                          \
                .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE,                        \
        };
        esp_zb_attribute_list_t* basic_cluster = esp_zb_basic_cluster_create(&basicClusterCfg);
        esp_zb_cluster_list_add_basic_cluster(_cluster_list, basic_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

        _ep_config = {
                .endpoint = _endpoint,
                .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
                .app_device_id = ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID,
                .app_device_version = 0
        };
    }

    ~ZigbeeBasicEndpoint();
};


#endif //PIPE_TEMP_SENSOR_ZIGBEEBASICENDPOINT_H
