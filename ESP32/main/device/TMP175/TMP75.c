#include <stdio.h>
#include <unistd.h>

#define ESP32

#include "driver/i2c.h"
#include "info.h"
#include "TMP75.h"
// #include "bcm2835.h"

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* defines */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#define msdelai(us) usleep(us*1000)

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  constantes */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
const float precision[4] = {0.0625, 0.125, 0.25, 0.5};
const int code_precision[4] = {0b01100000, 0b01000000, 0b00100000, 0b00000000};

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  fonctions globales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  variables locales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
static int idx_precision;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  ESP32 */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
void esp32_i2c_write_read(uint16_t address, uint8_t* writeData, size_t writeDataSize, uint8_t* readData, size_t readDataSize) {

    // Create a new I2C command
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);

    i2c_master_write_byte(cmd_handle, address, true);
    
    for(int i = 0; i < writeDataSize - 1; i++) {
        bool ack = ( i == writeDataSize - 1);
        i2c_master_write_byte(cmd_handle, writeData[i], ack);
    }

    for(int i = 0; i < readDataSize ; i++) {
        i2c_master_read(cmd_handle, readData, 1, I2C_MASTER_NACK);
    }

    i2c_master_stop(cmd_handle);

    // Send the command
    i2c_master_cmd_begin(0, cmd_handle, 100 /* ticks before timeout */);

    // Delete the command
    i2c_cmd_link_delete(cmd_handle);
}



/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  fonctions locales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/*==========================================================*/
/*	converti des degrés en hexa*/  /* 
		résolution 0.0625 °C
*/
/*==========================================================*/
int deg_hexa(float temp) {
	int hex = (int) temp/precision[idx_precision];
	INFO("temp HEXA : (%.2f °C) 0x%04X", temp, ((hex & 0xFF0)>>4) | ((hex & 0x00F)<<12));
	return ((hex & 0xFF0)>>4) | ((hex & 0x00F)<<12);
}

/*==========================================================*/
/*	conversion valeur hexa en float  */  /* 
		converti la valeur de température en hexa en float en fonction de la resolution
*/
/*==========================================================*/
float conversion (int hexa) {
	float temp;
	int cal;
	switch(idx_precision){
		case PREC_0_0625 :
			cal = ((hexa  & 0xF000) >> 12) | ((hexa & 0x00FF)<<4);
		break;
		case PREC_0_125 :
			cal = ((hexa  & 0xE000) >> 13) | ((hexa & 0x00FF)<<3);
		break;
		case PREC_0_25 :
			cal = ((hexa  & 0xC000) >> 14) | ((hexa & 0x00FF)<<2);
		break;
		case PREC_0_5 :
			cal = ((hexa  & 0x8000) >> 15) | ((hexa & 0x00FF)<<1);
		break;
        default:
            return 42.24;
        break;
	}
	temp = cal * precision[idx_precision];
	INFO("hexa 0x%04X - cal 0x%04X - temp %.3f °C",hexa, cal, temp);
	return temp;
}

/*==========================================================*/
/*	initialise les seuils bas et haut */ /* 
	retourne 1 si Ok, sion 0
*/
/*==========================================================*/
int tmp75_seuils(float bas, float haut) {
	uint16_t ret=0;
	float temp;
	char wbuf[8];
	char buf[8];
	int i;
	uint8_t data;
	
	/* seuil bas */
	ret = deg_hexa(bas);
	wbuf[0] = Reg_Temp_basse;
	wbuf[1] = ret & 0x00FF;
	wbuf[2] = (ret & 0x0F00) >> 8;
	INFO("bcm2835_i2c_write = 0x%02X 0x%02X 0x%02X", wbuf[0], wbuf[1], wbuf[2]);

#ifdef ESP32
    esp32_i2c_write_read(adresse_TMP75, (uint8_t*)&wbuf, 3, NULL, 0);
    data = 0;
#else
	data = bcm2835_i2c_write(wbuf, 3);
#endif

	if(data != 0) {
		INFO("Write Reg_config Result = %d", data);
		return 0;
	}
	/*   1ere methode (fonctionne avec TMP75)
	*/
#ifdef ESP32
    esp32_i2c_write_read(adresse_TMP75, (uint8_t*)&wbuf, 3, (uint8_t*)&buf, 2);
    data = 0;
#else
 	data = bcm2835_i2c_write_read_rs(wbuf, 1, buf, 2);
#endif
	if(data != 0){
		INFO("Write Lecture Result = %i", data);
		return 0;
	}
	/* ou 2eme methode (fonctionne avec TMP75)
	data = bcm2835_i2c_write(wbuf, 1);
	if(data != 0)
		INFO("Write Lecture Result = %i", data);
	data = bcm2835_i2c_read(buf, 2);
	if(data != 0)
		INFO("Read Result = %d", data);   
	 */
	ret = buf[0] + (buf[1] << 8);	
	temp = conversion(ret);
	INFO("seuil bas : %.3f °C", temp);

	/* seuil haut */
	ret = deg_hexa(haut);
	wbuf[0] = Reg_Temp_haute;
	wbuf[1] = ret & 0x00FF;
	wbuf[2] = (ret & 0x0F00) >> 8;

#ifdef ESP32
    esp32_i2c_write_read(adresse_TMP75, (uint8_t*)&wbuf, 3, NULL, 0);
    data = 0;
#else
	data = bcm2835_i2c_write(wbuf, 3);
#endif
	if(data != 0) {
		INFO("Write Reg_config Result = %d", data);
		return 0;
	}
#ifdef ESP32
    esp32_i2c_write_read(adresse_TMP75, (uint8_t*)&wbuf, 3, (uint8_t*)&buf, 2);
    data = 0;
#else
 	data = bcm2835_i2c_write_read_rs(wbuf, 1, buf, 2);
#endif
	if(data != 0){
		INFO("Write Lecture Result = %i", data);
		return 0;
	}
	ret = buf[0] + (buf[1] << 8);	
	temp = conversion(ret);
	INFO("seuil haut : %.3f °C", temp);

	return 1;
}

/*==========================================================*/
/*	init TMP75 */ /* 
initialise I2C et TMP75
retourne 1 si Ok, sion 0
*/
/*==========================================================*/
int tmp75_init(int resolution) {
	uint32_t clk_div;
	idx_precision = resolution;
	
#ifdef ESP32
#else
	if (!bcm2835_init()) {
		INFO("bcm2835_init failed. Are you running as root??\n");
		return 0;
	}
      
	if (!bcm2835_i2c_begin()) {
		INFO("bcm2835_i2c_begin failed. Are you running as root??\n");
		return 0;
	}
	
	INFO("Slave address set to: 0x%02X", adresse_TMP75);   
	bcm2835_i2c_setSlaveAddress(adresse_TMP75);
// 	clk_div = BCM2835_I2C_CLOCK_DIVIDER_150;
// 	clk_div = BCM2835_I2C_CLOCK_DIVIDER_626;
// 	clk_div = BCM2835_I2C_CLOCK_DIVIDER_2500;
// 	INFO("Clock divider set to: %d", clk_div);
// 	bcm2835_i2c_setClockDivider(clk_div);
	clk_div = 1000000;
	INFO("Baudrate set to: %d", clk_div);
	bcm2835_i2c_set_baudrate(1000000);
#endif
	
	tmp75_seuils(18, 30);

	return 1;
}

/*==========================================================*/
/*	Arrete I2C et lib BCM2835 */ /* 
	rétablit les fonctions initiales du GPIO
	retourne 1 si Ok, sion 0
*/
/*==========================================================*/
int tmp75_close(void) {
#ifdef ESP32
    return 0;
#else
	bcm2835_i2c_end();   
	return bcm2835_close();
#endif
}

/*==========================================================*/
/*	lecture TMP75 */ /* 
	realise une lecture renvoie le resultat dans temperature[0]
	retourne 1 si Ok, sion 0
*/
/*==========================================================*/
int tmp75_lect(float *temperature) {
	uint16_t ret=0;
	char wbuf[8];
	char buf[8];
	int i;
	uint8_t data;
	
// 	for (i=0; i<8; i++)
// 		buf[i] = 'n';

	wbuf[0] = Reg_config;
	wbuf[1] = TMP75_SD | code_precision[idx_precision] | TMP75_OS;
	
	INFO("bcm2835_i2c_write = 0x%02X 0x%02X", wbuf[0], wbuf[1]);
#ifdef ESP32
    esp32_i2c_write_read(adresse_TMP75, (uint8_t*)&wbuf, 2, NULL, 0);
    data = 0;
#else
	data = bcm2835_i2c_write(wbuf, 2);
#endif
	if(data != 0) {
		INFO("Write Reg_config Result = %d", data);
		return 0;
	}
	
	/* delai de conversion de TMP75 =  fct de resolution */
	msdelai(250/(idx_precision+1));

	wbuf[0] = Reg_Lecture;	
	/*   1ere methode (fonctionne avec TMP75)
	*/
#ifdef ESP32
    esp32_i2c_write_read(adresse_TMP75, (uint8_t*)&wbuf, 3, (uint8_t*)buf, 2);
    data = 0;
#else
 	data = bcm2835_i2c_write_read_rs(wbuf, 1, buf, 2);
#endif
	if(data != 0){
		INFO("Write Lecture Result = %i", data);
		return 0;
	}

	/* ou 2eme methode (fonctionne avec TMP75)
	data = bcm2835_i2c_write(wbuf, 1);
	if(data != 0)
		INFO("Write Lecture Result = %i", data);
	data = bcm2835_i2c_read(buf, 2);
	if(data != 0)
		INFO("Read Result = %d", data);   
	 */
		
/*
	for (i=0; i<8; i++) {
		if(buf[i] != 'n')
			INFO("Read Buf[%d] = 0x%02X", i, buf[i]);
	}    
*/		
	ret = buf[0] + (buf[1] << 8);
	
	temperature[0] = conversion(ret);
	INFO("\t\t ----- temperature : %.2f °C ------", temperature[0]);

	return 1;
}

