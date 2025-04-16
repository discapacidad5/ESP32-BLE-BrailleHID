#ifndef ESP32_BRAILLE_DEVICE_H
#define ESP32_BRAILLE_DEVICE_H

#include "NimBLECharacteristic.h"
#include <BrailleConfiguration.h>
#include <BaseCompositeDevice.h>
#include <Callback.h>
#include <mutex>

// Forwards 
class BrailleDevice;

struct BrailleInputReport {
    uint8_t modifiers = 0x00;
    uint8_t reserved = 0x00;
    uint8_t keys[6]; // 8 bits per key * 101 keys = 6 bytes - copied from KeyboardDevice.h
};

class BrailleCallbacks : public NimBLECharacteristicCallbacks {
public:
    BrailleCallbacks(BrailleDevice* device);

    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override;
    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override;
    void onStatus(NimBLECharacteristic* pCharacteristic, int code) override;
    void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override;

private:
BrailleDevice* _device;
};


class BrailleDevice : public BaseCompositeDevice {
private:
    BrailleConfiguration _config;
    NimBLECharacteristic* _input;
    NimBLECharacteristic* _output;

    BrailleInputReport _inputReport;
    BrailleCallbacks* _callbacks;

public:
    BrailleDevice();
    BrailleDevice(const BrailleConfiguration& config);
    ~BrailleDevice();
    
    void init(NimBLEHIDDevice* hid) override;
    const BaseCompositeDeviceConfiguration* getDeviceConfig() const override;

private:

    // Threading
    std::mutex _mutex;
};

#endif