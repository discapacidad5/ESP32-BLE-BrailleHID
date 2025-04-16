#ifndef BRAILLEDESCRIPTORS_H
#define BRAILLEDESCRIPTORS_H

#include <HIDTypes.h>

#define BRAILLE_REPORT_ID 0x40


// The report map describes the HID device (a braille display in this case) and
// the messages (reports in HID terms) sent and received.
static const uint8_t _brailleHIDReportDescriptor[] = {
  0x05, 0x41,        // Usage Page (Braille Display)
  0x09, 0x01,        // Usage (Braille Display)
  0xA1, 0x01,        // Collection (Application)
  REPORT_ID(1), BRAILLE_REPORT_ID,
  0x09, 0x02,        // Usage (Braille Row)
  0xA1, 0x02,        // Collection (Logical)

      0x09, 0x03,        // Usage (8 Dot Braille Cell)
      0x15, 0x00,        // Logical Minimum (0)
      0x26, 0xFF, 0x00,  // Logical Maximum (255)
      0x75, 0x08,        // Report Size (8 bits per cell) for 8 Dot
      0x95, 0x28,        // Report Count (40 cells)
      0x91, 0x02,        // Output (Data, Var, Abs)

  0xC0,              // End Collection (Logical)

0xC0               // End Collection (Application)
};


#endif // BRAILLEDESCRIPTORS_H
