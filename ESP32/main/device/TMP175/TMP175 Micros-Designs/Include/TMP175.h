#include "Hardware_i2c.h"

#define SHUTDOWN_MODE_OFF  0x00
#define SHUTDOWN_MODE_ON   0x01
#define COMPARATOR_MODE    0x00
#define INTERRUPT_MODE     0x02
#define POLARITY_0         0x00
#define POLARITY_1         0x04
#define FAULT_QUEUE_1      0x00
#define FAULT_QUEUE_2      0x08
#define FAULT_QUEUE_4      0x10
#define FAULT_QUEUE_6      0x18
#define RESOLUTION_9       0x00
#define RESOLUTION_10      0x20
#define RESOLUTION_11      0x40
#define RESOLUTION_12      0x60
#define ONE_SHOT           0x80

void vSetConfigurationTMP175(UINT8 Config,UINT8 Address);
void vSetTemperatureLowTMP175(float Value);
float fReadTemperatureLowTMP175(void);
void vSetTemperatureHighTMP175(float Value);
float fReadTemperatureHighTMP175(void);
float fReadTemperatureTMP175(void);
void vStartSingleConversionTMP175(void);
