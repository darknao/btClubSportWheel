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

#include "fanatec.h"
#include "iWRAP.h"


/* WT12 */
#define WT12 Serial1
#define CTS 18

uint8_t iwrap_mode = IWRAP_MODE_MUX;

uint8_t hid_data[35];
int16_t bt_delay = 0;
uint16_t max_delay = 400; //40; // overhead if below 30

bool in_changed; // useless for now
bool bt_retry; // useless too

int iwrap_out(int len, unsigned char *data);
void hid_output(uint8_t link_id, uint16_t data_length, const uint8_t *data);
void my_iwrap_evt_ring(uint8_t link_id, const iwrap_address_t *address, uint16_t channel, const char *profile);
void my_iwrap_evt_hid_suspend(uint8_t link_id);
void my_iwrap_rsp_list_result(uint8_t link_id, const char *mode, uint16_t blocksize, uint32_t elapsed_time, uint16_t local_msc, uint16_t remote_msc, const iwrap_address_t *bd_addr, uint16_t channel, uint8_t direction, uint8_t powermode, uint8_t role, uint8_t crypt, uint16_t buffer, uint8_t eretx);
int my_iwrap_debug(const char *data);
void my_iwrap_evt_no_carrier(uint8_t link_id, uint16_t error_code, const char *message);

void bt_button(uint8_t button, bool val);
void bt_X(unsigned int val);
void bt_Y(unsigned int val);
inline void bt_hat(int val);
void bt_setWheel(unsigned int val);
/* END WT12 */


void setup();
void loop();
void idle();

csw_in_t wheel_in;
csw_out_t wheel_out;
bool bt_connected;

void setup() {
  fsetup();

  /* WT12 */
  pinMode (CTS, INPUT);
  WT12.begin(115200, SERIAL_8N1);
  WT12.attachRts(19);

  // callback
  iwrap_output = iwrap_out;
  iwrap_evt_hid_output = hid_output;
  iwrap_evt_ring = my_iwrap_evt_ring;
  iwrap_evt_hid_suspend = my_iwrap_evt_hid_suspend;
  iwrap_rsp_list_result = my_iwrap_rsp_list_result;
  iwrap_debug = my_iwrap_debug;
  iwrap_evt_no_carrier = my_iwrap_evt_no_carrier;

  // prebuild HID packet
  hid_data[0] = 0x9f;
  hid_data[1] = 0x21;
  hid_data[2] = 0xa1;

  in_changed = false; // useless
  bt_retry = false; // useless


  //Joystick.useManualSend(true);

  
  // prebuild output packet
  memset(wheel_out.raw, 0, sizeof(csw_out_t));
  wheel_out.header = 0xa5;
  wheel_out.id = 0x03;
  //wheel_out.leds = 0xffff;

  
  bt_connected = false;
  // debug
  Serial.begin(115200);
  Serial.println("MCU Ready");  
  delay(5000);
  Serial.println("check for active connection...");

  iwrap_send_command("LIST", iwrap_mode);
  //iwrap_send_command("SET BT PAIR", iwrap_mode);
}


byte rotary_debounce = 0;
int8_t rotary_value = 0;
uint8_t main_link_id = 1;

void loop() {
  
  if(bt_connected) {
    // Read Fanatec Packet
    //SPI.beginTransaction(settingsA);
    //transfer_data2(ftx_pck, frx_pck, sizeof(ftx_pck));
    transfer_data(&wheel_out, &wheel_in, sizeof(wheel_out.raw));
    /*
    digitalWrite(chipselect,LOW); 
    for (i = 0; i < frx_size; i++) {
      frx_pck[i] = SPI.transfer(0);
    }
    digitalWrite(chipselect,HIGH); 
    //SPI.endTransaction();
  */
  /*
    busy = (frx_pck[0] >> 7) & 0x01;
    // Bit shifting
    for (i = 0;  i < frx_size - 1;  ++i) {
       frx_pck[i] = (frx_pck[i] << 1) | ((frx_pck[i+1] >> 7) & 1);
    } 
    
    // nulling last byte (this is completly useless, but make reading easize)
    frx_pck[frx_size-1] = 0;
    */
    if(wheel_in.header == 0xa5){
      bt_setWheel(wheel_in.id);
      // Left stick
      bt_X(255-(wheel_in.axisX+127));
      bt_Y(wheel_in.axisY+127);
  
      // All buttons
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
  /*
      if( rotary_debounce > 0 && rotary_debounce <= 100 ){
       rotary_debounce++;
      } else {
    */
      if (rotary_debounce == 0) {    
        //rotary_debounce = 0;
        rotary_value = wheel_in.encoder;
        
        bt_button(17, rotary_value==-1);
        bt_button(18, rotary_value==1);
        if (rotary_value != 0){
          Serial.println(String("DBNCE START!!!!!!!!!ROTARY : ") + rotary_value);
          rotary_debounce++;
        }
      }

      int16_t hat;
      switch (wheel_in.buttons[0]&0x0f){
        case 0: hat=0xFF;break;
        case 1: hat=0;break;
        case 2: hat=6;break;
        case 4: hat=2;break;
        case 8: hat=4;break;
        default: hat=0xFF;
      }
      bt_hat(hat);
      
      //Serial.println(String("button: ") + hid_data[3]);

      
    }
  
    // Send HID report (all inputs)
    //Joystick.send_now();
      if(bt_delay > max_delay)
      {
        //hid_data[3] = (hid_data[3]+1)&0xff;
        iwrap_send_data(main_link_id, sizeof(hid_data), hid_data, iwrap_mode);
        
        in_changed = false;
        bt_delay = 0;
        if (rotary_debounce!=0)Serial.println(String("RESET!!!!!!!!!ROTARY : ") + rotary_value);
        rotary_debounce = 0;
      }
      bt_delay++;
  }

  // Read WT12 incoming data
  uint16_t result;
  while((result = WT12.read()) < 256) iwrap_parse(result & 0xFF, iwrap_mode);

  // some delay
  //idle();
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

void bt_setWheel(unsigned int val) {
  uint8_t old = hid_data[32];
  hid_data[32] = val & 0xFF;
  if (old != hid_data[32]) in_changed = true;   
}
        
int iwrap_out(int len, unsigned char *data) {
  // iWRAP output to module goes through software serial
  int nbytes = WT12.availableForWrite();

  if(digitalRead(CTS) == LOW && nbytes >= len){
      nbytes = WT12.write(data, len);
      bt_retry = false;
      //Serial.println(String("sent : ") + nbytes );
  } else {
      bt_retry = true;
      Serial.println(String("Throttling (")+max_delay+")");
      bt_delay = -32000;
      max_delay++;
      //delay(1000);
  }
  
  return nbytes;
}
    
void hid_output(uint8_t link_id, uint16_t data_length, const uint8_t *data) {
  Serial.println(String("HID OUTPUT FROM " )+ link_id + " (size:" + data_length + ")");
  
  for(int i=0; i<data_length; i++) {
    Serial.print(data[i], HEX);
    Serial.print(":");
    
  }
  Serial.println();

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

int my_iwrap_debug(const char *data) {
  return Serial.print(data);
}

void my_iwrap_evt_ring(uint8_t link_id, const iwrap_address_t *address, uint16_t channel, const char *profile) {
  Serial.println(String("Connection from " )+ link_id);
  bt_connected = true;
}

void my_iwrap_evt_hid_suspend(uint8_t link_id) {
  Serial.println(String("Disconnection from " )+ link_id);
  bt_connected = false;
}

void my_iwrap_evt_no_carrier(uint8_t link_id, uint16_t error_code, const char *message) {
  Serial.println(String("Disconnection from " )+ link_id);
  bt_connected = false;
}

void my_iwrap_rsp_list_result(uint8_t link_id, const char *mode, uint16_t blocksize, uint32_t elapsed_time, uint16_t local_msc, uint16_t remote_msc, const iwrap_address_t *bd_addr, uint16_t channel, uint8_t direction, uint8_t powermode, uint8_t role, uint8_t crypt, uint16_t buffer, uint8_t eretx) {
  Serial.println(String("Already connected to ") + link_id);
  bt_connected = true;
}

void idle() {
    delay(2);
}


