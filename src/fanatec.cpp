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

// SPI setting to communicate with Fanatec PCB.
// Basically default setting, except speed is set to 12Mhz
SPISettings settingsA(12000000, MSBFIRST, SPI_MODE0);
SPISettings settingsB(12000000, MSBFIRST, SPI_MODE1);

// Conversion table for CSW 7segs to CSL
uint8_t csw2csl_disp[8] = {6, 4, 0, 2, 5, 7, 1, 3};

wheel_type rim_inserted = NO_WHEEL;
unsigned int CS_WAIT = 5;

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

// Try to detect which wheel is connected by reading the header bit
// transfer*Data also reset this state if the header bit is not the one expected
wheel_type detectWheelType() {
  if(rim_inserted == NO_WHEEL) {
    switch(getFirstByte()) {
      case 0x52:
      case 0xD2: rim_inserted = CSW_WHEEL; break;
      case 0xE0: rim_inserted = CSL_WHEEL; break;
      case 0xA5: rim_inserted = MCL_WHEEL; break;
      default: rim_inserted = NO_WHEEL; break;
    }
      #ifdef HAS_DEBUG
      Serial.println(String("Detected protocol: ") + rim_inserted);
      #endif
  }
  return rim_inserted;
}

// Fetch first byte from SPI transaction for protocol detection
uint8_t getFirstByte() {
  uint8_t firstByte;
  uint8_t buf;
  // Send packet, twice (see transferCslData)
  for (int z=0; z<2; z++) {
    SPI.beginTransaction(settingsA);
    digitalWrite(CS, LOW);
    delayMicroseconds(CS_WAIT);
    firstByte = SPI.transfer(0x00);
    /*
      The CSW µC will resume any previously interrupted transaction.
      firstByte will then not be the actual first byte, but something between.
      This loop make sure we reach the end of a transaction before starting a new one
      The CSL (P1) transaction size is only 1 byte, so it's not affected.
    */

    #ifdef HAS_DEBUG
      Serial.print("Firstbyte: ");
      Serial.println(firstByte, HEX);
    #endif
    if(firstByte  == 0x52){
      #ifdef HAS_DEBUG
        Serial.println("csw: fast forward to next transaction");
      #endif
      for(int i=0; i<=31; i++) {
        buf = SPI.transfer(0x00);
        #ifdef HAS_DEBUG
          Serial.print(buf, HEX);
          Serial.print(":");
        #endif
      }
      #ifdef HAS_DEBUG
        Serial.println();
      #endif
    } else if(firstByte == 0xD2){
      // 0x52 with extra byte from previous crc (wrong communication settings)
      // so we need to flush this extra byte befor going forward
      #ifdef HAS_DEBUG
        Serial.println("csw: flushing buffer & fast forward to next transaction");
      #endif
      for(int i=0; i<=(31*2); i++) {
        buf = SPI.transfer(0x00);
        #ifdef HAS_DEBUG
          Serial.print(buf, HEX);
          Serial.print(":");
        #endif
      }
      #ifdef HAS_DEBUG
        Serial.println();
      #endif

    } else if(firstByte != 0xE0 && firstByte != 0 ) {
      // looks like we are in the middle of a transaction
      #ifdef HAS_DEBUG
      Serial.println("Realigning...");
      #endif
      uint8_t previousByte = 0;
      uint8_t s;
      for(int i=0; i<35; i++) {
        s = SPI.transfer(0x00);
        #ifdef HAS_DEBUG
          Serial.print("searching: next byte: ");
          Serial.println(s, HEX);
        #endif
        if( (previousByte == 0xA5 && s == 0x09) || (previousByte == 0x52 && s == 0x84) ){
          // Here we go
          firstByte = previousByte;
          #ifdef HAS_DEBUG
          Serial.println("mcl: Fast Forward to next transaction");
          #endif
          for(int i=0; i<31; i++) {
            buf = SPI.transfer(0x00);
            #ifdef HAS_DEBUG
              Serial.print(buf, HEX);
              Serial.print(":");
            #endif
          }
          #ifdef HAS_DEBUG
            Serial.println();
          #endif
          break;
        } else {
          previousByte = s;
        }
      }
    }
    digitalWrite(CS, HIGH);
    SPI.endTransaction();

    // wait for CS to settle a little
    delayMicroseconds(10);
  }
  delay(10);
  return firstByte;
}

// CSW I/O
void transferCswData(csw_out_t* out, csw_in_t* in, uint8_t length) {
  // get CRC
  out->crc = crc8(out->raw, length-1);

  // Send/Receive packet
  SPI.beginTransaction(settingsA);
  digitalWrite(CS, LOW);
  //delayMicroseconds(20);
  for(int i=0; i<length; i++) {
    in->raw[i] = SPI.transfer(out->raw[i]);
  }
  digitalWrite(CS, HIGH);
  SPI.endTransaction();

  /*
    The CSW frame start with a 1 bit value.
    Its meaning is not certain, but it's more likely an error flag
    set if the previous received packet was malformed.
    This bit is discarded here, and everything is shifted
    to realligned the data correctly with the csw_in_t struct.
  */
  /*
  for (int i = 0;  i < length - 1;  ++i) {
     in->raw[i] = (in->raw[i] << 1) | ((in->raw[i+1] >> 7) & 1);
  }
  */
  if (in->header == 0xd2 || in->header == 0x52){
    // data still not alligned (?!)
    #ifdef HAS_DEBUG
    Serial.print("csw: data not alligned :");
    Serial.println(in->header, HEX);
    #endif
    for (int i = 0;  i < length - 1;  ++i) {
       in->raw[i] = (in->raw[i] << 1) | ((in->raw[i+1] >> 7) & 1);
    }
    in->raw[length - 1] = (in->raw[length - 1] << 1);
  }

  #ifdef HAS_DEBUG
  uint8_t crc = crc8(in->raw, length-1);
  if((crc&0xFE) != in->crc){
    Serial.print("Bad CRC: ");
    Serial.print(in->crc, HEX);
    Serial.print(" != ");
    Serial.println(crc, HEX);
  }
  #endif

  if ((in->header & 0xFE) != 0xa4) rim_inserted = NO_WHEEL;
  //if ((in->header == 0x00) || (in->header == 0xFF)) rim_inserted = NO_WHEEL;
}

// CSL I/O
void transferCslData(csl_out_t* out, csl_in_t* in, uint8_t length, uint8_t selector) {
  out->selector = selector;

  /*
    The CSL output is based on which selector we send.
    Since we received and send simultaneously,
    only the second packet is relevant.
  */
  for (int z=0; z<2; z++) {
    SPI.beginTransaction(settingsA);
    digitalWrite(CS, LOW);
    delayMicroseconds(CS_WAIT);
    for(int i=0; i<length; i++) {
      in->raw[i] = SPI.transfer(out->raw[i]);
    }
    digitalWrite(CS, HIGH);
    SPI.endTransaction();
  }
  if (out->selector == 0x00 && in->raw[0] != 0xE0) rim_inserted = NO_WHEEL;
}

void transferMclData(mcl_out_t* out, mcl_in_t* in, uint8_t length) {
  // get CRC
  out->crc = crc8(out->raw, length-1);

  // Send/Receive packet
  SPI.beginTransaction(settingsA);
  digitalWrite(CS, LOW);
  for(int i=0; i<length; i++) {
    in->raw[i] = SPI.transfer(out->raw[i]);
  }
  digitalWrite(CS, HIGH);
  SPI.endTransaction();

  #ifdef HAS_DEBUG
  uint8_t crc = crc8(in->raw, length-1);
  if(crc != in->crc){
    Serial.print("Bad CRC: ");
    Serial.print(in->crc, HEX);
    Serial.print(" != ");
    Serial.println(crc, HEX);
  }
  #endif
  if (in->header != 0xA5) rim_inserted = NO_WHEEL;
  //if ((in->header == 0x00) || (in->header == 0xFF)) rim_inserted = NO_WHEEL;

}

// Convert the CSW 7seg bits to CSL
uint8_t csw7segToCsl(uint8_t csw_disp) {
  uint8_t csl_disp = 0x00;

  for (uint8_t x=0; x < sizeof(csw2csl_disp); x++) {
    csl_disp |=  ((~csw_disp >> x) & 0x1) << csw2csl_disp[x];
  }
  return csl_disp;
}

// Convert the CSW revlights bits to CSL RGB led
uint8_t cswLedsToCsl(uint16_t csw_leds) {
  uint8_t csl_leds = 0xFF;

  // First 2 leds -> green
  if (csw_leds & 0x180)
    csl_leds = ~0x10;
  // 3-4 -> yellow
  if (csw_leds & 0x60)
    csl_leds = ~0x50;
  // 5-6 -> red
  if (csw_leds & 0x18)
    csl_leds = ~0x40;
  // 7-8 -> blue
  if (csw_leds & 0x6)
    csl_leds = ~0x01;
  // 9 -> white
  if (csw_leds & 0x1)
    csl_leds = ~0x51;

  return csl_leds;
}

uint8_t csw7segToAscii(uint8_t csw_disp) {
  uint8_t ascii;

  switch(csw_disp) {
    //case 0x40: ascii = 0x5B; break; // -
    case 0x39: ascii = 0x28; break; // [
    case 0x0F: ascii = 0x29; break; // ]
    case 0x3F: ascii = 0x00; break; // 0
    case 0x06: ascii = 0x01; break; // 1
    case 0x5B: ascii = 0x02; break; // 2
    case 0x4F: ascii = 0x03; break; // 3
    case 0x66: ascii = 0x04; break; // 4
    case 0x6D: ascii = 0x05; break; // 5
    case 0x7D: ascii = 0x06; break; // 6
    case 0x07: ascii = 0x07; break; // 6
    case 0x7F: ascii = 0x08; break; // 8
    case 0x6F: ascii = 0x09; break; // 9

    case 0x54: ascii = 0x4E; break; // N
    case 0x50: ascii = 0x52; break; // R

    default:
      ascii = 0x0A;
  }

  return ascii;
}


void fsetup() {

  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  SPI.begin();
  SPI.setClockDivider(0);
}