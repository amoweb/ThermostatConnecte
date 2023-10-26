// initialise le bus SPI pour lecture écriture 16 bits dans buf[2]

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <string.h>
#include "esp_log.h"

#include "bus_spi.h"	

//===========================================================================
//                             DEFINITIONS                                  =
//===========================================================================

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  constantes */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
static const char *TAG = "Bus_SPI";
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  variables locales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
spi_device_handle_t spi;

////////////////////////////////////////////////////////////////////////
//                         gestion liaison SPI
////////////////////////////////////////////////////////////////////////

/*==========================================================*/
/* initialise SPI */ /*  */
/*==========================================================*/
esp_err_t init_SPI(int mosi, int miso, int sclk, int cs, int host_spi)
{
	esp_err_t ret;
	spi_bus_config_t buscfg={
        .mosi_io_num = mosi,
        .miso_io_num = miso,
        .sclk_io_num = sclk,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1
	};
	spi_device_interface_config_t devcfg={
					.command_bits=0,
					.address_bits=0,
					.dummy_bits=0,
					.clock_speed_hz=2000000,
// 					.clock_speed_hz=500000,
					.duty_cycle_pos=128,
					.mode=0,				// NOTE mode 0
							// !!! à verifier
// 					.input_delay_ns=125,
					.spics_io_num = cs,
// 					.cs_ena_posttrans=3,
			// !!! pour éviter l'envoi de data sans controle du statut queue_size=1
					.queue_size=1
	};
	// NOTE DMA désactivé
// 	ret=spi_bus_initialize(host_spi, &buscfg, SPI_DMA_CH_AUTO);
	ret=spi_bus_initialize(host_spi, &buscfg, 0);
    if (ret != ESP_OK) return ret;
	assert(ret==ESP_OK);
	ret=spi_bus_add_device(host_spi, &devcfg, &spi);
    if (ret != ESP_OK) return ret;
	assert(ret==ESP_OK);
	ESP_LOGW(TAG, "------------- init_SPI - core = %d", xPortGetCoreID());
    return ret;
}

/*==========================================================*/
/* envoi 16 bits sur SPI et retourne 16 bits lus */  /*  */
/*==========================================================*/
int write_SPI16(unsigned char *buf) {
	esp_err_t ret;
	spi_transaction_t t;
	memset(&t, 0, sizeof(t));
	t.length = 2 * 8;
	t.rxlength = 2 * 8;
	t.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
	t.tx_data[0] = buf[0];
	t.tx_data[1] = buf[1];

	ret = spi_device_polling_transmit(spi, &t);

	buf[0] = t.rx_data[0];
	buf[1] = t.rx_data[1];
	
	if(ret != ESP_OK) {
		ESP_LOGE(TAG,"ERREUR spi_device_polling_transmit ret %i cmd 0x%02X 0x%02X ", ret, buf[0], buf[1]);
	}
	return ret;
}


  
