//
// Created by HanWool Lee on 25. 1. 1.
//

#ifndef PIPE_TEMP_SENSOR_ZIGBEETEMPERATURESENSORENDPOINT_H
#define PIPE_TEMP_SENSOR_ZIGBEETEMPERATURESENSORENDPOINT_H

#include "ZigbeeEP_m.h"

class  ZigbeeTemperatureSensorEndpoint: public ZigbeeEP_M {
public:
    ZigbeeTemperatureSensorEndpoint(uint8_t endpoint): ZigbeeEP_M(endpoint) {
        _device_id = ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID;
        if(endpoint == 1) {
            esp_zb_temperature_sensor_cfg_t temp_sensor_cfg = ESP_ZB_DEFAULT_TEMPERATURE_SENSOR_CONFIG();
            _cluster_list = esp_zb_temperature_sensor_clusters_create(&temp_sensor_cfg);
        } else {
            esp_zb_temperature_meas_cluster_cfg_t temperatureMeasClusterCfg = {
                    .measured_value = ESP_ZB_ZCL_TEMP_MEASUREMENT_MEASURED_VALUE_DEFAULT,
                    .min_value = ESP_ZB_ZCL_TEMP_MEASUREMENT_MIN_MEASURED_VALUE_DEFAULT,
                    .max_value = ESP_ZB_ZCL_TEMP_MEASUREMENT_MAX_MEASURED_VALUE_DEFAULT
            };
            esp_zb_attribute_list_t *cluster = esp_zb_temperature_meas_cluster_create(&temperatureMeasClusterCfg);
            _cluster_list = esp_zb_zcl_cluster_list_create();
            esp_zb_cluster_list_add_temperature_meas_cluster(_cluster_list, cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
        }
        _ep_config = {
                .endpoint = _endpoint,
                .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
                .app_device_id = ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID,
                .app_device_version = 0
        };
    }

    void setMinMaxValue(float min, float max) {
        int16_t zb_min = zb_float_to_s16(min);
        int16_t zb_max = zb_float_to_s16(max);
        esp_zb_attribute_list_t *temp_measure_cluster =
                esp_zb_cluster_list_get_cluster(_cluster_list, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
        esp_zb_cluster_update_attr(temp_measure_cluster, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_MIN_VALUE_ID, (void *)&zb_min);
        esp_zb_cluster_update_attr(temp_measure_cluster, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_MAX_VALUE_ID, (void *)&zb_max);
    }
    void setTolerance(float tolerance) {
        // Convert tolerance to ZCL uint16_t
        uint16_t zb_tolerance = (uint16_t)(tolerance * 100);
        esp_zb_attribute_list_t *temp_measure_cluster =
                esp_zb_cluster_list_get_cluster(_cluster_list, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
        esp_zb_temperature_meas_cluster_add_attr(temp_measure_cluster, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_TOLERANCE_ID, (void *)&zb_tolerance);
    }
    void setReporting(uint16_t min_interval, uint16_t max_interval, float delta) {
        esp_zb_zcl_reporting_info_t reporting_info = {
                .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_SRV,
                .ep = _endpoint,
                .cluster_id = ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
                .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                .attr_id = ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
                .u =
                        {
                                .send_info =
                                        {
                                                .min_interval = min_interval,
                                                .max_interval = max_interval,
                                                .delta =
                                                        {
                                                                .u16 = (uint16_t)(delta * 100),  // Convert delta to ZCL uint16_t
                                                        },
                                                .def_min_interval = min_interval,
                                                .def_max_interval = max_interval,
                                        },
                        },
                .dst =
                        {
                                .profile_id = ESP_ZB_AF_HA_PROFILE_ID,
                        },
                .manuf_code = ESP_ZB_ZCL_ATTR_NON_MANUFACTURER_SPECIFIC,
        };
        esp_zb_lock_acquire(portMAX_DELAY);
        esp_zb_zcl_update_reporting_info(&reporting_info);
        esp_zb_lock_release();
    }

    void setTemperature(float temperature) {
        int16_t zb_temperature = zb_float_to_s16(temperature);
        log_v("Updating temperature sensor value...");
        /* Update temperature sensor measured value */
        log_d("Setting temperature to %d", zb_temperature);
        esp_zb_lock_acquire(portMAX_DELAY);
        esp_zb_zcl_set_attribute_val(
                _endpoint, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID, &zb_temperature, false
        );
        esp_zb_lock_release();
    }

    void report() {
        /* Send report attributes command */
        esp_zb_zcl_report_attr_cmd_t report_attr_cmd;
        report_attr_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
        report_attr_cmd.attributeID = ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID;
        report_attr_cmd.direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI;
        report_attr_cmd.clusterID = ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT;
        report_attr_cmd.zcl_basic_cmd.src_endpoint = _endpoint;

        esp_zb_lock_acquire(portMAX_DELAY);
        esp_zb_zcl_report_attr_cmd_req(&report_attr_cmd);
        esp_zb_lock_release();
        log_v("Temperature report sent");
    }
private:

    static int16_t zb_float_to_s16(float temp) {
        return (int16_t)(temp * 100);
    }
};
#endif //PIPE_TEMP_SENSOR_ZIGBEETEMPERATURESENSORENDPOINT_H
