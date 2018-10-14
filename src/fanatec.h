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

#define NO_RIM 0
#define BMW_RIM 1
#define FORMULA_RIM 2
#define PORSCHE_RIM 3
#define UNIHUB 4

#define XBOXHUB 6
#define CSLP1XBOX 7
#define CSLP1PS4 8
#define CSLMCLGT3 9

enum wheel_type {
  NO_WHEEL,
  CSW_WHEEL,
  CSL_WHEEL,
  MCL_WHEEL
};

#pragma pack(push, 1)
struct csw_in_t {
  union {
    struct {
      uint8_t header;
      uint8_t id;
      uint8_t buttons[3];
      uint8_t axisX;
      uint8_t axisY;
      int8_t encoder;
      uint8_t btnHub[2];
      uint8_t btnPS[2];

      uint8_t garbage[19];
      uint8_t fwvers;
      uint8_t crc;
    };
    uint8_t raw[33];
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


struct csl_out_t {
  union {
    struct {
      uint8_t disp;
      uint8_t selector;
    };
    uint8_t raw[2];
  };
};

struct csl_in_t {
  union {
    struct {
      uint8_t buttons;
      uint8_t nothing;
    };
    uint8_t raw[2];
  };
};

struct mcl_in_t {
  union {
    struct {
      uint8_t header;
      uint8_t id;
      uint8_t buttons[3];
      uint8_t axisX;
      uint8_t axisY;
      int8_t encoder;
      uint8_t btnHub[2];
      uint8_t btnPS[2];

      uint8_t garbage[19];
      uint8_t fwvers;
      uint8_t crc;
    };
    uint8_t raw[33];
  };
};

struct mcl_out_t {
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

wheel_type detectWheelType();
uint8_t getFirstByte();
uint8_t crc8(const uint8_t* buf, uint8_t length);
void transferCswData(csw_out_t* out, csw_in_t* in, uint8_t length);
void transferCslData(csl_out_t* out, csl_in_t* in, uint8_t length, uint8_t selector);
void transferMclData(mcl_out_t* out, mcl_in_t* in, uint8_t length);
uint8_t csw7segToCsl(uint8_t csw_disp);
uint8_t csw7segToAscii(uint8_t csw_disp);

uint8_t cswLedsToCsl(uint16_t csw_leds);

void fsetup();
#endif