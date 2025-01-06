/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "Arduino.h"
#include "Zigbee.h"
#include "zcl/esp_zigbee_zcl_power_config.h"

#include "sdkconfig.h"
#include "Thermister.h"
#include "Battery.h"
#include "ZigbeeTemperatureSensorEndpoint.h"

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  55         /* Sleep for 55s will + 5s delay for establishing connection => data reported every 1 minute */

Thermister thermister1 = Thermister(CONFIG_THERMISTER1_GPIO, 10*1000);
Thermister thermister2 = Thermister(CONFIG_THERMISTER2_GPIO, 10*1000);
Battery battery = Battery(0);
ZigbeeTemperatureSensorEndpoint temperatureSensorEndpoint1 = ZigbeeTemperatureSensorEndpoint(1);
ZigbeeTemperatureSensorEndpoint temperatureSensorEndpoint2 = ZigbeeTemperatureSensorEndpoint(2);

void measureAndSleep() {
    battery.measure();
    ESP_LOGI("main", "Batt: %ldmV %dcV (%d%%)", battery.mv(), battery.cv(), battery.percent());
    temperatureSensorEndpoint1.setBatteryPercentage(battery.percent());
    temperatureSensorEndpoint1.setBatteryVoltage(battery.cv());
    temperatureSensorEndpoint1.reportBatteryStatus();
    delay(100);
    // get temperature and report
    float t1 = thermister1.temperature();
    temperatureSensorEndpoint1.setTemperature(t1);
    temperatureSensorEndpoint1.report();
    delay(100);
    // get temperature and report
    float t2 = thermister2.temperature();
    temperatureSensorEndpoint2.setTemperature(t2);
    temperatureSensorEndpoint2.report();
    delay(100);

    // Put device to deep sleep
    ESP_LOGI("main", "Going to sleep now");
    digitalWrite(CONFIG_LED_GPIO, LOW);
    esp_deep_sleep_start();
}

void setup() {
    pinMode(CONFIG_LED_GPIO, OUTPUT);
    digitalWrite(CONFIG_LED_GPIO, HIGH);
    pinMode(CONFIG_BOOT_GPIO, INPUT_PULLUP);

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

    // Init Zigbee
    battery.measure();
    temperatureSensorEndpoint1.setManufacturerAndModel("Cultr.Camp", "PT01");
    ESP_LOGI("main", "Batt: %ldmV %dcV (%d%%)", battery.mv(), battery.cv(), battery.percent());
    temperatureSensorEndpoint1.setPowerSource(ZB_POWER_SOURCE_BATTERY, battery.percent(), battery.cv());
    temperatureSensorEndpoint1.setMinMaxValue(-25, 125);
    temperatureSensorEndpoint1.setTolerance(1);
    Zigbee.addEndpoint(&temperatureSensorEndpoint1);

    temperatureSensorEndpoint2.setMinMaxValue(-25, 125);
    temperatureSensorEndpoint2.setTolerance(1);
    Zigbee.addEndpoint(&temperatureSensorEndpoint2);


    // Create a custom Zigbee configuration for End Device with keep alive 10s to avoid interference with reporting data
    esp_zb_cfg_t zigbeeConfig = ZIGBEE_DEFAULT_ED_CONFIG();
    zigbeeConfig.nwk_cfg.zed_cfg.keep_alive = 10000;

    if(!Zigbee.begin(&zigbeeConfig, false)) {
        ESP_LOGI("ZIGBEE", "Zigbee failed to start!");
        ESP_LOGI("ZIGBEE", "Rebooting...");
        ESP.restart();
    }
    ESP_LOGI("ZIGBEE", "Connecting to network");
    bool led = false;
    while (!Zigbee.connected()) {
        ESP_LOGI("ZIGBEE", "connecting");
        digitalWrite(CONFIG_LED_GPIO, led);
        led = !led;
        delay(500);
    }
    ESP_LOGI("ZIGBEE", "Successfully connected to Zigbee network. wait 10 sec for complete identify");
    delay(1000);

    temperatureSensorEndpoint1.setReport(ESP_ZB_ZCL_CLUSTER_ID_POWER_CONFIG, ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID);
    temperatureSensorEndpoint1.setTemperature(thermister1.temperature());
    temperatureSensorEndpoint2.setTemperature(thermister2.temperature());
    // Delay approx 1s (may be adjusted) to allow establishing proper connection with coordinator, needed for sleepy devices
    delay(5000);
}

void loop() {
    // Checking button for factory reset
    if (digitalRead(CONFIG_BOOT_GPIO) == LOW) {  // Push button pressed
        // Key debounce handling
        delay(100);
        int startTime = millis();
        while (digitalRead(CONFIG_BOOT_GPIO) == LOW) {
            delay(50);
            if ((millis() - startTime) > 3000) {
                // If key pressed for more than 3secs, factory reset Zigbee and reboot
                ESP_LOGI("LOOP", "Resetting Zigbee to factory and rebooting in 1s.");
                delay(1000);
                Zigbee.factoryReset();
            }
        }
    }

    // Call the function to measure temperature and put the device to sleep
    measureAndSleep();
    delay(1000);
}