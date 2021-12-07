/*
   \file TMP175.h
   \version: 1.0
   
   \brief Este fichero contiene funciones para control de TMP175/75 
   
   \web www.micros-designs.com.ar
   \date 02/02/11
   
 *- Version Log --------------------------------------------------------------*
 *   Fecha       Autor                Comentarios                             *
 *----------------------------------------------------------------------------*
 * 02/02/11      Suky        Original                                         *
 *----------------------------------------------------------------------------*/ 
///////////////////////////////////////////////////////////////////////////
////                                                                   ////
////                                                                   ////
////        (C) Copyright 2011 www.micros-designs.com.ar               ////
//// Este código puede ser usado, modificado y distribuido libremente  ////
//// sin eliminar esta cabecera y  sin garantía de ningún tipo.        ////
////                                                                   ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////

#include "../Include/TMP175.h"

UINT8 	_AddressTMP;
float 	Factor;
UINT16 	Mascara;
UINT8 	Rotar;


void vSetConfigurationTMP175(UINT8 Config,UINT8 Address){
   
   _AddressTMP=Address<<1;
   switch(Config&0x60){
      case 0x00:
         Factor=0.5;
         Mascara=0x00FF;
         Rotar=3;
      break;
      case 0x20:
         Factor=0.25;
         Mascara=0x01FF;
         Rotar=2;
      break;
      case 0x40:
         Factor=0.125;
         Mascara=0x03FF;
         Rotar=1;
      break;
      case 0x60:
         Factor=0.0625;
         Mascara=0x07FF;
         Rotar=0;
      break;
   }

   vHW_i2c_Start();
   ui8HW_i2c_Write(_AddressTMP&0xFE);
   ui8HW_i2c_Write(0x01);
   ui8HW_i2c_Write(Config);
   vHW_i2c_Stop();  
}

void vSetTemperatureLowTMP175(float Value){
   UINT16 Temp;
   
   if(Value>=0.0){
      Temp=Value/0.0625;
   }else{
      Temp=(Value+128.0)/0.0625;
      Temp!=0x0800;
   }
   Temp<<=4;

   vHW_i2c_Start();
   ui8HW_i2c_Write(_AddressTMP&0xFE);
   ui8HW_i2c_Write(0x02);
   ui8HW_i2c_Write(*((char *)&Temp+1));
   ui8HW_i2c_Write(*((char *)&Temp));
   vHW_i2c_Stop();
}

float fReadTemperatureLowTMP175(void){
   UINT16 Temp;
   float Cal;
   
   vHW_i2c_Start();
   ui8HW_i2c_Write(_AddressTMP&0xFE);
   ui8HW_i2c_Write(0x02);
   vHW_i2c_ReStart();
   ui8HW_i2c_Write(_AddressTMP | 0x01);
   *((char *)&Temp+1)=ui8HW_i2c_Read(SEND_ACK);
   *((char *)&Temp)=ui8HW_i2c_Read(SEND_NACK);
   vHW_i2c_Stop();   
   
   Temp>>=4;
   Cal=Temp*0.0625;
   if((0x0800&Temp)!=0x0000){
      Cal-=128.0;
   }
   
   return(Cal);
}

void vSetTemperatureHighTMP175(float Value){
   UINT16 Temp;
   
   if(Value>=0.0){
      Temp=Value/0.0625;
   }else{
      Temp=(Value+128.0)/0.0625;
      Temp!=0x0800;
   }
   Temp<<=4;
   
   vHW_i2c_Start();
   ui8HW_i2c_Write(_AddressTMP&0xFE);
   ui8HW_i2c_Write(0x03);
   ui8HW_i2c_Write(*((char *)&Temp+1));
   ui8HW_i2c_Write(*((char *)&Temp));
   vHW_i2c_Stop();
}

float fReadTemperatureHighTMP175(void){
   UINT16 Temp;
   float Cal;
   
   vHW_i2c_Start();
   ui8HW_i2c_Write(_AddressTMP&0xFE);
   ui8HW_i2c_Write(0x03);
   vHW_i2c_ReStart();
   ui8HW_i2c_Write(_AddressTMP | 0x01);
   *((char *)&Temp+1)=ui8HW_i2c_Read(SEND_ACK);
   *((char *)&Temp)=ui8HW_i2c_Read(SEND_NACK);
   vHW_i2c_Stop();   
   
   Temp>>=4;
   Cal=Temp*0.0625;
   if((0x0800&Temp)!=0x0000){
      Cal-=128.0;
   }
   
   return(Cal);
}

float fReadTemperatureTMP175(void){
   UINT16 Temp;
   float Cal;
   
   vHW_i2c_Start();
   ui8HW_i2c_Write(_AddressTMP&0xFE);
   ui8HW_i2c_Write(0x00);
   vHW_i2c_ReStart();
   ui8HW_i2c_Write(_AddressTMP | 0x01);
   *((char *)&Temp+1)=ui8HW_i2c_Read(SEND_ACK);
   *((char *)&Temp)=ui8HW_i2c_Read(SEND_NACK);
   vHW_i2c_Stop();   
   
   Temp>>=(Rotar+4);
   Cal=((float)Factor*(Temp&Mascara));
   if(((~Mascara)&Temp)!=0x0000){
      Cal-=128.0;
   }
   
   return(Cal);
}

// Only SHUTDOWN MODE 
void vStartSingleConversionTMP175(void){
   UINT8 Temp;

   vHW_i2c_Start();
   ui8HW_i2c_Write(_AddressTMP&0xFE);
   ui8HW_i2c_Write(0x01);
   vHW_i2c_ReStart();
   ui8HW_i2c_Write(_AddressTMP | 0x01);
   Temp=ui8HW_i2c_Read(SEND_NACK);
   vHW_i2c_Stop();     
   
   vHW_i2c_Start();
   ui8HW_i2c_Write(_AddressTMP&0xFE);
   ui8HW_i2c_Write(0x01);
   ui8HW_i2c_Write(Temp|0x80);
   vHW_i2c_Stop(); 
}
