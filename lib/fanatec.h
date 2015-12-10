/*
 * Copyright (C) 2015 darknao
 * https://github.com/darknao/btClubSportWheel
 *
 * This file is part of btClubSportWheel.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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
#ifndef fanatec_h
#define fanatec_h


#include <stdint.h>
#include <avr/pgmspace.h>
#include <SPI.h>
//#include <SPIFIFO.h>

#define CS 10
#define FRXSIZE 34

//void transfer_data(const uint8_t* out, uint8_t* in, uint8_t length);

/*
Fanatec Packet out:
headers
 ^  wheel ID?
 |  ^  display
 |  |    ^    revlights
 |  |    |        ^  rumbles
 |  |    |        |     ^                              nothing                                  CRC8
|__|__|________|_____|_____|____________________________________________________________________|__|
 a5 03 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 XX
 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32

A5 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 5A
a5 03  0x0  0x0  0x0  0x0  0x0  0x0 0x48 0x83 0x87 0x43 0x5b 0x9a 0x39 0xa0 0xc9 0xb5 0xd8 0x19 0x72 0x30 0x28 0xfa 0x62 0xf7 0x93  0xc 0xcb 0x98 0xd0 0x12 0x42
*/
#pragma pack(push, 1)
struct csw_in_t {
  union {
    struct {
      //bool unk:1;
      uint8_t header;
      uint8_t id;
      uint8_t buttons[3];
      int8_t axisX;
      int8_t axisY;
      int8_t encoder;
      
      uint8_t garbage[24];
      uint8_t crc;
      // +3 padding bits
    };
    uint8_t raw[34];
  };
};

struct csw_out_t {
  union {
    struct {
      uint8_t header;
      uint8_t id;
      uint8_t disp[3];
      uint16_t leds;
      uint8_t rumble[2];

      uint8_t nothing[23];
      uint8_t crc;
    };
    uint8_t raw[33];
  };
};
#pragma pack(pop)
void transfer_data(csw_out_t* out, csw_in_t* in, uint8_t length);
//void transfer_data2(const uint8_t* out, uint8_t* in, uint8_t length);

uint8_t crc8(const uint8_t* buf, uint8_t length);

#endif

