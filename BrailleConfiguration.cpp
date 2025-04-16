#include "BrailleConfiguration.h"
#include "BrailleDevice.h"
#include "BrailleDescriptors.h"
#include "HIDTypes.h"

BrailleConfiguration::BrailleConfiguration() :
    BaseCompositeDeviceConfiguration(BRAILLE_REPORT_ID)
{
}

BrailleConfiguration::BrailleConfiguration(uint8_t reportId) :
    BaseCompositeDeviceConfiguration(reportId)
{
}

uint8_t BrailleConfiguration::getDeviceReportSize() const
{
    return sizeof(_brailleHIDReportDescriptor);
}

size_t BrailleConfiguration::makeDeviceReport(uint8_t* buffer, size_t bufferSize) const
{
    size_t hidDescriptorSize = sizeof(_brailleHIDReportDescriptor);
    if(hidDescriptorSize < bufferSize){
        memcpy(buffer, _brailleHIDReportDescriptor, hidDescriptorSize);
    } else {
        return -1;
    }
    
    return hidDescriptorSize;
}