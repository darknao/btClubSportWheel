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

#include "fanatec.h"

SPISettings settingsA(12000000, MSBFIRST, SPI_MODE0);

// CRC lookup table with polynomial of 0x131
PROGMEM prog_uchar _crc8_table[256] = {
  0, 94, 188, 226, 97, 63, 221, 131,
  194, 156, 126, 32, 163, 253, 31, 65,
  157, 195, 33, 127, 252, 162, 64, 30,
  95, 1, 227, 189, 62, 96, 130, 220,
  35, 125, 159, 193, 66, 28, 254, 160,
  225, 191, 93, 3, 128, 222, 60, 98,
  190, 224, 2, 92, 223, 129, 99, 61,
  124, 34, 192, 158, 29, 67, 161, 255,
  70, 24, 250, 164, 39, 121, 155, 197,
  132, 218, 56, 102, 229, 187, 89, 7,
  219, 133, 103, 57, 186, 228, 6, 88,
  25, 71, 165, 251, 120, 38, 196, 154,
  101, 59, 217, 135, 4, 90, 184, 230,
  167, 249, 27, 69, 198, 152, 122, 36,
  248, 166, 68, 26, 153, 199, 37, 123,
  58, 100, 134, 216, 91, 5, 231, 185,
  140, 210, 48, 110, 237, 179, 81, 15,
  78, 16, 242, 172, 47, 113, 147, 205,
  17, 79, 173, 243, 112, 46, 204, 146,
  211, 141, 111, 49, 178, 236, 14, 80,
  175, 241, 19, 77, 206, 144, 114, 44,
  109, 51, 209, 143, 12, 82, 176, 238,
  50, 108, 142, 208, 83, 13, 239, 177,
  240, 174, 76, 18, 145, 207, 45, 115,
  202, 148, 118, 40, 171, 245, 23, 73,
  8, 86, 180, 234, 105, 55, 213, 139,
  87, 9, 235, 181, 54, 104, 138, 212,
  149, 203, 41, 119, 244, 170, 72, 22,
  233, 183, 85, 11, 136, 214, 52, 106,
  43, 117, 151, 201, 74, 20, 246, 168,
  116, 42, 200, 150, 21, 75, 169, 247,
  182, 232, 10, 84, 215, 137, 107, 53
};

// return CRC8 from buf
uint8_t crc8(const uint8_t* buf, uint8_t length) {
    uint8_t crc = 0xff;
    while (length) {
        crc = pgm_read_byte_near(_crc8_table + (*buf ^ crc));
        buf++;
        length--;
    }
    return crc;
}

// Send/Receive Fanatec Packet
void transfer_data(csw_out_t* out, csw_in_t* in, uint8_t length) {
  //uint8_t data[length]; // 32 bytes of data, 1 byte for CRC
  //uint8_t crc;

  // get CRC
  out->crc = crc8(out->raw, length-1);

  //memcpy(data, out, length);

  // append CRC to data packet
  //data[length-1] = crc;

  // Send packet
  digitalWrite(CS, LOW);
  SPI.beginTransaction(settingsA);
  for(int i=0; i<length; i++) {
    in->raw[i] = SPI.transfer(out->raw[i]);
  }
  SPI.endTransaction();
  digitalWrite(CS, HIGH);
  // Bit shifting

  for (int i = 0;  i < length - 1;  ++i) {
     in->raw[i] = (in->raw[i] << 1) | ((in->raw[i+1] >> 7) & 1);
  } 

}

/*
void transfer_data2(const uint8_t* out, uint8_t* in, uint8_t length) {
  uint8_t data[length]; // 32 bytes of data, 1 byte for CRC
  uint8_t crc;
  int i;

  // get CRC
  crc = crc8(out, length-1);

  memcpy(data, out, length);

  // append CRC to data packet
  data[length-1] = crc;

  // Send packet
  // basic version
  SPIFIFO.clear();
  // Warm up the FIFO with 3 write
  SPIFIFO.write(data[0], SPI_CONTINUE);
  SPIFIFO.write(data[1], SPI_CONTINUE);
  SPIFIFO.write(data[2], SPI_CONTINUE);
  SPIFIFO.read();
  for(i=3; i<length-1; i++) {
    SPIFIFO.write(data[i], SPI_CONTINUE);
    in[i-3] = SPIFIFO.read();
  }
  SPIFIFO.write(data[i]);
  in[i++] = SPIFIFO.read();
  in[i++] = SPIFIFO.read();
  in[i++] = SPIFIFO.read();
}
*/

void fsetup() {

  pinMode(CS, OUTPUT);
  digitalWrite(CS,HIGH); 
  SPI.begin();
  SPI.setClockDivider(0);


}