#include <p18cxxx.h>
#include <i2c.h>
#include "GenericTypeDefs.h"

#define SEND_ACK	0
#define SEND_NACK	1

void vHW_i2c_Start(void);

UINT8 ui8HW_i2c_Write(UINT8 Data);

void vHW_i2c_ReStart(void);

UINT8 ui8HW_i2c_Read(UINT8 Ack);

void vHW_i2c_Stop(void);
