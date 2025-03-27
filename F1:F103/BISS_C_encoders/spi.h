
#pragma once
#include <stdint.h>

#define AMOUNT_OF_SPI   (2)

#define ENCODER_BUFSZ   (8)

typedef enum{
    SPI_NOTREADY,
    SPI_READY,
    SPI_BUSY
} spiStatus;

extern spiStatus spi_status[AMOUNT_OF_SPI+1];

void spi_onoff(uint8_t idx, uint8_t on);
void spi_deinit(uint8_t idx);
void spi_setup(uint8_t idx);
int spi_start_enc(int encodernum);
int spi_read_enc(uint8_t encno, uint8_t buf[8]);
