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
#ifndef _FANATEC_H_
#define _FANATEC_H_

#include <SPI.h>
#define CS 10

#define FORMULA_RIM 1
#define BMW_RIM 2
#define RSR_RIM 3
#define UNIHUB 4

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
      uint8_t btnHub[2];
      uint8_t btnPS[2];

      uint8_t garbage[20];
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

uint8_t crc8(const uint8_t* buf, uint8_t length);
void transfer_data(csw_out_t* out, csw_in_t* in, uint8_t length);


void fsetup();
#endif