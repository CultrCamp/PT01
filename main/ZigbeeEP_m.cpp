//
// Created by HanWool Lee on 25. 1. 2.
//

/* Common Class for Zigbee End Point */

#include "ZigbeeEP_m.h"

#if SOC_IEEE802154_SUPPORTED && CONFIG_ZB_ENABLED

#include "esp_zigbee_cluster.h"
#include "zcl/esp_zigbee_zcl_power_config.h"


void ZigbeeEP_M::setPowerSource(zb_power_source_t power_source, uint8_t battery_percentage, uint8_t battery_centivolt) {
    esp_zb_attribute_list_t *basic_cluster = esp_zb_cluster_list_get_cluster(_cluster_list, ESP_ZB_ZCL_CLUSTER_ID_BASIC, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_update_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_POWER_SOURCE_ID, (void *)&power_source);

    if (power_source == ZB_POWER_SOURCE_BATTERY) {
        // Add power config cluster and battery percentage attribute
        battery_percentage = battery_percentage * 2;
        esp_zb_attribute_list_t *power_config_cluster = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_POWER_CONFIG);
        esp_zb_power_config_cluster_add_attr(power_config_cluster, ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING_ID, (void *)&battery_percentage);
        esp_zb_power_config_cluster_add_attr(power_config_cluster, ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID, (void*)&battery_centivolt);
        esp_zb_cluster_list_add_power_config_cluster(_cluster_list, power_config_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    }
    _power_source = power_source;
}

void ZigbeeEP_M::setBatteryVoltage(uint16_t milliVolt) {
    esp_zb_lock_acquire(portMAX_DELAY);
    esp_zb_zcl_set_attribute_val(
            _endpoint, ESP_ZB_ZCL_CLUSTER_ID_POWER_CONFIG, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID, &milliVolt,
            false
    );
    esp_zb_lock_release();
    log_v("Battery percentage updated");
}

void ZigbeeEP_M::reportBatteryVoltage() {
    esp_zb_zcl_report_attr_cmd_t report_attr_cmd;
    report_attr_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
    report_attr_cmd.attributeID = ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID;
    report_attr_cmd.direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI;
    report_attr_cmd.clusterID = ESP_ZB_ZCL_CLUSTER_ID_POWER_CONFIG;
    report_attr_cmd.zcl_basic_cmd.src_endpoint = _endpoint;

    esp_zb_lock_acquire(portMAX_DELAY);
    esp_zb_zcl_report_attr_cmd_req(&report_attr_cmd);
    esp_zb_lock_release();
    log_v("Battery voltage reported");
}

void ZigbeeEP_M::reportBatteryStatus() {
    /* Send report attributes command */
    reportBatteryPercentage();
    reportBatteryVoltage();
    log_v("Battery status reported");
}


void ZigbeeEP_M::setReport(uint16_t cluster_id, uint16_t attr_id) {
    ESP_LOGI("ZIGBEE", "setReport(0x%04X, 0x%04X)", cluster_id, attr_id);
    esp_zb_zcl_reporting_info_t reporting_info = {
            .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_SRV,
            .ep = _endpoint,
            .cluster_id = cluster_id,
            .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
            .attr_id = attr_id,
            .u =
                    {
                            .send_info =
                                    {
                                            .min_interval = 0,
                                            .max_interval = 1,
                                            .delta =
                                                    {
                                                            .u16 = (uint16_t)(0 * 100),  // Convert delta to ZCL uint16_t
                                                    },
                                            .def_min_interval = 0,
                                            .def_max_interval = 1,
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
    ESP_LOGI("ZIGBEE", "setReport(0x%04X, 0x%04X) DONE", cluster_id, attr_id);
}

#endif  //SOC_IEEE802154_SUPPORTED && CONFIG_ZB_ENABLED
