#line 1 "csw.cpp"
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

#include "WProgram.h"
#include "Arduino.h"
#include "csw.h"

// Fanatec packet (teensy <--> wheel)
csw_in_t wheel_in;
csw_out_t wheel_out;
//uint8_t ftx_pck[33];
//uint8_t frx_pck[FRXSIZE];

void setup() {
  pinMode (CS, OUTPUT);
  digitalWrite(CS, HIGH); 

  SPI.begin();
  SPI.setClockDivider(0);
  //SPIFIFO.begin(CS, SPI_CLOCK_24MHz);
  Joystick.useManualSend(true);

  // prebuild output packet
  memset(wheel_out.raw, 0, sizeof(csw_out_t));
  wheel_out.header = 0xa5;
  wheel_out.id = 0x03;
}

void loop() {
  
  // bool busy;
  //int i;

  // Read / Write Fanatec packet
  //transfer_data(wheel_out.raw, wheel_in.raw, sizeof(wheel_out));
  transfer_data(&wheel_out, &wheel_in, sizeof(wheel_out.raw));
  // Extract the firt bit (i don't know its meaning yet)
  // busy = (frx_pck[0] >> 7) & 0x01;

  // Bit shifting
  /*
  for (i = 0;  i < FRXSIZE - 1;  ++i) {
     frx_pck[i] = (frx_pck[i] << 1) | ((frx_pck[i+1] >> 7) & 1);
  } 
  */
  // A5 3 0 0 0 0 0 0 40 83 87 43 5B 8A 39 A0 C9 B5 D8 1D 72 20 24 FA 62 EF 93 44 CB 98 D0 12 D0 0
  // A5 A5 3 0 0 0 0 0 40 83 87 43 5B 8A 39 A0 C9 B5 D8 1D 72 20 24 FA 62 EF 93 44 CB 98 D0 12 D0 0
  // nulling last byte (this is completly useless, but make reading easier)
  //frx_pck[FRXSIZE-1] = 0;

/*
HID packet coming from fanaleds:

rumble init
f8 09 01 03 c8 c8 01 00 00 00
          | L  R |

seg init
f8 09 01 02 00 00 71 00 00 00


Led init
f8 09 00 04 00 c8 01 00 00 00

Fanatec Packet:
headers
 ^  wheel ID?
 |  ^  display
 |  |    ^    revlights
 |  |    |        ^  rumbles
 |  |    |        |     ^                              nothing                                  CRC8
|__|__|________|_____|_____|____________________________________________________________________|__|
 a5 03 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 XX
 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32
*/

  // Fetching HID packet, if any
  if (Joystick.recv(&hid_pck, 0) > 0) {
      if(hid_pck[2] == 0x01 && hid_pck[3] == 0x02){
          // 7 seg
          wheel_out.disp[0] = (hid_pck[4] & 0xff);
          wheel_out.disp[1] = (hid_pck[5] & 0xff);
          wheel_out.disp[2] = (hid_pck[6] & 0xff);
      }
      if(hid_pck[2] == 0x01 && hid_pck[3] == 0x03){
          // rumbles
          wheel_out.rumble[0] = (hid_pck[4] & 0xff);
          wheel_out.rumble[1] = (hid_pck[5] & 0xff);
      }
      if(hid_pck[2] == 0x08){
          // Rev Lights
	  wheel_out.leds = (hid_pck[3] & 0xff) << 8 | (hid_pck[4] & 0xff);
          //ftx_pck[5] = (hid_pck[4] & 0xff);
          //ftx_pck[6] = (hid_pck[3] & 0xff);
      }
  } 

  if(wheel_in.header == 0xa5){

    // Wheel model
    // 1: BMW GT2
    // 2: Formula
    // 3: Porsche GT
    // 4: Universal Hub
    Joystick.setWheel(wheel_in.id);

    // Left stick
    Joystick.X(255-(wheel_in.axisX+127));
    Joystick.Y(wheel_in.axisY+127);

    // All buttons
    Joystick.button(1, wheel_in.buttons[0]&0x80);
    Joystick.button(2, wheel_in.buttons[0]&0x40);
    Joystick.button(3, wheel_in.buttons[0]&0x20);
    Joystick.button(4, wheel_in.buttons[0]&0x10);
    Joystick.button(5, wheel_in.buttons[1]&0x80);
    Joystick.button(6, wheel_in.buttons[1]&0x40);
    Joystick.button(7, wheel_in.buttons[1]&0x20);
    Joystick.button(8, wheel_in.buttons[1]&0x10);
    Joystick.button(9, wheel_in.buttons[1]&0x04);
    Joystick.button(10, wheel_in.buttons[1]&0x02);
    Joystick.button(11, wheel_in.buttons[2]&0x08);
    Joystick.button(12, wheel_in.buttons[2]&0x04);
    Joystick.button(13, wheel_in.buttons[2]&0x02);
    Joystick.button(14, wheel_in.buttons[2]&0x20);
    // paddles shitfer 
    Joystick.button(15, wheel_in.buttons[1]&0x08);
    Joystick.button(16, wheel_in.buttons[1]&0x01);


    // Rotary Encoder (right stick)
    // Each tick last only one frame
    // I use the debounce timer to make it last 20 frames
    // to make sure it register
    if( rotary_debounce > 0 && rotary_debounce <= 20 ){
       rotary_debounce++;
    } else {
      rotary_debounce = 0;
      rotary_value = wheel_in.encoder;
      Joystick.button(17, rotary_value==-1);
      Joystick.button(18, rotary_value==1);
      if (rotary_value != 0){
        rotary_debounce++;
      }
    }

    // Rocker switch (or hat; right stick)
    // anti-clockwise
    // 0: N 2: W 4: S 6: E
    // 1,3,5 and 7 are NW SW SE NE
    switch (wheel_in.buttons[0]&0x0f){
      case 0: hat=0xFF;break;
      case 1: hat=0;break;
      case 2: hat=6;break;
      case 4: hat=2;break;
      case 8: hat=4;break; 
    }
    Joystick.hat(hat);
  }

  // Send HID report (all inputs)
  Joystick.send_now();
  
  // some delay
  idle();
}



void idle() {
  // or not
  // delay(1);
}

