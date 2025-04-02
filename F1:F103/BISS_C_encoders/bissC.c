/*
 * This file is part of the encoders project.
 * Copyright 2025 Edward V. Emelianov <edward.emelianoff@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "bissC.h"
#include "usb_dev.h"
#ifdef EBUG
#include "strfunc.h"
#endif
#include "flash.h"

#define MAX_BITSTREAM_UINTS 4  // 4 * 32 = 128 bits capacity
// min/max zeros before preambule
#define MINZEROS    4
#define MAXZEROS    40

static uint32_t bitstream[MAX_BITSTREAM_UINTS] = {0};

// Get bit at position n from uint32_t array (MSB-first packing)
static inline uint8_t get_bit(uint8_t n) {
    return (bitstream[n >> 5] >> (31 - (n & 0x1F))) & 1;
}

// Convert bytes to packed uint32_t bitstream (MSB-first)
static void bytes_to_bitstream(const uint8_t *bytes, uint32_t num_bytes, uint32_t *num_bits) {
    *num_bits = 0;
    uint32_t current = 0;
    uint32_t pos = 0;

    for(uint32_t i = 0; i < num_bytes; i++){
        for(int8_t j = 7; j >= 0; j--){ // MSB first
            current = (current << 1) | ((bytes[i] >> j) & 1);
            if (++(*num_bits) % 32 == 0){
                bitstream[pos++] = current;
                current = 0;
            }
        }
    }
    /* Store remaining bits if any
    if(*num_bits % 32 != 0){
        current <<= (32 - (*num_bits % 32));
        bitstream[pos] = current;
    }*/
}

// Compute CRC-6 using polynomial x^6 + x + 1
static uint8_t compute_crc(uint8_t start_bit, uint8_t num_bits) {
    uint8_t crc = 0x00;
    for(uint32_t i = 0; i < num_bits; i++) {
        uint8_t bit = get_bit(start_bit + i);
        uint8_t msb = (crc >> 5) ^ bit;
        crc = ((crc << 1) & 0x3F);
        if (msb) crc ^= 0x03;
    }
    DBG("Compute CRC:");
    DBGs(uhex2str(crc));
    return crc;
}

BiSS_Frame parse_biss_frame(const uint8_t *bytes, uint32_t num_bytes){
    BiSS_Frame frame = {0};
    uint32_t num_bits = 0;
    register uint8_t enclen = the_conf.encbits;

    bytes_to_bitstream(bytes, num_bytes, &num_bits);

    // Preamble detection state machine
    enum {SEARCH_START, SEARCH_ZERO, COUNT_ZEROS} state = SEARCH_START;
    uint32_t zero_count = 0;
    uint32_t data_start = 0;
    int found = 0;

    for(uint32_t i = 0; i < num_bits && !found; i++){
        uint8_t curbit = get_bit(i);
        switch(state){
        case SEARCH_START:
            if(curbit){
                state = SEARCH_ZERO;
            }
            break;
        case SEARCH_ZERO:
            if(!curbit){
                state = COUNT_ZEROS;
                zero_count = 1;
            }
            break;
        case COUNT_ZEROS:
            if(!curbit){
                zero_count++;
            }else{
                if(zero_count >= MINZEROS && zero_count <= MAXZEROS){
                    if((i + 1) < num_bits && !get_bit(i + 1)){
                        data_start = i + 2;
                        found = 1;
                    }
                }
                state = SEARCH_START;
            }
            break;
        }
    }

    // Validate frame structure
    if(!found || (data_start + enclen+8) > num_bits){
        frame.frame_valid = 0;
        return frame;
    }
    DBG("Data start:");
    DBGs(u2str(data_start));
    // Extract 26-bit data
    for(int i = 0; i < enclen; i++){
        frame.data <<= 1;
        frame.data |= get_bit(data_start + i);
    }

    // Extract 2-bit flags (INVERTED!)
    frame.error = !get_bit(data_start + enclen);
    frame.warning = !get_bit(data_start + enclen + 1);

    // Extract and validate CRC
    uint8_t received_crc = 0;
    for(uint32_t i = 0; i < 6; i++){
        received_crc <<= 1;
        // CRC transmitted MSB and inverted! Include position data and EW bits
        if(!get_bit(data_start + enclen + 2 + i)) received_crc |= 1;
    }
    DBG("received CRC:");
    DBGs(uhex2str(received_crc));
    frame.crc_valid = (compute_crc(data_start, enclen + 2) == received_crc);
    frame.frame_valid = 1;
    return frame;
}
