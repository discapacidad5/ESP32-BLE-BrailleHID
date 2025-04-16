#ifndef BRAILLE_CONFIGURATION_H
#define BRAILLE_CONFIGURATION_H

#include <BaseCompositeDevice.h>

class BrailleConfiguration : public BaseCompositeDeviceConfiguration
{   
public:
    BrailleConfiguration();
    BrailleConfiguration(uint8_t reportId);
    uint8_t getDeviceReportSize() const override;
    size_t makeDeviceReport(uint8_t* buffer, size_t bufferSize) const override;
};

#endif