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
#include "iWRAP.h"

#ifndef JOYSTICK_INTERFACE

//#include "zbluetooth.h"
#define IWRAP_STATE_IDLE            0
#define IWRAP_STATE_UNKNOWN         1
#define IWRAP_STATE_PENDING_AT      2
#define IWRAP_STATE_PENDING_SET     3
#define IWRAP_STATE_PENDING_LIST    4
#define IWRAP_STATE_PENDING_CALL    5
#define IWRAP_STATE_COMM_FAILED     255

#define IWRAP_MAX_PAIRINGS          16

#define WT12 Serial1
#define CTS 18

uint8_t iwrap_mode;
bool in_changed;
int8_t main_link_id;
bool bt_retry;

int iwrap_out(int len, unsigned char *data);
void hid_output(uint8_t link_id, uint16_t data_length, const uint8_t *data);
void my_iwrap_evt_ring(uint8_t link_id, const iwrap_address_t *address, uint16_t channel, const char *profile);
void my_iwrap_evt_hid_suspend(uint8_t link_id);
void bt_send_hid();
void bt_button(uint8_t button, bool val);
void bt_X(unsigned int val);
void bt_Y(unsigned int val);
inline void bt_hat(int val);

#endif // JOYSTICK_INTERFACE

uint8_t hid_data[35];

// Fanatec packet (teensy <--> wheel)
csw_in_t wheel_in;
csw_out_t wheel_out;
//uint8_t ftx_pck[33];
//uint8_t frx_pck[FRXSIZE];


void setup() {
  #ifndef JOYSTICK_INTERFACE
  WT12.begin(115200, SERIAL_8N1);
  WT12.attachRts(19); // 6 or 19
  pinMode (CTS, INPUT);

  iwrap_output = iwrap_out;
  iwrap_evt_hid_output = hid_output;
  iwrap_evt_ring = my_iwrap_evt_ring;
  iwrap_evt_hid_suspend = my_iwrap_evt_hid_suspend;
  in_changed = false;
  main_link_id = 1;
  iwrap_mode = IWRAP_MODE_MUX;
  bt_retry = false;
  #else
  Joystick.useManualSend(true);
  
  #endif // JOYSTICK_INTERFACE

  pinMode (CS, OUTPUT);
  digitalWrite(CS, HIGH); 
  
  SPI.begin();
  SPI.setClockDivider(0);

  // prebuild output packet
  memset(wheel_out.raw, 0, sizeof(csw_out_t));
  wheel_out.header = 0xa5;
  wheel_out.id = 0x03;

  hid_data[0] = 0x9f;
  hid_data[1] = 0x21;
  hid_data[2] = 0xa1;
  hid_data[32] = 0x03;
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
  #ifdef JOYSTICK_INTERFACE
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
  #endif //JOYSTICK_INTERFACE

  if(wheel_in.header == 0xa5){

    #ifdef JOYSTICK_INTERFACE
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
    #else // JOYSTICK_INTERFACE

        // Left stick
    bt_X(255-(wheel_in.axisX+127));
    bt_Y(wheel_in.axisY+127);

    bt_button(1, wheel_in.buttons[0]&0x80);
    bt_button(2, wheel_in.buttons[0]&0x40);
    bt_button(3, wheel_in.buttons[0]&0x20);
    bt_button(4, wheel_in.buttons[0]&0x10);
    bt_button(5, wheel_in.buttons[1]&0x80);
    bt_button(6, wheel_in.buttons[1]&0x40);
    bt_button(7, wheel_in.buttons[1]&0x20);
    bt_button(8, wheel_in.buttons[1]&0x10);
    bt_button(9, wheel_in.buttons[1]&0x04);
    bt_button(10, wheel_in.buttons[1]&0x02);
    bt_button(11, wheel_in.buttons[2]&0x08);
    bt_button(12, wheel_in.buttons[2]&0x04);
    bt_button(13, wheel_in.buttons[2]&0x02);
    bt_button(14, wheel_in.buttons[2]&0x20);
    // paddles shitfer 
    bt_button(15, wheel_in.buttons[1]&0x08);
    bt_button(16, wheel_in.buttons[1]&0x01);

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
    bt_hat(hat);
    #endif // JOYSTICK_INTERFACE
  }
  
  // Send HID report (all inputs)
  
  #ifdef JOYSTICK_INTERFACE
  Joystick.send_now();
  #else

      if(main_link_id > 0   )
    {
      //hid_data[3] = (hid_data[3]+1)&0xff;

      iwrap_send_data(main_link_id, sizeof(hid_data), hid_data, iwrap_mode);
      //Serial.println("hid sent");
      in_changed = false;
      //delay(20);
      //bt_timer = 500;
    }
  #endif // JOYSTICK_INTERFACE
    //bt_timer--;
  // some delay
  idle();
}



void idle() {
  // or not
  // delay(1);
}

#ifndef JOYSTICK_INTERFACE
int iwrap_out(int len, unsigned char *data) {
    // iWRAP output to module goes through software serial
    int nbytes = 0;

    if(digitalRead(CTS) == LOW && WT12.availableForWrite() >= len){
      nbytes = WT12.write(data, len);
      bt_retry = false;
    } else {
      bt_retry = true;
    }
    delay(100);
    return nbytes;

}
    
void hid_output(uint8_t link_id, uint16_t data_length, const uint8_t *data) {
  /*
  Serial.println(String("HID OUTPUT FROM " )+ link_id + " (size:" + data_length + ")");
  
  for(int i=0; i<data_length; i++) {
    Serial.print(data[i], HEX);
    Serial.print(":");
    
  }
  Serial.println();
  */
      if(data[2] == 0x01 && data[3] == 0x02){
          // 7 seg
          wheel_out.disp[0] = (data[4] & 0xff);
          wheel_out.disp[1] = (data[5] & 0xff);
          wheel_out.disp[2] = (data[6] & 0xff);
      }
      if(data[2] == 0x01 && data[3] == 0x03){
          // rumbles
          wheel_out.rumble[0] = (data[4] & 0xff);
          wheel_out.rumble[1] = (data[5] & 0xff);
      }
      if(data[2] == 0x08){
          // Rev Lights
          wheel_out.leds = (data[3] & 0xff) << 8 | (data[4] & 0xff);
          //ftx_pck[5] = (hid_pck[4] & 0xff);
          //ftx_pck[6] = (hid_pck[3] & 0xff);
      }

}

void my_iwrap_evt_ring(uint8_t link_id, const iwrap_address_t *address, uint16_t channel, const char *profile) {
    //Serial.println(String("Connection from " )+ link_id);
    if(link_id == 0) link_id++;
  main_link_id = link_id;
}

void my_iwrap_evt_hid_suspend(uint8_t link_id) {
  //Serial.println(String("Disconnection from " )+ link_id);
  main_link_id = -1;
  
}

void bt_send_hid() {
    //Serial.println(String("button: ") + hid_data[3]);
    if(main_link_id > 0 && in_changed)
    {

      //hid_data[3] = (hid_data[3]+1)&0xff;
      iwrap_send_data(main_link_id, sizeof(hid_data), hid_data, iwrap_mode);
      //Serial.println("hid sent");
      in_changed = false;
    }

}

void bt_button(uint8_t button, bool val) {
          uint8_t old;
            if (--button >= 18) return;
            if (button >= 16) {
               old = hid_data[5];
                if (val) hid_data[5] |= (0x1 << (button-16));
                else hid_data[5] &= ~(0x1 << (button-16));     
                if (old != hid_data[5]) in_changed = true;
            } else if (button >= 8) {
              old = hid_data[4];
                if (val) hid_data[4] |= (0x1 << (button-8));
                else hid_data[4] &= ~(0x1 << (button-8));     
                if (old != hid_data[4]) in_changed = true;     
            } else {
              old = hid_data[3];
                if (val) hid_data[3] |= (0x1 << (button));
                else hid_data[3] &= ~(0x1 << (button));         
                if (old != hid_data[3]) in_changed = true;   
            }
}

        void bt_X(unsigned int val) {
            uint8_t old = hid_data[6];
            hid_data[6] = val & 0xFF;
            if (old != hid_data[6]) in_changed = true;    
        }

        void bt_Y(unsigned int val) {
          uint8_t old = hid_data[7];
            hid_data[7] = val & 0xFF;
            if (old != hid_data[7]) in_changed = true;   

        }
        inline void bt_hat(int val) {
          uint8_t old = hid_data[8];
            hid_data[8] = val & 0xFF;
            if (old != hid_data[8]) in_changed = true;   
        }

#endif