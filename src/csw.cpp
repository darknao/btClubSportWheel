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
#include "Debouncer.h"

/* WT12 (Bluetooth specifics) */
#define WT12 Serial1
#define CTS 18

#define MAX_SPEED   5000

uint8_t iwrap_mode = IWRAP_MODE_MUX;

uint8_t hid_data[35];
uint32_t max_delay = 150000; // overhead if below 120ms
// 400 is too short with fanaleds

bool in_changed;

// Bluetooth events
int iwrap_out(int len, unsigned char *data);
void my_iwrap_evt_ring(uint8_t link_id, const iwrap_address_t *address, uint16_t channel, const char *profile);
void my_iwrap_evt_hid_suspend(uint8_t link_id);
void my_iwrap_rsp_list_result(uint8_t link_id, const char *mode, uint16_t blocksize, uint32_t elapsed_time, uint16_t local_msc, uint16_t remote_msc, const iwrap_address_t *bd_addr, uint16_t channel, uint8_t direction, uint8_t powermode, uint8_t role, uint8_t crypt, uint16_t buffer, uint8_t eretx);
int my_iwrap_debug(const char *data);
void my_iwrap_evt_no_carrier(uint8_t link_id, uint16_t error_code, const char *message);
void hid_output(uint8_t link_id, uint16_t data_length, const uint8_t *data);
/* END WT12 */

void setup();
void loop();
void idle();
void init_wheel();

/* Wheel inputs */
void whClear();
void whButton(uint8_t button, bool val);
void whStick(unsigned int x, unsigned int y);
void whDoubleAxis(unsigned int x, unsigned int y);
void whDoubleClutch(unsigned int x, unsigned int y);
void whHat(int8_t val, bool is_csl);
void whSetId(unsigned int val);

csw_in_t csw_in;
csw_out_t csw_out;

csl_in_t csl_in;
csl_out_t csl_out;

mcl_in_t mcl_in;
mcl_out_t mcl_out;

bool bt_connected;
bool got_hid;
bool show_fwvers;
uint32_t timing;
uint32_t timing_bt;
uint32_t disp_timout;
uint32_t usb_time;

uint8_t hid_pck[7];

Debouncer * btDebncer = new Debouncer[89];
Debouncer hatDebncer = Debouncer();

byte rotary_debounce = 0;
int8_t rotary_value = 0;
uint8_t main_link_id = 1;

uint8_t clutch_max = 0xFF;


void setup() {
  fsetup();

  /*
    8 Extra Buttons
    can be connected
    from pin 2 to pin 9
  */
  for (int pin = 2; pin <= 9; ++pin)
  {
    pinMode(pin, INPUT_PULLUP);
  }

  // debounce timer for hat switch
  hatDebncer.interval(50);
  // debounce timer for rotary encoder
  btDebncer[17].interval(30);
  btDebncer[18].interval(30);


  /* WT12 */
  #ifndef IS_USB
    pinMode (CTS, INPUT);
    WT12.begin(115200, SERIAL_8N1);
    WT12.attachRts(19);

    // callback
    iwrap_output = iwrap_out;
    iwrap_evt_hid_output = hid_output;
    iwrap_evt_ring = my_iwrap_evt_ring;
    iwrap_evt_hid_suspend = my_iwrap_evt_hid_suspend;
    iwrap_rsp_list_result = my_iwrap_rsp_list_result;
    iwrap_evt_no_carrier = my_iwrap_evt_no_carrier;

    // prebuild HID packet
    hid_data[0] = 0x9f;
    hid_data[1] = 0x21;
    hid_data[2] = 0xa1;

    in_changed = false;

  #else
    Joystick.useManualSend(true);
  #endif // IS_USB
  whClear();

  // prebuild output packet
  memset(csw_out.raw, 0, sizeof(csw_out_t));
  memset(csl_out.raw, 0, sizeof(csl_out_t));
  memset(mcl_out.raw, 0, sizeof(mcl_out_t));
  csw_out.header = 0xa5;
  mcl_out.header = 0xa5;
  csw_out.id = 0x00;
  got_hid = false;

  // debug
  #ifdef HAS_DEBUG
    // iwrap_debug = my_iwrap_debug;
    Serial.begin(115200);
    Serial.println("MCU Ready");
    for (int i = 0; i < 100; ++i)
    {
      delay(100);
      Serial.print(".");
      /* code */
    }

    Serial.println("check for active connection...");
  #endif

  #ifndef IS_USB
    bt_connected = false;
    iwrap_send_command("LIST", iwrap_mode);
  #else
    bt_connected = true;
  #endif
  // iwrap_send_command("SET BT PAIR", iwrap_mode);
  timing = micros();
  timing_bt = millis();
  usb_time = micros();
}



void loop() {

  if(bt_connected) {
    #ifdef IS_USB
      // Fetching HID packet
      uint16_t hid_size;
      hid_size = Joystick.recv(&hid_pck, 0);
      if(hid_size > 0) hid_output(1, hid_size, hid_pck);
    #endif


    switch(detectWheelType()) {
      case CSW_WHEEL:
        // csw stuff
        // Read Fanatec Packet

        //csw_out.raw[9] = 0x0F; // xbox light
        transferCswData(&csw_out, &csw_in, sizeof(csw_out.raw));
        init_wheel();

        #ifdef HAS_DEBUG
           Serial.print("CSW_IN:");
           for(int i=0; i<sizeof(csw_in.raw); i++) {

              Serial.print(csw_in.raw[i], HEX);
              Serial.print(":");
            }
            Serial.println();
        #endif

        // Wheel ID
        whSetId(csw_in.id);

        // Left stick
        whStick(csw_in.axisX, csw_in.axisY);

        // All buttons
        whButton(1, csw_in.buttons[0] & 0x80); // first top right
        whButton(2, csw_in.buttons[0] & 0x40); // middle right
        whButton(3, csw_in.buttons[0] & 0x20); // second top right
        whButton(4, csw_in.buttons[0] & 0x10); // bottom right
        whButton(5, csw_in.buttons[1] & 0x80); // third center
        whButton(6, csw_in.buttons[1] & 0x40); // first center
        whButton(7, csw_in.buttons[1] & 0x20); // middle left
        whButton(9, csw_in.buttons[1] & 0x04); // bottom left
        whButton(11, csw_in.buttons[2] & 0x08); // second center
        whButton(12, csw_in.buttons[2] & 0x04); // stick button

        // paddles shitfer
        whButton(15, csw_in.buttons[1] & 0x08); // left
        whButton(16, csw_in.buttons[1] & 0x01); // right

        rotary_value = csw_in.encoder;

        if(csw_in.id != CSLMCLGT3){
          whButton(8, csw_in.buttons[1] & 0x10); // first top left
          whButton(10, csw_in.buttons[1] & 0x02); // second top left
          whButton(13, csw_in.buttons[2] & 0x02); // hat button
          whButton(14, csw_in.buttons[2] & 0x20); // display button



          whButton(17, rotary_value == -1); // left
          whButton(18, rotary_value == 1); // right

        }


        if(csw_in.id == UNIHUB || csw_in.id == XBOXHUB){
          // Uni Hub extra buttons
          // BUT_5 array (optional 3 buttons)
          whButton(19, csw_in.btnHub[0] & 0x08);
          whButton(20, csw_in.btnHub[0] & 0x10);
          whButton(21, csw_in.btnHub[0] & 0x20);

          // Playstation buttons
          whButton(22, csw_in.btnPS[0] & 0x01);
          whButton(23, csw_in.btnPS[0] & 0x02);
          whButton(24, csw_in.btnPS[0] & 0x04);
          whButton(25, csw_in.btnPS[0] & 0x08);
          whButton(26, csw_in.btnPS[0] & 0x10);
          whButton(27, csw_in.btnPS[0] & 0x20);
          whButton(28, csw_in.btnPS[0] & 0x40);
          whButton(29, csw_in.btnPS[0] & 0x80);

          whButton(30, csw_in.btnPS[1] & 0x01);
          whButton(31, csw_in.btnPS[1] & 0x02);
          whButton(32, csw_in.btnPS[1] & 0x04);
          whButton(33, csw_in.btnPS[1] & 0x08);
          whButton(34, csw_in.btnPS[1] & 0x10);
          whButton(35, csw_in.btnPS[1] & 0x20);
          whButton(36, csw_in.btnPS[1] & 0x40);
          whButton(37, csw_in.btnPS[1] & 0x80);
        }

        if(csw_in.id == XBOXHUB){
          // Xbox Hub has 1 extra button
          whButton(38, csw_in.btnHub[1] & 0x08);
        }


        whHat(csw_in.buttons[0] & 0x0f, false);

        // Serial.println(String("button: ") + hid_data[3]);


        if(csw_in.id == CSLMCLGT3){
        whButton(8, (csw_in.buttons[1] & 0x10) && ((csw_in.garbage[3] & 0xF0) == 0x10)); // switch left up
        whButton(19, (csw_in.buttons[2] & 0x80) && ((csw_in.garbage[3] & 0xF0) == 0x10)); // switch left down
        whButton(10, (csw_in.buttons[1] & 0x02) && ((csw_in.garbage[3] & 0xF0) == 0x10)); // switch right up
        whButton(20, (csw_in.buttons[2] & 0x40) && ((csw_in.garbage[3] & 0xF0) == 0x10)); // switch right down

        whButton(33, (csw_in.buttons[1] & 0x10) && ((csw_in.garbage[3] & 0xF0) == 0x20)); // switch left up
        whButton(34, (csw_in.buttons[2] & 0x80) && ((csw_in.garbage[3] & 0xF0) == 0x20)); // switch left down
        whButton(35, (csw_in.buttons[1] & 0x02) && ((csw_in.garbage[3] & 0xF0) == 0x20)); // switch right up
        whButton(36, (csw_in.buttons[2] & 0x40) && ((csw_in.garbage[3] & 0xF0) == 0x20)); // switch right down

        whButton(37, (csw_in.buttons[1] & 0x10) && ((csw_in.garbage[3] & 0xF0) == 0x30)); // switch left up
        whButton(38, (csw_in.buttons[2] & 0x80) && ((csw_in.garbage[3] & 0xF0) == 0x30)); // switch left down
        whButton(39, (csw_in.buttons[1] & 0x02) && ((csw_in.garbage[3] & 0xF0) == 0x30)); // switch right up
        whButton(40, (csw_in.buttons[2] & 0x40) && ((csw_in.garbage[3] & 0xF0) == 0x30)); // switch right down

        whButton(41, (csw_in.buttons[1] & 0x10) && ((csw_in.garbage[3] & 0xF0) == 0x40)); // switch left up
        whButton(42, (csw_in.buttons[2] & 0x80) && ((csw_in.garbage[3] & 0xF0) == 0x40)); // switch left down
        whButton(43, (csw_in.buttons[1] & 0x02) && ((csw_in.garbage[3] & 0xF0) == 0x40)); // switch right up
        whButton(44, (csw_in.buttons[2] & 0x40) && ((csw_in.garbage[3] & 0xF0) == 0x40)); // switch right down

        whButton(45, (csw_in.buttons[1] & 0x10) && ((csw_in.garbage[3] & 0xF0) == 0x50)); // switch left up
        whButton(46, (csw_in.buttons[2] & 0x80) && ((csw_in.garbage[3] & 0xF0) == 0x50)); // switch left down
        whButton(47, (csw_in.buttons[1] & 0x02) && ((csw_in.garbage[3] & 0xF0) == 0x50)); // switch right up
        whButton(48, (csw_in.buttons[2] & 0x40) && ((csw_in.garbage[3] & 0xF0) == 0x50)); // switch right down

        whButton(49, (csw_in.buttons[1] & 0x10) && ((csw_in.garbage[3] & 0xF0) == 0x60)); // switch left up
        whButton(50, (csw_in.buttons[2] & 0x80) && ((csw_in.garbage[3] & 0xF0) == 0x60)); // switch left down
        whButton(51, (csw_in.buttons[1] & 0x02) && ((csw_in.garbage[3] & 0xF0) == 0x60)); // switch right up
        whButton(52, (csw_in.buttons[2] & 0x40) && ((csw_in.garbage[3] & 0xF0) == 0x60)); // switch right down

        whButton(53, (csw_in.buttons[1] & 0x10) && ((csw_in.garbage[3] & 0xF0) == 0x70)); // switch left up
        whButton(54, (csw_in.buttons[2] & 0x80) && ((csw_in.garbage[3] & 0xF0) == 0x70)); // switch left down
        whButton(55, (csw_in.buttons[1] & 0x02) && ((csw_in.garbage[3] & 0xF0) == 0x70)); // switch right up
        whButton(56, (csw_in.buttons[2] & 0x40) && ((csw_in.garbage[3] & 0xF0) == 0x70)); // switch right down

        whButton(57, (csw_in.buttons[1] & 0x10) && ((csw_in.garbage[3] & 0xF0) == 0x80)); // switch left up
        whButton(58, (csw_in.buttons[2] & 0x80) && ((csw_in.garbage[3] & 0xF0) == 0x80)); // switch left down
        whButton(59, (csw_in.buttons[1] & 0x02) && ((csw_in.garbage[3] & 0xF0) == 0x80)); // switch right up
        whButton(60, (csw_in.buttons[2] & 0x40) && ((csw_in.garbage[3] & 0xF0) == 0x80)); // switch right down

        whButton(61, (csw_in.buttons[1] & 0x10) && ((csw_in.garbage[3] & 0xF0) == 0x90)); // switch left up
        whButton(62, (csw_in.buttons[2] & 0x80) && ((csw_in.garbage[3] & 0xF0) == 0x90)); // switch left down
        whButton(63, (csw_in.buttons[1] & 0x02) && ((csw_in.garbage[3] & 0xF0) == 0x90)); // switch right up
        whButton(64, (csw_in.buttons[2] & 0x40) && ((csw_in.garbage[3] & 0xF0) == 0x90)); // switch right down

        whButton(65, (csw_in.buttons[1] & 0x10) && ((csw_in.garbage[3] & 0xF0) == 0xA0)); // switch left up
        whButton(66, (csw_in.buttons[2] & 0x80) && ((csw_in.garbage[3] & 0xF0) == 0xA0)); // switch left down
        whButton(67, (csw_in.buttons[1] & 0x02) && ((csw_in.garbage[3] & 0xF0) == 0xA0)); // switch right up
        whButton(68, (csw_in.buttons[2] & 0x40) && ((csw_in.garbage[3] & 0xF0) == 0xA0)); // switch right down

        whButton(69, (csw_in.buttons[1] & 0x10) && ((csw_in.garbage[3] & 0xF0) == 0xB0)); // switch left up
        whButton(70, (csw_in.buttons[2] & 0x80) && ((csw_in.garbage[3] & 0xF0) == 0xB0)); // switch left down
        whButton(71, (csw_in.buttons[1] & 0x02) && ((csw_in.garbage[3] & 0xF0) == 0xB0)); // switch right up
        whButton(72, (csw_in.buttons[2] & 0x40) && ((csw_in.garbage[3] & 0xF0) == 0xB0)); // switch right down

        whButton(73, (csw_in.buttons[1] & 0x10) && ((csw_in.garbage[3] & 0xF0) == 0xC0)); // switch left up
        whButton(74, (csw_in.buttons[2] & 0x80) && ((csw_in.garbage[3] & 0xF0) == 0xC0)); // switch left down
        whButton(75, (csw_in.buttons[1] & 0x02) && ((csw_in.garbage[3] & 0xF0) == 0xC0)); // switch right up
        whButton(76, (csw_in.buttons[2] & 0x40) && ((csw_in.garbage[3] & 0xF0) == 0xC0)); // switch right down

        whButton(14, csw_in.buttons[2] & 0x02); // xbox

        if ((csw_in.garbage[2] & 0x0F) == 0x02 && !csw_in.axisX) // left clutch fully pressed
        {
          clutch_max = constrain(clutch_max + rotary_value, 0, 0xFF);
        } else {
          whButton(17, rotary_value <= -1); // left
          whButton(18, rotary_value >= 1); // right
        }

        // clutch paddle
        switch(csw_in.garbage[2] & 0x0F) {
          case 0x01:
            // bite point
            whDoubleClutch(~csw_in.axisX, ~csw_in.axisY);
            break;
          case 0x02:
            // bite point advanced
            whDoubleClutch(map(~csw_in.axisX & 0xFF,0,0xFF,0,clutch_max) , ~csw_in.axisY&0xff);
            break;
          default:
          whDoubleAxis(~csw_in.axisX, ~csw_in.axisY);
        }

        //whButton(11, mcl_in.buttons[2] & 0x20); // display button

          whButton(21, ((csw_in.garbage[3] & 0x0f) == 0x01) && !((csw_in.buttons[2] & 0x20) == 0x20));
          whButton(22, ((csw_in.garbage[3] & 0x0f) == 0x02) && !((csw_in.buttons[2] & 0x20) == 0x20));
          whButton(23, ((csw_in.garbage[3] & 0x0f) == 0x03) && !((csw_in.buttons[2] & 0x20) == 0x20));
          whButton(24, ((csw_in.garbage[3] & 0x0f) == 0x04) && !((csw_in.buttons[2] & 0x20) == 0x20));
          whButton(25, ((csw_in.garbage[3] & 0x0f) == 0x05) && !((csw_in.buttons[2] & 0x20) == 0x20));
          whButton(26, ((csw_in.garbage[3] & 0x0f) == 0x06) && !((csw_in.buttons[2] & 0x20) == 0x20));
          whButton(27, ((csw_in.garbage[3] & 0x0f) == 0x07) && !((csw_in.buttons[2] & 0x20) == 0x20));
          whButton(28, ((csw_in.garbage[3] & 0x0f) == 0x08) && !((csw_in.buttons[2] & 0x20) == 0x20));
          whButton(29, ((csw_in.garbage[3] & 0x0f) == 0x09) && !((csw_in.buttons[2] & 0x20) == 0x20));
          whButton(30, ((csw_in.garbage[3] & 0x0f) == 0x0A) && !((csw_in.buttons[2] & 0x20) == 0x20));
          whButton(31, ((csw_in.garbage[3] & 0x0f) == 0x0B) && !((csw_in.buttons[2] & 0x20) == 0x20));
          whButton(32, ((csw_in.garbage[3] & 0x0f) == 0x0C) && !((csw_in.buttons[2] & 0x20) == 0x20));


          whStick(0, 0);
        }


        break;
      case CSL_WHEEL:
        // csl stuff
        transferCslData(&csl_out, &csl_in, sizeof(csl_out.raw), 0x00);
        whSetId(CSLP1XBOX);
        init_wheel();

        // Joystick / 1st disp
        csl_out.disp = csw7segToCsl(csw_out.disp[0]);
        transferCslData(&csl_out, &csl_in, sizeof(csl_out.raw), 0x41);
        whHat(csl_in.buttons & 0x1E, true);
        whButton(13, csl_in.buttons & 0x01); // hat button

        // Right Line / 2st disp
        csl_out.disp = csw7segToCsl(csw_out.disp[1]);
        transferCslData(&csl_out, &csl_in, sizeof(csl_out.raw), 0x02);
        whButton(11, csl_in.buttons & 0x01); // wrench
        whButton(5, csl_in.buttons & 0x04); // RT
        whButton(14, csl_in.buttons & 0x08); // xbox
        whButton(6, csl_in.buttons & 0x10); // RSB

        // Left cluster / 3st disp
        csl_out.disp = csw7segToCsl(csw_out.disp[2]);
        transferCslData(&csl_out, &csl_in, sizeof(csl_out.raw), 0x44);
        whButton(15, csl_in.buttons & 0x01); // left paddle
        whButton(9, csl_in.buttons & 0x02); // lines
        whButton(10, csl_in.buttons & 0x04); // squares
        whButton(8, csl_in.buttons & 0x08); // LSB
        whButton(7, csl_in.buttons & 0x10); // LT

        // Right cluster / RGB Led
        csl_out.disp = cswLedsToCsl(csw_out.leds);
        transferCslData(&csl_out, &csl_in, sizeof(csl_out.raw), 0x08);
        whButton(16, csl_in.buttons & 0x01); // right paddle
        whButton(1, csl_in.buttons & 0x02); // B
        whButton(2, csl_in.buttons & 0x04); // A
        whButton(3, csl_in.buttons & 0x08); // Y
        whButton(4, csl_in.buttons & 0x10); // X

        whStick(0, 0);

        break;
      case MCL_WHEEL:
        // McLaren GT3

        transferMclData(&mcl_out, &mcl_in, sizeof(mcl_out.raw));
        init_wheel();

        // Wheel ID
        whSetId(mcl_in.id);

        whHat(mcl_in.buttons[0] & 0x0f, false);

        // All buttons
        whButton(1, mcl_in.buttons[0] & 0x80); // Y
        whButton(2, mcl_in.buttons[0] & 0x40); // B
        whButton(3, mcl_in.buttons[0] & 0x20); // X
        whButton(4, mcl_in.buttons[0] & 0x10); // A
        whButton(5, mcl_in.buttons[1] & 0x80); // P
        whButton(6, mcl_in.buttons[1] & 0x40); // N
        whButton(7, mcl_in.buttons[1] & 0x20); // LSB


        whButton(9, mcl_in.buttons[1] & 0x04); // RSB

        whButton(8, (mcl_in.buttons[1] & 0x10) && ((mcl_in.garbage[3] & 0xF0) == 0x10)); // switch left up
        whButton(19, (mcl_in.buttons[2] & 0x80) && ((mcl_in.garbage[3] & 0xF0) == 0x10)); // switch left down
        whButton(10, (mcl_in.buttons[1] & 0x02) && ((mcl_in.garbage[3] & 0xF0) == 0x10)); // switch right up
        whButton(20, (mcl_in.buttons[2] & 0x40) && ((mcl_in.garbage[3] & 0xF0) == 0x10)); // switch right down

        whButton(33, (mcl_in.buttons[1] & 0x10) && ((mcl_in.garbage[3] & 0xF0) == 0x20)); // switch left up
        whButton(34, (mcl_in.buttons[2] & 0x80) && ((mcl_in.garbage[3] & 0xF0) == 0x20)); // switch left down
        whButton(35, (mcl_in.buttons[1] & 0x02) && ((mcl_in.garbage[3] & 0xF0) == 0x20)); // switch right up
        whButton(36, (mcl_in.buttons[2] & 0x40) && ((mcl_in.garbage[3] & 0xF0) == 0x20)); // switch right down

        whButton(37, (mcl_in.buttons[1] & 0x10) && ((mcl_in.garbage[3] & 0xF0) == 0x30)); // switch left up
        whButton(38, (mcl_in.buttons[2] & 0x80) && ((mcl_in.garbage[3] & 0xF0) == 0x30)); // switch left down
        whButton(39, (mcl_in.buttons[1] & 0x02) && ((mcl_in.garbage[3] & 0xF0) == 0x30)); // switch right up
        whButton(40, (mcl_in.buttons[2] & 0x40) && ((mcl_in.garbage[3] & 0xF0) == 0x30)); // switch right down

        whButton(41, (mcl_in.buttons[1] & 0x10) && ((mcl_in.garbage[3] & 0xF0) == 0x40)); // switch left up
        whButton(42, (mcl_in.buttons[2] & 0x80) && ((mcl_in.garbage[3] & 0xF0) == 0x40)); // switch left down
        whButton(43, (mcl_in.buttons[1] & 0x02) && ((mcl_in.garbage[3] & 0xF0) == 0x40)); // switch right up
        whButton(44, (mcl_in.buttons[2] & 0x40) && ((mcl_in.garbage[3] & 0xF0) == 0x40)); // switch right down

        whButton(45, (mcl_in.buttons[1] & 0x10) && ((mcl_in.garbage[3] & 0xF0) == 0x50)); // switch left up
        whButton(46, (mcl_in.buttons[2] & 0x80) && ((mcl_in.garbage[3] & 0xF0) == 0x50)); // switch left down
        whButton(47, (mcl_in.buttons[1] & 0x02) && ((mcl_in.garbage[3] & 0xF0) == 0x50)); // switch right up
        whButton(48, (mcl_in.buttons[2] & 0x40) && ((mcl_in.garbage[3] & 0xF0) == 0x50)); // switch right down

        whButton(49, (mcl_in.buttons[1] & 0x10) && ((mcl_in.garbage[3] & 0xF0) == 0x60)); // switch left up
        whButton(50, (mcl_in.buttons[2] & 0x80) && ((mcl_in.garbage[3] & 0xF0) == 0x60)); // switch left down
        whButton(51, (mcl_in.buttons[1] & 0x02) && ((mcl_in.garbage[3] & 0xF0) == 0x60)); // switch right up
        whButton(52, (mcl_in.buttons[2] & 0x40) && ((mcl_in.garbage[3] & 0xF0) == 0x60)); // switch right down

        whButton(53, (mcl_in.buttons[1] & 0x10) && ((mcl_in.garbage[3] & 0xF0) == 0x70)); // switch left up
        whButton(54, (mcl_in.buttons[2] & 0x80) && ((mcl_in.garbage[3] & 0xF0) == 0x70)); // switch left down
        whButton(55, (mcl_in.buttons[1] & 0x02) && ((mcl_in.garbage[3] & 0xF0) == 0x70)); // switch right up
        whButton(56, (mcl_in.buttons[2] & 0x40) && ((mcl_in.garbage[3] & 0xF0) == 0x70)); // switch right down

        whButton(57, (mcl_in.buttons[1] & 0x10) && ((mcl_in.garbage[3] & 0xF0) == 0x80)); // switch left up
        whButton(58, (mcl_in.buttons[2] & 0x80) && ((mcl_in.garbage[3] & 0xF0) == 0x80)); // switch left down
        whButton(59, (mcl_in.buttons[1] & 0x02) && ((mcl_in.garbage[3] & 0xF0) == 0x80)); // switch right up
        whButton(60, (mcl_in.buttons[2] & 0x40) && ((mcl_in.garbage[3] & 0xF0) == 0x80)); // switch right down

        whButton(61, (mcl_in.buttons[1] & 0x10) && ((mcl_in.garbage[3] & 0xF0) == 0x90)); // switch left up
        whButton(62, (mcl_in.buttons[2] & 0x80) && ((mcl_in.garbage[3] & 0xF0) == 0x90)); // switch left down
        whButton(63, (mcl_in.buttons[1] & 0x02) && ((mcl_in.garbage[3] & 0xF0) == 0x90)); // switch right up
        whButton(64, (mcl_in.buttons[2] & 0x40) && ((mcl_in.garbage[3] & 0xF0) == 0x90)); // switch right down

        whButton(65, (mcl_in.buttons[1] & 0x10) && ((mcl_in.garbage[3] & 0xF0) == 0xA0)); // switch left up
        whButton(66, (mcl_in.buttons[2] & 0x80) && ((mcl_in.garbage[3] & 0xF0) == 0xA0)); // switch left down
        whButton(67, (mcl_in.buttons[1] & 0x02) && ((mcl_in.garbage[3] & 0xF0) == 0xA0)); // switch right up
        whButton(68, (mcl_in.buttons[2] & 0x40) && ((mcl_in.garbage[3] & 0xF0) == 0xA0)); // switch right down

        whButton(69, (mcl_in.buttons[1] & 0x10) && ((mcl_in.garbage[3] & 0xF0) == 0xB0)); // switch left up
        whButton(70, (mcl_in.buttons[2] & 0x80) && ((mcl_in.garbage[3] & 0xF0) == 0xB0)); // switch left down
        whButton(71, (mcl_in.buttons[1] & 0x02) && ((mcl_in.garbage[3] & 0xF0) == 0xB0)); // switch right up
        whButton(72, (mcl_in.buttons[2] & 0x40) && ((mcl_in.garbage[3] & 0xF0) == 0xB0)); // switch right down

        whButton(73, (mcl_in.buttons[1] & 0x10) && ((mcl_in.garbage[3] & 0xF0) == 0xC0)); // switch left up
        whButton(74, (mcl_in.buttons[2] & 0x80) && ((mcl_in.garbage[3] & 0xF0) == 0xC0)); // switch left down
        whButton(75, (mcl_in.buttons[1] & 0x02) && ((mcl_in.garbage[3] & 0xF0) == 0xC0)); // switch right up
        whButton(76, (mcl_in.buttons[2] & 0x40) && ((mcl_in.garbage[3] & 0xF0) == 0xC0)); // switch right down

        whButton(12, mcl_in.buttons[2] & 0x04); // hat button
        whButton(14, mcl_in.buttons[2] & 0x02); // xbox

        // paddles shitfer
        whButton(15, mcl_in.buttons[1] & 0x08); // left
        whButton(16, mcl_in.buttons[1] & 0x01); // right


        rotary_value = mcl_in.encoder;

        if ((mcl_in.garbage[2] & 0x0F) == 0x02 && !mcl_in.axisX) // left clutch fully pressed
        {
          clutch_max = constrain(clutch_max + rotary_value, 0, 0xFF);
        } else {
          whButton(17, rotary_value <= -1); // left
          whButton(18, rotary_value >= 1); // right
        }

        // clutch paddle
        switch(mcl_in.garbage[2] & 0x0F) {
          case 0x01:
            // bite point
            whDoubleClutch(~mcl_in.axisX, ~mcl_in.axisY);
            break;
          case 0x02:
            // bite point advanced
            whDoubleClutch(map(~mcl_in.axisX & 0xFF,0,0xFF,0,clutch_max) , ~mcl_in.axisY&0xff);
            break;
          default:
          whDoubleAxis(~mcl_in.axisX, ~mcl_in.axisY);
        }

        //whButton(11, mcl_in.buttons[2] & 0x20); // display button

          whButton(21, ((mcl_in.garbage[3] & 0x0f) == 0x01) && !((mcl_in.buttons[2] & 0x20) == 0x20));
          whButton(22, ((mcl_in.garbage[3] & 0x0f) == 0x02) && !((mcl_in.buttons[2] & 0x20) == 0x20));
          whButton(23, ((mcl_in.garbage[3] & 0x0f) == 0x03) && !((mcl_in.buttons[2] & 0x20) == 0x20));
          whButton(24, ((mcl_in.garbage[3] & 0x0f) == 0x04) && !((mcl_in.buttons[2] & 0x20) == 0x20));
          whButton(25, ((mcl_in.garbage[3] & 0x0f) == 0x05) && !((mcl_in.buttons[2] & 0x20) == 0x20));
          whButton(26, ((mcl_in.garbage[3] & 0x0f) == 0x06) && !((mcl_in.buttons[2] & 0x20) == 0x20));
          whButton(27, ((mcl_in.garbage[3] & 0x0f) == 0x07) && !((mcl_in.buttons[2] & 0x20) == 0x20));
          whButton(28, ((mcl_in.garbage[3] & 0x0f) == 0x08) && !((mcl_in.buttons[2] & 0x20) == 0x20));
          whButton(29, ((mcl_in.garbage[3] & 0x0f) == 0x09) && !((mcl_in.buttons[2] & 0x20) == 0x20));
          whButton(30, ((mcl_in.garbage[3] & 0x0f) == 0x0A) && !((mcl_in.buttons[2] & 0x20) == 0x20));
          whButton(31, ((mcl_in.garbage[3] & 0x0f) == 0x0B) && !((mcl_in.buttons[2] & 0x20) == 0x20));
          whButton(32, ((mcl_in.garbage[3] & 0x0f) == 0x0C) && !((mcl_in.buttons[2] & 0x20) == 0x20));


          whStick(0, 0);


        #ifdef HAS_DEBUG
           Serial.print("MCL_IN:");
           for(int i=0; i<sizeof(mcl_in.raw); i++) {

              Serial.print(mcl_in.raw[i], HEX);
              Serial.print(":");
            }
            Serial.println();
        #endif

        break;
      default:
        // no wheel  ?
        whClear();
        delay(10);
    }

    // Need more inputs?
    // 8 Extra Buttons (pins 2 to 9 -> 41 to 48)
    for (int i = 0; i < 8; ++i)
    {
      whButton(77+i, !digitalRead(2+i));
    }

    // Send HID report (all inputs)
    #ifdef IS_USB
      uint32_t now = micros();
      if (now >= usb_time)
      {
        uint32_t delta_t = now - usb_time;
        #ifdef HAS_DEBUG
          Serial.println(String("usb loop time : ") + delta_t);
        #endif

        if (delta_t >= MAX_SPEED)
        {
          // delayMicroseconds(MAX_SPEED - delta_t);
          #ifdef HAS_DEBUG
              Serial.println(String("usb loop time pre send: ") + (micros() - usb_time));
          #endif
          Joystick.send_now();
          #ifdef HAS_DEBUG
              Serial.println(String("usb loop time post send: ") + (micros() - usb_time));
          #endif
          usb_time = micros();
        }
        else {
          #ifdef HAS_DEBUG
              Serial.println(String("skipping report"));
          #endif
        }
      }



      // rotary_debounce = 0;
    #else
      uint32_t timout;
      timout = micros() - timing;
        if(timout > max_delay || (in_changed && timout > 10000))
        {
          // hid_data[3] = (hid_data[3]+1)&0xff;
          iwrap_send_data(main_link_id, sizeof(hid_data), hid_data, iwrap_mode);
          timing = micros();
          #ifdef HAS_DEBUG
            if (rotary_debounce!=0)Serial.println(String("RESET!!!!!!!!!ROTARY : ") + rotary_value);
            if (in_changed)Serial.println("input sent");
          #endif
          in_changed = false;
          // rotary_debounce = 0;
        }
      #endif
  }

  #ifndef IS_USB
  // Read WT12 incoming data
  uint16_t result;

  while((result = WT12.read()) < 256 && !got_hid) iwrap_parse(result & 0xFF, iwrap_mode);
  if(got_hid) got_hid = false;
  #endif
  // timing = micros() - timing;
  // if(timing > 550)  Serial.println(String("timing: ") + timing);
  // some delay
  idle();
}

void whButton(uint8_t button, bool val) {
  val = btDebncer[button].get(val);
  #ifdef IS_USB
    Joystick.button(button, val);
  #else
    uint8_t old;
    if (--button >= 48) return;
      if (button >= 40) {
        old = hid_data[8];
        if (val) hid_data[8] |= (0x1 << (button-40));
        else hid_data[8] &= ~(0x1 << (button-40));
        if (old != hid_data[8]){
          in_changed = true;
          #ifdef HAS_DEBUG
            Serial.println(String("whButton: new input! ") + old + " -> " + hid_data[8]);
          #endif
        }
    } else if (button >= 32) {
        old = hid_data[7];
        if (val) hid_data[7] |= (0x1 << (button-32));
        else hid_data[7] &= ~(0x1 << (button-32));
        if (old != hid_data[7]){
          in_changed = true;
          #ifdef HAS_DEBUG
            Serial.println(String("whButton: new input! ") + old + " -> " + hid_data[7]);
          #endif
        }
    } else if (button >= 24) {
        old = hid_data[6];
        if (val) hid_data[6] |= (0x1 << (button-24));
        else hid_data[6] &= ~(0x1 << (button-24));
        if (old != hid_data[6]){
          in_changed = true;
          #ifdef HAS_DEBUG
            Serial.println(String("whButton: new input! ") + old + " -> " + hid_data[6]);
          #endif
        }
    } else if (button >= 16) {
        old = hid_data[5];
        if (val) hid_data[5] |= (0x1 << (button-16));
        else hid_data[5] &= ~(0x1 << (button-16));
        if (old != hid_data[5]){
          in_changed = true;
          #ifdef HAS_DEBUG
            Serial.println(String("whButton: new input! ") + old + " -> " + hid_data[5]);
          #endif
        }
    } else if (button >= 8) {
        old = hid_data[4];
        if (val) hid_data[4] |= (0x1 << (button-8));
        else hid_data[4] &= ~(0x1 << (button-8));
        if (old != hid_data[4]){
          in_changed = true;
          #ifdef HAS_DEBUG
            Serial.println(String("whButton: new input! ") + old + " -> " + hid_data[4]);
          #endif
        }
    } else {
        old = hid_data[3];
        if (val) hid_data[3] |= (0x1 << (button));
        else hid_data[3] &= ~(0x1 << (button));
        if (old != hid_data[3]){
          in_changed = true;
          #ifdef HAS_DEBUG
            Serial.println(String("whButton: new input! ") + old + " -> " + hid_data[3]);
          #endif
        }
    }
  #endif
}

void whStick(unsigned int x, unsigned int y) {
  x = 255 - (x + 127);
  y = y + 127;
  #ifdef IS_USB
    Joystick.X(x);
    Joystick.Y(y);
  #else
  uint8_t old = hid_data[9];
  hid_data[9] = x & 0xFF;
  if (old != hid_data[9]) {
    in_changed = true;
    #ifdef HAS_DEBUG
      Serial.println(String("whStick X: new input! ") + old + " -> " + hid_data[9]);
    #endif
  }
  old = hid_data[10];
  hid_data[10] = y & 0xFF;
  if (old != hid_data[10]){
    in_changed = true;
    #ifdef HAS_DEBUG
      Serial.println(String("whStick Y: new input! ") + old + " -> " + hid_data[10]);
    #endif
  }
  #endif
}

void whDoubleAxis(unsigned int x, unsigned int y) {
  #ifdef IS_USB
    Joystick.clutch1(x);
    Joystick.clutch2(y);
  #else
  uint8_t old = hid_data[9];
  hid_data[9] = x & 0xFF;
  if (old != hid_data[9]) {
    in_changed = true;
    #ifdef HAS_DEBUG
      Serial.println(String("whStick X: new input! ") + old + " -> " + hid_data[9]);
    #endif
  }
  old = hid_data[10];
  hid_data[10] = y & 0xFF;
  if (old != hid_data[10]){
    in_changed = true;
    #ifdef HAS_DEBUG
      Serial.println(String("whStick Y: new input! ") + old + " -> " + hid_data[10]);
    #endif
  }
  #endif
}

void whDoubleClutch(unsigned int x, unsigned int y) {
  if(y > x) x = y;

  #ifdef IS_USB
    Joystick.clutch1(x);
    Joystick.clutch2(0);
  #else
    uint8_t old = hid_data[9];
    hid_data[9] = x & 0xFF;
    if (old != hid_data[9]) {
      in_changed = true;
      #ifdef HAS_DEBUG
        Serial.println(String("whStick X: new input! ") + old + " -> " + hid_data[9]);
      #endif
    }
  #endif
}

void whHat(int8_t val, bool is_csl) {
  val = hatDebncer.get(val);
  if (is_csl) { // CSL
    switch (val){
      case 4: val=0;break;
      case 2: val=2;break;
      case 8: val=4;break;
      case 16: val=6;break;
      default: val=0xFF;
    }
  } else { // CSW
    switch (val){
      case 1: val=0;break;
      case 2: val=6;break;
      case 4: val=2;break;
      case 8: val=4;break;
      default: val=0xFF;
    }
  }
  #ifdef IS_USB
    Joystick.hat(val);
  #else
    uint8_t old = hid_data[11];
    hid_data[11] = val & 0xFF;
    if (old != hid_data[11]){
      in_changed = true;
      #ifdef HAS_DEBUG
        Serial.println(String("whHat: new input! ") + old + " -> " + hid_data[11]);
      #endif
    }
  #endif
}

void whSetId(unsigned int val) {
  csw_out.id = val & 0xFF;
  #ifdef IS_USB
    Joystick.setWheel(val);
  #else
    uint8_t old = hid_data[32];
    hid_data[32] = csw_out.id;
    if (old != hid_data[32]){
      in_changed = true;
      #ifdef HAS_DEBUG
        Serial.println(String("whSetId: new input! ") + old + " -> " + hid_data[32]);
      #endif
    }
  #endif
}

void whClear(){
  whSetId(NO_RIM);
  whStick(0, 0);
  whHat(0, false);
  whDoubleAxis(0x00, 0x00);
  for (int i = 1; i <= 37; ++i)
  {
    whButton(i, 0);
  }
  clutch_max = 0xFF;
  show_fwvers = true;
  disp_timout = 0;
}

int iwrap_out(int len, unsigned char *data) {
  // iWRAP output to module goes through software serial
  int nbytes = WT12.availableForWrite();

  if(digitalRead(CTS) == LOW && nbytes >= len){
      nbytes = WT12.write(data, len);
      //Serial.println(String("sent : ") + nbytes );
  } else {
      #ifdef HAS_DEBUG
        Serial.println(String("[!] Throttling (")+max_delay+")");
      #endif
      max_delay++;
      //delay(1000);
  }
  // got_hid = true;
  return nbytes;
}

void hid_output(uint8_t link_id, uint16_t data_length, const uint8_t *data) {
  if(data[2] == 0x01 && data[3] == 0x02){
      // 7 seg
    if(!show_fwvers){
      if (detectWheelType() == MCL_WHEEL) {
        mcl_out.raw[1] = 0x11;
        mcl_out.raw[2] = csw7segToAscii(data[4] & 0xff);
        mcl_out.raw[3] = csw7segToAscii(data[5] & 0xff);
        mcl_out.raw[4] = csw7segToAscii(data[6] & 0xff);

      #ifdef HAS_DEBUG
        Serial.print(String("HID display: " ));
        Serial.print(data[4], HEX);
        Serial.print(":");
        Serial.print(data[5], HEX);
        Serial.print(":");
        Serial.print(data[6], HEX);
        Serial.println();
      #endif
      } else {
        if(csw_in.id == CSLMCLGT3){
          csw_out.raw[1] = 0x11;
          csw_out.raw[2] = csw7segToAscii(data[4] & 0xff);
          csw_out.raw[3] = csw7segToAscii(data[5] & 0xff);
          csw_out.raw[4] = csw7segToAscii(data[6] & 0xff);
        } else {
          csw_out.disp[0] = (data[4] & 0xff);
          csw_out.disp[1] = (data[5] & 0xff);
          csw_out.disp[2] = (data[6] & 0xff);
        }
      #ifdef HAS_DEBUG
        Serial.println(String("HID display: " )+ csw_out.disp[0]+":"+csw_out.disp[1]+":"+csw_out.disp[2]);
      #endif
      }
    }
  } else if(data[2] == 0x01 && data[3] == 0x03){
      // rumbles
    if (csw_out.id != UNIHUB && csw_in.id != CSLMCLGT3){
      csw_out.rumble[0] = (data[4] & 0xff);
      csw_out.rumble[1] = (data[5] & 0xff);
    }
      #ifdef HAS_DEBUG
        Serial.println(String("HID rumbles: " )+ csw_out.rumble[0]+":"+csw_out.rumble[1]);
      #endif
  } else if(data[2] == 0x08){
      // Rev Lights
    if (csw_out.id != UNIHUB && csw_in.id != CSLMCLGT3){
      csw_out.leds = (data[3] & 0xff) << 8 | (data[4] & 0xff);
      // ftx_pck[5] = (hid_pck[4] & 0xff);
      // ftx_pck[6] = (hid_pck[3] & 0xff);
    }
      #ifdef HAS_DEBUG
        Serial.println(String("HID leds   : " )+ csw_out.leds);
      #endif
  } else if(data[1] == 0x14){
      // ??

  } else {

      #ifdef HAS_DEBUG
        Serial.print(String("[!] HID Unknown from " )+ link_id + " (size:" + data_length + ") : ");

        for(int i=0; i<data_length; i++) {
          Serial.print(data[i], HEX);
          Serial.print(":");

        }
        Serial.println();
      #endif

  }
  timing_bt = millis();
}


void init_wheel() {
  if(show_fwvers){

    if (disp_timout == 0){
      // start showing fw vers
      if (detectWheelType() == MCL_WHEEL) {
        String fw_vers = String(mcl_in.fwvers);
        mcl_out.raw[1] = 0x11;
        // Erase all
        mcl_out.raw[2] = 0x0A;
        mcl_out.raw[3] = 0x0A;
        mcl_out.raw[4] = 0x0A;
        //
        mcl_out.raw[2] = fw_vers.charAt(0);
        if(fw_vers.length()>1)
          mcl_out.raw[3] = fw_vers.charAt(1);
        if(fw_vers.length()>2)
          mcl_out.raw[4] = fw_vers.charAt(2);
      } else {
        String fw_vers = String(csw_in.fwvers);
        if(csw_in.id == CSLMCLGT3){
          csw_out.raw[1] = 0x11;
          // Erase all
          csw_out.raw[2] = 0x0A;
          csw_out.raw[3] = 0x0A;
          csw_out.raw[4] = 0x0A;
          //
          csw_out.raw[2] = fw_vers.charAt(0);
          if(fw_vers.length()>1)
            csw_out.raw[3] = fw_vers.charAt(1);
          if(fw_vers.length()>2)
            csw_out.raw[4] = fw_vers.charAt(2);
        } else {
          // TODO: AsciiTo7seg conversion
          csw_out.disp[0] = 0x00;
          csw_out.disp[1] = 0x00;
          csw_out.disp[2] = 0x00;
        }
      }
      disp_timout = millis();
    } else if(millis() - disp_timout >= 4000) {
      // stop
      show_fwvers = false;
      mcl_out.raw[1] = 0x11;
      mcl_out.raw[2] = 0x0A;
      mcl_out.raw[3] = 0x0A;
      mcl_out.raw[4] = 0x0A;
      mcl_out.raw[9] = 0x00;
      if(csw_in.id == CSLMCLGT3){
        csw_out.raw[1] = 0x11;
        csw_out.raw[2] = 0x0A;
        csw_out.raw[3] = 0x0A;
        csw_out.raw[4] = 0x0A;
      }
    }
  } else {
      // xbox light
  //    mcl_out.raw[1] = 0x00;
  //    mcl_out.raw[9] = 0x0F;
  }

}


#ifdef HAS_DEBUG
int my_iwrap_debug(const char *data) {
  return Serial.print(data);
}
#endif

void my_iwrap_evt_ring(uint8_t link_id, const iwrap_address_t *address, uint16_t channel, const char *profile) {
  #ifdef HAS_DEBUG
    Serial.println(String("Connection from " )+ link_id);
  #endif
  bt_connected = true;
}

void my_iwrap_evt_hid_suspend(uint8_t link_id) {
  #ifdef HAS_DEBUG
    Serial.println(String("Disconnection from " )+ link_id);
  #endif
  bt_connected = false;
}

void my_iwrap_evt_no_carrier(uint8_t link_id, uint16_t error_code, const char *message) {
  #ifdef HAS_DEBUG
    Serial.println(String("Disconnection from " )+ link_id);
  #endif
  bt_connected = false;
}

void my_iwrap_rsp_list_result(uint8_t link_id, const char *mode, uint16_t blocksize, uint32_t elapsed_time, uint16_t local_msc, uint16_t remote_msc, const iwrap_address_t *bd_addr, uint16_t channel, uint8_t direction, uint8_t powermode, uint8_t role, uint8_t crypt, uint16_t buffer, uint8_t eretx) {
  #ifdef HAS_DEBUG
    Serial.println(String("Already connected to ") + link_id);
  #endif
  bt_connected = true;
}

void idle() {
  #ifndef IS_USB
    delay(1);
  #endif
}


