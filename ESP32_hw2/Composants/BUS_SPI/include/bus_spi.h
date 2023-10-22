#ifndef BUS_SPI_H
#define BUS_SPI_H

void init_SPI(int mosi, int miso, int sclk, int cs, int host_spi);
int write_SPI16(unsigned char *buf);

#endif // BUS_SPI_H