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



#include <SPI.h>

#include "iWRAP.h"

#define IWRAP_STATE_IDLE            0
#define IWRAP_STATE_UNKNOWN         1
#define IWRAP_STATE_PENDING_AT      2
#define IWRAP_STATE_PENDING_SET     3
#define IWRAP_STATE_PENDING_LIST    4
#define IWRAP_STATE_PENDING_CALL    5
#define IWRAP_STATE_COMM_FAILED     255

#define IWRAP_MAX_PAIRINGS          16

#define CS 10
#define WT12 Serial1
#define CTS 18

uint8_t iwrap_mode = IWRAP_MODE_MUX;
int iwrap_out(int len, unsigned char *data);
void hid_output(uint8_t link_id, uint16_t data_length, const uint8_t *data);
void my_iwrap_evt_ring(uint8_t link_id, const iwrap_address_t *address, uint16_t channel, const char *profile);
void my_iwrap_evt_hid_suspend(uint8_t link_id);
void my_iwrap_rsp_list_result(uint8_t link_id, const char *mode, uint16_t blocksize, uint32_t elapsed_time, uint16_t local_msc, uint16_t remote_msc, const iwrap_address_t *bd_addr, uint16_t channel, uint8_t direction, uint8_t powermode, uint8_t role, uint8_t crypt, uint16_t buffer, uint8_t eretx);
int my_iwrap_debug(const char *data);
void my_iwrap_evt_no_carrier(uint8_t link_id, uint16_t error_code, const char *message);

uint8_t hid_data[35];
bool in_changed;
bool bt_retry;
void setup();
void loop();
bool bt_connected;
int16_t bt_delay = 0;
uint16_t max_delay = 400; //40; // overhead if below 30
void idle();

uint8_t crc8(const uint8_t* buf, uint8_t length);


const byte chipselect = 10;
SPISettings settingsA(12000000, MSBFIRST, SPI_MODE0);

// Fanatec packet (output)
uint8_t ftx_pck[33];
uint8_t frx_pck[34];  
// HID packet (input)
uint8_t hid_pck[10];

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

void bt_button(uint8_t button, bool val);
void bt_X(unsigned int val);
void bt_Y(unsigned int val);
inline void bt_hat(int val);
void bt_setWheel(unsigned int val);
void transfer_data(csw_out_t* out, csw_in_t* in, uint8_t length);
csw_in_t wheel_in;
csw_out_t wheel_out;

void setup() {
  pinMode (CS, OUTPUT);
  digitalWrite(CS,HIGH); 
  pinMode (CTS, INPUT);

delay(5000);
  SPI.begin();
  SPI.setClockDivider(0);
  //SPIFIFO.begin(chipselect, SPI_CLOCK_24MHz);
  //Joystick.useManualSend(true);
  Serial.begin(115200);
  WT12.begin(115200, SERIAL_8N1);
  WT12.attachRts(19);
  
  // prebuild output packet
  memset(wheel_out.raw, 0, sizeof(csw_out_t));
  wheel_out.header = 0xa5;
  wheel_out.id = 0x03;
 //wheel_out.leds = 0xffff;

  iwrap_output = iwrap_out;
  iwrap_evt_hid_output = hid_output;
  iwrap_evt_ring = my_iwrap_evt_ring;
  iwrap_evt_hid_suspend = my_iwrap_evt_hid_suspend;
  iwrap_rsp_list_result = my_iwrap_rsp_list_result;
  iwrap_debug = my_iwrap_debug;
  iwrap_evt_no_carrier = my_iwrap_evt_no_carrier;
  hid_data[0] = 0x9f;
  hid_data[1] = 0x21;
  hid_data[2] = 0xa1;
  in_changed = false;
  bt_retry = false;
  
  bt_connected = false;
  Serial.println("MCU Ready");  
  Serial.println("check for active connection...");
  iwrap_send_command("LIST", iwrap_mode);
  //iwrap_send_command("SET BT PAIR", iwrap_mode);
}

int16_t hat = 0;
int frx_size = 34;
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
          switch (wheel_in.buttons[0]&0x0f){
        case 0: hat=0xFF;break;
        case 1: hat=0;break;
        case 2: hat=6;break;
        case 4: hat=2;break;
        case 8: hat=4;break; 
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
  
  // Extract the firt bit (i don't know its meaning yet)
  // busy = (frx_pck[0] >> 7) & 0x01;

  // Bit shifting
  for (int i = 0;  i < length - 1;  ++i) {
     in->raw[i] = (in->raw[i] << 1) | ((in->raw[i+1] >> 7) & 1);
  } 
}