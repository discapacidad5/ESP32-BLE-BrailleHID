#include <NimBLEDevice.h>
#include <NimBLEUtils.h>
#include <NimBLEServer.h>
#include "NimBLEHIDDevice.h"
#include "HIDTypes.h"
#include "HIDKeyboardTypes.h"
#include <driver/adc.h>
#include "sdkconfig.h"

#include "BleCompositeHID.h"
#include "BleConnectionStatus.h"

#include <sstream>
#include <iostream>
#include <iomanip>


#if defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define LOG_TAG "BLECompositeHID"
#else
#include "esp_log.h"
static const char *LOG_TAG = "BLECompositeHID";
#endif

#define SERVICE_UUID_DEVICE_INFORMATION        "180A"      // Service - Device information

#define CHARACTERISTIC_UUID_SYSTEM_ID          "2A23"      // Characteristic - System ID 0x2A23
#define CHARACTERISTIC_UUID_MODEL_NUMBER       "2A24"      // Characteristic - Model Number String - 0x2A24
#define CHARACTERISTIC_UUID_SOFTWARE_REVISION  "2A28"      // Characteristic - Software Revision String - 0x2A28
#define CHARACTERISTIC_UUID_SERIAL_NUMBER      "2A25"      // Characteristic - Serial Number String - 0x2A25
#define CHARACTERISTIC_UUID_FIRMWARE_REVISION  "2A26"      // Characteristic - Firmware Revision String - 0x2A26
#define CHARACTERISTIC_UUID_HARDWARE_REVISION  "2A27"      // Characteristic - Hardware Revision String - 0x2A27

uint16_t vidSource;
uint16_t vid;
uint16_t pid;
uint16_t guidVersion;
uint16_t hidType;
std::string modelNumber;
std::string softwareRevision;
std::string serialNumber;
std::string firmwareRevision;
std::string hardwareRevision;
std::string systemID;

std::string uint8_to_hex_string(const uint8_t *v, const size_t s) {
  std::stringstream ss;

  ss << std::hex << std::setfill('0');

  for (int i = 0; i < s; i++) {
    ss << std::hex << std::setw(2) << static_cast<int>(v[i]);
  }

  return ss.str();
}

BleCompositeHID::BleCompositeHID(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel) : _hid(nullptr), _autoSendTaskHandle(NULL) // Initialize task handle
{
    this->deviceName = deviceName.substr(0, CONFIG_BT_NIMBLE_GAP_DEVICE_NAME_MAX_LEN - 1);
    this->deviceManufacturer = deviceManufacturer;
    this->batteryLevel = batteryLevel;
    this->_connectionStatus = new BleConnectionStatus();
}

BleCompositeHID::~BleCompositeHID()
{
    // Consider calling end() here to clean up tasks if not done elsewhere
    delete this->_connectionStatus;
    // if (_hid) { delete _hid; _hid = nullptr; } // NimBLEHIDDevice might be managed by NimBLEServer? Check docs.
}

void BleCompositeHID::begin()
{
    this->begin(BLEHostConfiguration());
}

void BleCompositeHID::begin(const BLEHostConfiguration& config)
{
    _configuration = config; // we make a copy, so the user can't change actual values midway through operation, without calling the begin function again

    modelNumber = _configuration.getModelNumber();
    softwareRevision = _configuration.getSoftwareRevision();
    serialNumber = _configuration.getSerialNumber();
    firmwareRevision = _configuration.getFirmwareRevision();
    hardwareRevision = _configuration.getHardwareRevision();
    systemID = _configuration.getSystemID();

    vidSource = _configuration.getVidSource();
	vid = _configuration.getVid();
	pid = _configuration.getPid();
	guidVersion = _configuration.getGuidVersion();
    hidType = _configuration.getHidType();

#ifndef PNPVersionField
    // Legacy behaviour for versions of Nimble <= 1.4.1
	uint8_t high = highByte(vid);
	uint8_t low = lowByte(vid);

	vid = low << 8 | high;

	high = highByte(pid);
	low = lowByte(pid);

	pid = low << 8 | high;

	high = highByte(guidVersion);
	low = lowByte(guidVersion);
	guidVersion = low << 8 | high;
#endif

    // Start BLE server task
    // Increased stack size slightly, adjust if needed
    xTaskCreate(this->taskServer, "server", 5120, (void *)this, 5, NULL);
}

void BleCompositeHID::end(void)
{
    if(_autoSendTaskHandle != NULL) { // Check if task handle is valid before deleting
        vTaskDelete(this->_autoSendTaskHandle);
        _autoSendTaskHandle = NULL; // Set handle to NULL after deletion
    }
    // Optional: Add NimBLEDevice::deinit(true); // true to release memory
    ESP_LOGI(LOG_TAG, "BleCompositeHID ended.");
}


void BleCompositeHID::timedSendDeferredReports(void *pvParameter)
{
    BleCompositeHID *BleCompositeHIDInstance = (BleCompositeHID *)pvParameter;

    ESP_LOGI(LOG_TAG, "timedSendDeferredReports task started.");
    if (BleCompositeHIDInstance && BleCompositeHIDInstance->_hid) // Check instance validity
    {
        std::function<void()> reportFunc;
        while(true) { // Loop indefinitely until task is deleted
            if(BleCompositeHIDInstance->_deferredReports.ConsumeSync(reportFunc)) { // ConsumeSync waits
                if (BleCompositeHIDInstance->isConnected()) { // Only send if connected
                    reportFunc();
                    if(BleCompositeHIDInstance->_configuration.getQueueSendRate() > 0) {
                        vTaskDelay((1000 / BleCompositeHIDInstance->_configuration.getQueueSendRate()) / portTICK_PERIOD_MS);
                    }
                } else {
                     ESP_LOGD(LOG_TAG, "Deferred report skipped, not connected.");
                     // Optional: Add a small delay here if not connected to prevent busy-waiting on ConsumeSync if queue fills rapidly?
                     vTaskDelay(10 / portTICK_PERIOD_MS);
                }
            } else {
                // ConsumeSync returned false, likely because Finish() was called or queue is empty and processing should stop
                ESP_LOGI(LOG_TAG, "ConsumeSync returned false, exiting timedSendDeferredReports task.");
                break; // Exit loop
            }
        }
    } else {
         ESP_LOGE(LOG_TAG, "Invalid instance or HID device in timedSendDeferredReports.");
    }
    vTaskDelete(NULL); // Delete task when exiting loop
}

void BleCompositeHID::addDevice(BaseCompositeDevice *device)
{
    if(device) { // Basic null check
        device->_parent = this;
        _devices.push_back(device);
    } else {
        ESP_LOGW(LOG_TAG, "Attempted to add a NULL device.");
    }
}

bool BleCompositeHID::isConnected()
{
    // Delegate to connection status helper, ensure it's not null
    return (this->_connectionStatus != nullptr && this->_connectionStatus->isConnected());
}

void BleCompositeHID::setBatteryLevel(uint8_t level)
{
    this->batteryLevel = level;
    if (this->_hid)
    {
        // Notify if connected, otherwise just update internal value
        this->_hid->setBatteryLevel(this->batteryLevel, this->isConnected());
    }
}

void BleCompositeHID::queueDeviceDeferredReport(std::function<void()> && reportFunc)
{
    this->_deferredReports.Produce(std::move(reportFunc)); // Use std::move
}

void BleCompositeHID::sendDeferredReports()
{
    if (this->_hid && this->isConnected()) // Check HID and connection status
    {
        std::function<void()> reportFunc;
        while(this->_deferredReports.Consume(reportFunc)){ // Non-blocking consume
            reportFunc();
        }
    }
}

void BleCompositeHID::taskServer(void *pvParameter)
{
    BleCompositeHID *BleCompositeHIDInstance = (BleCompositeHID *)pvParameter;

    // ESP_LOGI(LOG_TAG, "Setting custom MAC address (example - currently commented out)");
    // Use the procedure below to set a custom Bluetooth MAC address
    // Compiler adds 0x02 to the last value of board's base MAC address to get the BT MAC address, so take 0x02 away from the value you actually want when setting
    // uint8_t newMACAddress[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF - 0x02};
    // esp_base_mac_addr_set(&newMACAddress[0]); // Set new MAC address

    ESP_LOGI(LOG_TAG, "Initializing NimBLE device: %s", BleCompositeHIDInstance->deviceName.c_str());
    NimBLEDevice::init(BleCompositeHIDInstance->deviceName); // Initialize NimBLE with the device name (for internal use by NimBLE)

    ESP_LOGI(LOG_TAG, "Creating NimBLE server...");
    NimBLEServer *pServer = NimBLEDevice::createServer();
    if (!pServer) {
        ESP_LOGE(LOG_TAG, "Failed to create NimBLE server!");
        vTaskDelete(NULL);
        return;
    }
    pServer->setCallbacks(BleCompositeHIDInstance->_connectionStatus);
    pServer->advertiseOnDisconnect(true); // start advertising again after disconnecting

    ESP_LOGI(LOG_TAG, "Creating NimBLE HID device...");
    BleCompositeHIDInstance->_hid = new NimBLEHIDDevice(pServer);
    if (!BleCompositeHIDInstance->_hid) {
        ESP_LOGE(LOG_TAG, "Failed to create NimBLEHIDDevice!");
        // Clean up server? NimBLEDevice::deinit?
        vTaskDelete(NULL);
        return;
    }

    // Setup the HID descriptor buffers
    size_t totalBufferSize = 2048; // Should be enough for most composite devices
    uint8_t tempHidReportDescriptor[totalBufferSize];
    int hidReportDescriptorSize = 0;
    ESP_LOGI(LOG_TAG, "Initializing child devices and building HID descriptor...");

    // Setup child devices to build the HID report descriptor
    for(auto device : BleCompositeHIDInstance->_devices){
        if (!device) {
            ESP_LOGW(LOG_TAG, "Skipping NULL device pointer in _devices vector.");
            continue;
        }
        const char* currentDeviceName = "Unknown"; // Default name
        auto config = device->getDeviceConfig();
        if (config) {
            currentDeviceName = config->getDeviceName();
        }

        ESP_LOGD(LOG_TAG, "Before device %s init", currentDeviceName);
        device->init(BleCompositeHIDInstance->_hid); // Pass HID device pointer to child
        ESP_LOGD(LOG_TAG, "After device %s init", currentDeviceName);

        if (!config) {
            ESP_LOGE(LOG_TAG, "Device %p has NULL configuration, skipping.", (void*)device);
            continue;
        }

        // Use a temporary buffer for each device's report descriptor part
        uint8_t deviceReportBuffer[BLE_ATT_ATTR_MAX_LEN]; // Max size for one part
        size_t reportSize = config->makeDeviceReport(deviceReportBuffer, sizeof(deviceReportBuffer));

        // Validate the returned size
        if(reportSize == 0 || reportSize == (size_t)-1){ // Check for 0 or error (-1 cast to size_t)
             ESP_LOGE(LOG_TAG, "Error creating or empty report descriptor for device %s (size: %zu)", currentDeviceName, reportSize); // Use %zu for size_t
             continue; // Skip this device if its descriptor is invalid
        } else if (reportSize > sizeof(deviceReportBuffer)) {
             // This case should technically not happen if makeDeviceReport respects bufferSize
             ESP_LOGE(LOG_TAG, "Device %s report size %zu exceeds temporary buffer %zu", currentDeviceName, reportSize, sizeof(deviceReportBuffer)); // Use %zu
             continue;
        } else {
             ESP_LOGD(LOG_TAG, "Created device %s descriptor part with size %zu", currentDeviceName, reportSize); // Use %zu
        }

        // Check if adding this report part exceeds the total composite descriptor buffer
        if (hidReportDescriptorSize + reportSize > totalBufferSize) {
            ESP_LOGE(LOG_TAG, "Total HID descriptor size exceeds buffer limit (%zu bytes) while adding device %s. Stopping descriptor build.", totalBufferSize, currentDeviceName); // Use %zu
            break; // Stop adding more devices
        }

        // Append the device's descriptor part to the combined descriptor
        memcpy(tempHidReportDescriptor + hidReportDescriptorSize, deviceReportBuffer, reportSize);
        hidReportDescriptorSize += reportSize;
    }
    ESP_LOGI(LOG_TAG, "Final Combined HID Report Descriptor Size: %d bytes", hidReportDescriptorSize); // %d is okay for int

    // Set the final combined report map (only if it's valid)
    if (hidReportDescriptorSize > 0) {
        // Create a correctly sized buffer for the final map
        uint8_t finalHidReportDescriptor[hidReportDescriptorSize];
        memcpy(finalHidReportDescriptor, tempHidReportDescriptor, hidReportDescriptorSize);
        // Log the final descriptor for debugging if needed (use VERBOSE level)
        // ESP_LOG_BUFFER_HEXDUMP(LOG_TAG, finalHidReportDescriptor, hidReportDescriptorSize, ESP_LOG_VERBOSE);
        BleCompositeHIDInstance->_hid->setReportMap(finalHidReportDescriptor, hidReportDescriptorSize);
        ESP_LOGI(LOG_TAG, "HID Report Map set successfully.");
    } else {
        ESP_LOGE(LOG_TAG, "No valid HID descriptors were added. Cannot set Report Map. Aborting server setup.");
        // Clean up resources before exiting task
        delete BleCompositeHIDInstance->_hid;
        BleCompositeHIDInstance->_hid = nullptr;
        // Optional: NimBLEDevice::deinit(true);
        vTaskDelete(NULL); // Exit task
        return;
    }

    // Set manufacturer info
    BleCompositeHIDInstance->_hid->setManufacturer(BleCompositeHIDInstance->deviceManufacturer);

    // Get or Create Device Information Service (0x180A)
    // NimBLEHIDDevice likely creates this automatically when setting PnP info,
    // but let's get a reference to add more characteristics if needed.
    NimBLEService *pService = pServer->getServiceByUUID(SERVICE_UUID_DEVICE_INFORMATION);
    if(!pService) {
         // If NimBLEHIDDevice didn't create it, we might need to create it manually.
         // However, setPnP usually handles this. Log a warning if it's missing.
         ESP_LOGW(LOG_TAG, "Device Information Service (0x180A) not found after HID setup. It might be created later by setPnP.");
         // If manual creation is needed: pService = pServer->createService(SERVICE_UUID_DEVICE_INFORMATION);
    }

    // Add Device Information characteristics (Model Number, Serial Number etc.)
    // Check if pService exists *after* potentially being created by setPnP below, or ensure setPnP is called first.
    // For now, assume setPnP handles service creation and retrieve it *afterwards* if needed.

    // Set PnP IDs (Vendor ID, Product ID, Version) - This often creates the 0x180A service if not present
    ESP_LOGI(LOG_TAG, "Setting PnP Info: VID=0x%04X, PID=0x%04X, Version=0x%04X, Source=0x%02X", vid, pid, guidVersion, vidSource);
    BleCompositeHIDInstance->_hid->setPnp(vidSource, vid, pid, guidVersion);

    // Set HID Information (bcdHID=1.11, country=0)
    BleCompositeHIDInstance->_hid->setHidInfo(0x00, 0x0111); // HID Version 1.11

    // Now retrieve the Device Info service again to add characteristics
    pService = pServer->getServiceByUUID(SERVICE_UUID_DEVICE_INFORMATION);
    if(pService) {
        ESP_LOGI(LOG_TAG, "Setting Device Information Characteristics...");
        // Model Number
        BLECharacteristic* pChr = pService->getCharacteristic(CHARACTERISTIC_UUID_MODEL_NUMBER);
        if(!pChr){ pChr = pService->createCharacteristic(CHARACTERISTIC_UUID_MODEL_NUMBER, NIMBLE_PROPERTY::READ);}
        if(pChr) pChr->setValue(modelNumber); else { ESP_LOGE(LOG_TAG, "Failed to create/get Model Number Characteristic"); }
        // Software Revision
        pChr = pService->getCharacteristic(CHARACTERISTIC_UUID_SOFTWARE_REVISION);
        if(!pChr){ pChr = pService->createCharacteristic(CHARACTERISTIC_UUID_SOFTWARE_REVISION, NIMBLE_PROPERTY::READ);}
        if(pChr) pChr->setValue(softwareRevision); else { ESP_LOGE(LOG_TAG, "Failed to create/get Software Revision Characteristic"); }
        // Serial Number
        pChr = pService->getCharacteristic(CHARACTERISTIC_UUID_SERIAL_NUMBER);
        if(!pChr){ pChr = pService->createCharacteristic(CHARACTERISTIC_UUID_SERIAL_NUMBER, NIMBLE_PROPERTY::READ);}
        if(pChr) pChr->setValue(serialNumber); else { ESP_LOGE(LOG_TAG, "Failed to create/get Serial Number Characteristic"); }
        // Firmware Revision
        pChr = pService->getCharacteristic(CHARACTERISTIC_UUID_FIRMWARE_REVISION);
        if(!pChr){ pChr = pService->createCharacteristic(CHARACTERISTIC_UUID_FIRMWARE_REVISION, NIMBLE_PROPERTY::READ);}
        if(pChr) pChr->setValue(firmwareRevision); else { ESP_LOGE(LOG_TAG, "Failed to create/get Firmware Revision Characteristic"); }
        // Hardware Revision
        pChr = pService->getCharacteristic(CHARACTERISTIC_UUID_HARDWARE_REVISION);
        if(!pChr){ pChr = pService->createCharacteristic(CHARACTERISTIC_UUID_HARDWARE_REVISION, NIMBLE_PROPERTY::READ);}
        if(pChr) pChr->setValue(hardwareRevision); else { ESP_LOGE(LOG_TAG, "Failed to create/get Hardware Revision Characteristic"); }
        // System ID (Example - Make sure systemID string is populated if used)
        // pChr = pService->getCharacteristic(CHARACTERISTIC_UUID_SYSTEM_ID);
        // if(!pChr){ pChr = pService->createCharacteristic(CHARACTERISTIC_UUID_SYSTEM_ID, NIMBLE_PROPERTY::READ);}
        // if(pChr) pChr->setValue(systemID); else { ESP_LOGE(LOG_TAG, "Failed to create/get System ID Characteristic"); }

    } else {
        ESP_LOGE(LOG_TAG, "Failed to get Device Information Service even after setPnP.");
    }

    // Set Security Mode
    // NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND); // Bonding enabled, MITM off, SC off (Legacy Pairing)
    // NimBLEDevice::setSecurityAuth(false, false, true); // No Bonding, No MITM, SC enabled
    NimBLEDevice::setSecurityAuth(true, false, false); // Bonding enabled, No MITM, SC off (Legacy Pairing - common default)
    ESP_LOGI(LOG_TAG, "Security settings configured.");

    // Start BLE services (HID, Device Info, Battery)
    ESP_LOGI(LOG_TAG, "Starting BLE services...");
    BleCompositeHIDInstance->_hid->startServices();
    BleCompositeHIDInstance->onStarted(pServer); // User callback

    // --- Configure and Start BLE advertisement ---
    ESP_LOGI(LOG_TAG, "Configuring BLE advertising...");
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising(); // Use NimBLEDevice static method is simpler
    pAdvertising->setAppearance(hidType); // Set appearance (e.g., HID_GAMEPAD, HID_KEYBOARD, HID_BRAILLE_DISPLAY?)
    ESP_LOGI(LOG_TAG, "Advertising Appearance set to: 0x%04X", hidType);

    // Advertise the HID Service UUID
    if (BleCompositeHIDInstance->_hid && BleCompositeHIDInstance->_hid->getHidService()) { // Check if HID service exists
         pAdvertising->addServiceUUID(BleCompositeHIDInstance->_hid->getHidService()->getUUID());
         ESP_LOGI(LOG_TAG, "HID Service UUID added to advertisement.");
    } else {
        ESP_LOGE(LOG_TAG, "HID Service is NULL, cannot add UUID to advertisement!");
    }

    // --- MODIFICACIÓN CLAVE INICIO ---
    // Explicitly set the advertised device name
    pAdvertising->setName(BleCompositeHIDInstance->deviceName);
    ESP_LOGI(LOG_TAG, "Advertised Name set to: %s", BleCompositeHIDInstance->deviceName.c_str());

    // Enable Scan Response (recommended for better discovery and sending full name if needed)
    pAdvertising->enableScanResponse(true);
    ESP_LOGI(LOG_TAG, "Scan Response enabled.");
    // Optional: Set specific Scan Response data if needed
    // pAdvertising->setScanResponseData(...);
    // --- MODIFICACIÓN CLAVE FIN ---

    // Start advertising
    ESP_LOGI(LOG_TAG, "Starting advertising...");
    bool success = pAdvertising->start();
    ESP_LOGI(LOG_TAG, "pAdvertising->start() call returned: %s", success ? "true" : "false");
    if(success) {
        ESP_LOGI(LOG_TAG, "Advertising started successfully as '%s'", BleCompositeHIDInstance->deviceName.c_str()); // Use INFO for successful start
    } else {
        ESP_LOGE(LOG_TAG, "Advertising failed to start!");
        // Consider adding error handling here
    }

    // Update battery level (initial level)
    BleCompositeHIDInstance->setBatteryLevel(BleCompositeHIDInstance->batteryLevel);
    ESP_LOGI(LOG_TAG, "Initial battery level set to: %d", BleCompositeHIDInstance->batteryLevel);

    // Start timed auto send for deferred reports (if enabled in config)
    if(BleCompositeHIDInstance->_configuration.getQueuedSending()){
        ESP_LOGI(LOG_TAG, "Starting timedSendDeferredReports task...");
        // Increased stack size slightly, adjust if needed. Priority 5 is standard.
        xTaskCreate(BleCompositeHIDInstance->timedSendDeferredReports, "autoSend", 4096, (void *)BleCompositeHIDInstance, 5, &BleCompositeHIDInstance->_autoSendTaskHandle);
        if (BleCompositeHIDInstance->_autoSendTaskHandle == NULL) {
            ESP_LOGE(LOG_TAG, "Failed to create timedSendDeferredReports task!");
        }
    } else {
        ESP_LOGI(LOG_TAG, "Queued sending task not enabled.");
    }

    // Task setup is complete, NimBLE handles events in its own tasks.
    ESP_LOGI(LOG_TAG, "taskServer setup finished. Task will exit.");
    vTaskDelete(NULL); // Allow this setup task to complete and be removed
}