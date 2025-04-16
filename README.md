# ESP32-BLE-CompositeHID

Forked from ESP32-BLE-Gamepad by lemmingDev to provide support support for composite human interface devices.

This library will let your ESP32 microcontroller behave as a bluetooth mouse, keyboard, gamepad (XInput or generic), or a combination of any of these devices.

## License
Published under the MIT license. Please see license.txt.

## Dependencies
 - Nimble-Arduino. Actual version support Nimble-Arduino-2.1.2
 - Callback. Available from the Arduino library manager or [here](https://github.com/tomstewart89/Callback).

## Optional dependencies
 - Keypad 
 - Bounce2

 ## Braille display features

 - [x] Identify as a 40 cell 8 Dot braille display
 - [x] Receive data from the device in an output report
 - [x] Compatible with VoiceOver
 - [x] Compatible with NVDA
 - [ ] Compatible with TalkBack
 - [ ] Properly read and interpret the data
   
## XInput gamepad features

 - [x] All buttons and joystick axes available
 - [x] XBox One S and XBox Series X controller support
 - [x] Linux XInput support (Kernel version < 6.5 only supports the XBox One S controller)
 - [x] Haptic feedback callbacks for strong and weak motor rumble support
 - [ ] LED support (pull requests welcome)

## Generic gamepad features (from ESP32-BLE-Gamepad)

 - [x] Button press (128 buttons)
 - [x] Button release (128 buttons)
 - [x] Axes movement (6 axes (configurable resolution up to 16 bit) (x, y, z, rZ, rX, rY) --> (Left Thumb X, Left Thumb Y, Right Thumb X, Right Thumb Y, Left Trigger, Right Trigger))
 - [x] 2 Sliders (configurable resolution up to 16 bit) (Slider 1 and Slider 2)
 - [x] 4 point of view hats (ie. d-pad plus 3 other hat switches)
 - [x] Simulation controls (rudder, throttle, accelerator, brake, steering)
 - [x] Special buttons (start, select, menu, home, back, volume up, volume down, volume mute) all disabled by default

## Mouse features
 - [x] Configurable button count
 - [x] X and Y axes
 - [ ] Configurable axes

## Keyboard features
 - [x] Supports most USB HID scancodes
 - [x] Media key support
 - [x] LED callbacks for caps/num/scroll lock keys

## Composite BLE host features (adapted from ESP32-BLE-Gamepad)
 - [x] Configurable HID descriptors per device
 - [x] Configurable VID and PID values
 - [x] Configurable BLE characteristics (name, manufacturer, model number, software revision, serial number, firmware revision, hardware revision)	
 - [x] Report optional battery level to host
 - [x] Uses efficient NimBLE bluetooth library
 - [x] Compatible with Windows
 - [x] Compatible with Android (Android OS maps default buttons / axes / hats slightly differently than Windows)
 - [x] Compatible with Linux (limited testing)
 - [x] Compatible with MacOS X (limited testing)
 - [ ] Compatible with iOS (No - not even for accessibility switch - This is not a “Made for iPhone” (MFI) compatible device)

## Installation
- (Make sure your IDE of choice has support for ESP32 boards available. [Instructions can be found here.](https://github.com/espressif/arduino-esp32#installation-instructions))
- Download the zip version of this library from Github using either the "Code" -> "Download Zip" button or by cloning this repository to your Arduino library folder. If using the downloaded zip method, in the Arduino IDE go to "Sketch" -> "Include Library" -> "Add .ZIP Library..." and select the file you just downloaded.
- Repeat the previous step but for the [NimBLE library](https://github.com/h2zero/NimBLE-Arduino)
   - If you are using the Arduino IDE, you can use the included NimBLE library installed from the library manager
- Using the Arduino IDE Library manager, download the "Callback" library by Tom Stewart.
- In the Arduino IDE, you can now go to "File" -> "Examples" -> "ESP32-BLE-CompositeHID" and select an example to get started.

## Example

``` C++
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

```

## Credits for ESP32-BLE-CompositeHID

Credit goes to lemmingDev for his work on [ESP32-BLE-Gamepad](https://github.com/lemmingDev/ESP32-BLE-Gamepad) which most of the gamepad portion of this library was based upon. 

USB HID codes for keyboards created by MightyPork, 2016 (see KeyboardHIDCodes.h)

## Credits for ESP32-BLE-Gamepad

Credits to [T-vK](https://github.com/T-vK) as this library is based on his ESP32-BLE-Mouse library (https://github.com/T-vK/ESP32-BLE-Mouse) that he provided.

Credits to [chegewara](https://github.com/chegewara) as the ESP32-BLE-Mouse library is based on [this piece of code](https://github.com/nkolban/esp32-snippets/issues/230#issuecomment-473135679) that he provided.

Credits to [wakwak-koba](https://github.com/wakwak-koba) for the NimBLE [code](https://github.com/wakwak-koba/ESP32-NimBLE-Gamepad) that he provided.


You might also be interested in:
- [ESP32-BLE-Mouse](https://github.com/T-vK/ESP32-BLE-Mouse)
- [ESP32-BLE-Keyboard](https://github.com/T-vK/ESP32-BLE-Keyboard)

or the NimBLE versions at

- [ESP32-NimBLE-Mouse](https://github.com/wakwak-koba/ESP32-NimBLE-Mouse)
- [ESP32-NimBLE-Keyboard](https://github.com/wakwak-koba/ESP32-NimBLE-Keyboard)
- [ESP32-BLE-Gamepad](https://github.com/lemmingDev/ESP32-BLE-Gamepad)
