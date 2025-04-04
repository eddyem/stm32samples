
#pragma once
#include <stdint.h>

// two SPI interfaces for two sensors
#define AMOUNT_OF_SPI   (2)
// static buffer size
#define ENCODER_BUFSZ_MAX   (32)
// encoder resolution
#define ENCRESOL_MIN    (8)
#define ENCRESOL_MAX    (32)

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
uint8_t *spi_read_enc(uint8_t encno);
