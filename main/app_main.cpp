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
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &my_handle));
    battery.measure();
    ESP_LOGI("main", "Batt: %ldmV(%d%%)", battery.mv(), battery.percent());
    temperatureSensorEndpoint1.setBatteryPercentage(battery.percent());
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

    esp_err_t err = nvs_set_i16(my_handle, "t1", (int16_t)(t1 * 100));
    printf((err != ESP_OK) ? "T1 Failed!\n" : "T1 Done\n");
    err = nvs_set_i16(my_handle, "t2", (int16_t)(t2 * 100));
    printf((err != ESP_OK) ? "T2 Failed!\n" : "T2 Done\n");
    ESP_LOGI("main", "T1: %.2f T2: %.2f", t1, t2);

     err = nvs_commit(my_handle);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    nvs_close(my_handle);
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
    temperatureSensorEndpoint1.setManufacturerAndModel("Cultr.Camp", "PT01");
    temperatureSensorEndpoint1.setPowerSource(ZB_POWER_SOURCE_BATTERY, 100);
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

    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &my_handle));

    int16_t t1 = 0;
    err = nvs_get_i16(my_handle, "t1", &t1);
    switch (err) {
        case ESP_OK:
            printf("Done\n");
            printf("T1 = %d\n", t1);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            printf("The value is not initialized yet!\n");
            t1 = 0;
            break;
        default :
            printf("Error (%s) reading!\n", esp_err_to_name(err));
    }
    temperatureSensorEndpoint1.setTemperature((float)t1 / 100);


    int16_t t2 = 0;
    err = nvs_get_i16(my_handle, "t2", &t2);
    switch (err) {
        case ESP_OK:
            printf("Done\n");
            printf("T2 = %d\n", t2);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            printf("The value is not initialized yet!\n");
            t2 = 0;
            break;
        default :
            printf("Error (%s) reading!\n", esp_err_to_name(err));
    }
    temperatureSensorEndpoint2.setTemperature((float)t2 / 100);
    nvs_close(my_handle);
    // Delay approx 1s (may be adjusted) to allow establishing proper connection with coordinator, needed for sleepy devices
    delay(10000);
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