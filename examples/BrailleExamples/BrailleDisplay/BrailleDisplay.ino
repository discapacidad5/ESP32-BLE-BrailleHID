#include <Arduino.h>
#include <BrailleDevice.h>
#include <BleCompositeHID.h>

BleCompositeHID compositeHID("HID", "hid", 100);
BrailleDevice* braille;

void setup()
{
    Serial.begin(115200);

    braille = new BrailleDevice();

    compositeHID.addDevice(braille);
    compositeHID.begin();

    Serial.println("Waiting for connection");
    delay(3000);
}

void loop()
{
    // do nothing
}
