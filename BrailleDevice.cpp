#include "BrailleDevice.h"
#include "BrailleDescriptors.h"
#include "BleCompositeHID.h"
#include <Arduino.h>
#include <iostream>
#include <string>

#if defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define LOG_TAG "BrailleDevice"
#else
#include "esp_log.h"
static const char *LOG_TAG = "BrailleDevice";
#endif

BrailleCallbacks::BrailleCallbacks(BrailleDevice* device) :
    _device(device)
{
}

void BrailleCallbacks::onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo)
{
    size_t len = pCharacteristic->getLength(); // Get actual bytes received
    ESP_LOGI(LOG_TAG, "Received %d bytes via onWrite", len);

    if (len > 0) {
        // Get pointer to the raw byte data (uint8_t*)
        std::string value = pCharacteristic->getValue();
        const uint8_t* data = (uint8_t *) value.data(); 

        if (len == 41) { // Check if we got the expected 41 bytes > 40 cells plus 1 byte for the report id

            ESP_LOGI(LOG_TAG, "Processing 41 Braille cell bytes...");
            // --- Your Logic Here ---
            // Access the 40 bytes via the 'data' pointer
            // Example: Print bytes as hex
            for (size_t i = 0; i < len && i < len; ++i) {
                Serial.printf("%02X", data[i]); 
            }
            Serial.println();
            // --- End Your Logic ---
        } else {
            ESP_LOGW(LOG_TAG, "Received unexpected data length: %d bytes (expected 40)", len);
        }
    } else {
        ESP_LOGW(LOG_TAG, "Received empty data in onWrite");
    }
}

void BrailleCallbacks::onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo)
{
    ESP_LOGD(LOG_TAG, "BrailleDevice::onRead");
}

void BrailleCallbacks::onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue)
{
    ESP_LOGD(LOG_TAG, "BrailleDevice::onSubscribe");
}

void BrailleCallbacks::onStatus(NimBLECharacteristic* pCharacteristic, int code)
{
    ESP_LOGD(LOG_TAG, "BrailleDevice::onStatus");
}

BrailleDevice::BrailleDevice() :
    _config(BrailleConfiguration(BRAILLE_REPORT_ID)),
    _input(),
    _output(),
    _inputReport(),
    _callbacks(nullptr)
{
    
}

BrailleDevice::BrailleDevice(const BrailleConfiguration& config) :
    _config(config),
    _input(),
    _output(),
    _inputReport(),
    _callbacks(nullptr)
{
    
}

BrailleDevice::~BrailleDevice()
{
    if (getOutput() && _callbacks){
        getOutput()->setCallbacks(nullptr);
        delete _callbacks;
        _callbacks = nullptr;
    }
}

void BrailleDevice::init(NimBLEHIDDevice* hid)
{
    _input = hid->getInputReport(_config.getReportId());
    _output = hid->getOutputReport(_config.getReportId());
    _callbacks = new BrailleCallbacks(this);
    _output->setCallbacks(_callbacks);

    setCharacteristics(_input, _output);
}

const BaseCompositeDeviceConfiguration* BrailleDevice::getDeviceConfig() const
{
    return &_config;
}