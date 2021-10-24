#include "../Include/Hardware_i2c.h"


void vHW_i2c_Start(void){
	IdleI2C();
	StartI2C();
	IdleI2C();
}
UINT8 ui8HW_i2c_Write(UINT8 Data){
	UINT8 Ack;
	
	Ack=WriteI2C(Data);
	
	if(Ack==0){return(0);}
	else{return(1);}
}
void vHW_i2c_ReStart(void){
	RestartI2C();
	IdleI2C();
}
UINT8 ui8HW_i2c_Read(UINT8 Ack){
	UINT8 Data;
		
	Data=ReadI2C();
	if(Ack==0){
		AckI2C();
	}else{
		NotAckI2C();
	}
	IdleI2C();
	
	return(Data);
}

void vHW_i2c_Stop(void){
	StopI2C();
	IdleI2C();
}