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

/* Wheel inputs */
void whClear();
void whButton(uint8_t button, bool val);
void whStick(unsigned int x, unsigned int y);
void whHat(int8_t val, bool is_csl);
void whSetId(unsigned int val);

csw_in_t csw_in;
csw_out_t csw_out;

csl_in_t csl_in;
csl_out_t csl_out;

bool bt_connected;
bool got_hid;
uint32_t timing;
uint32_t timing_bt;

uint8_t hid_pck[7];

Debouncer * btDebncer = new Debouncer[49];
Debouncer hatDebncer = Debouncer();

byte rotary_debounce = 0;
int8_t rotary_value = 0;
uint8_t main_link_id = 1;

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

  // prebuild output packet
  memset(csw_out.raw, 0, sizeof(csw_out_t));
  memset(csl_out.raw, 0, sizeof(csl_out_t));
  csw_out.header = 0xa5;
  csw_out.id = 0x00;
  got_hid = false;

  // debug
  #ifdef HAS_DEBUG
    // iwrap_debug = my_iwrap_debug;
    Serial.begin(115200);
    Serial.println("MCU Ready");
    delay(5000);
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
        transferCswData(&csw_out, &csw_in, sizeof(csw_out.raw));

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
        whButton(8, csw_in.buttons[1] & 0x10); // first top left
        whButton(9, csw_in.buttons[1] & 0x04); // bottom left
        whButton(10, csw_in.buttons[1] & 0x02); // second top left
        whButton(11, csw_in.buttons[2] & 0x08); // second center
        whButton(12, csw_in.buttons[2] & 0x04); // stick button
        whButton(13, csw_in.buttons[2] & 0x02); // hat button
        whButton(14, csw_in.buttons[2] & 0x20); // display button

        // paddles shitfer
        whButton(15, csw_in.buttons[1] & 0x08); // left
        whButton(16, csw_in.buttons[1] & 0x01); // right

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

        rotary_value = csw_in.encoder;

        whButton(17, rotary_value == -1); // left
        whButton(18, rotary_value == 1); // right

        whHat(csw_in.buttons[0] & 0x0f, false);

        // Serial.println(String("button: ") + hid_data[3]);

        break;
      case CSL_WHEEL:
        // csl stuff
        transferCslData(&csl_out, &csl_in, sizeof(csl_out.raw), 0x00);
        whSetId(CSLP1XBOX);

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
      default:
        // no wheel  ?
        whClear();
        delay(10);
    }

    // Need more inputs?
    // 8 Extra Buttons (pins 2 to 9 -> 41 to 48)
    for (int i = 0; i < 8; ++i)
    {
      whButton(41+i, !digitalRead(2+i));
    }

    // Send HID report (all inputs)
    #ifdef IS_USB
      Joystick.send_now();
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
  for (int i = 1; i <= 37; ++i)
  {
    whButton(i, 0);
  }
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
  #ifdef HAS_DEBUG
  timing_bt = millis() - timing_bt;
  if(timing_bt <= 35){
    Serial.println(String("[W] low timing :") + timing_bt + "ms ");
    // delay(10+(35-timing_bt));
  }
  #endif
  if(data[2] == 0x01 && data[3] == 0x02){
      // 7 seg
      csw_out.disp[0] = (data[4] & 0xff);
      csw_out.disp[1] = (data[5] & 0xff);
      csw_out.disp[2] = (data[6] & 0xff);
      #ifdef HAS_DEBUG
        Serial.println(String("HID display: " )+ csw_out.disp[0]+":"+csw_out.disp[1]+":"+csw_out.disp[2]);
      #endif
  } else if(data[2] == 0x01 && data[3] == 0x03){
      // rumbles
    if (csw_out.id != UNIHUB){
      csw_out.rumble[0] = (data[4] & 0xff);
      csw_out.rumble[1] = (data[5] & 0xff);
    }
      #ifdef HAS_DEBUG
        Serial.println(String("HID rumbles: " )+ csw_out.rumble[0]+":"+csw_out.rumble[1]);
      #endif
  } else if(data[2] == 0x08){
      // Rev Lights
    if (csw_out.id != UNIHUB){
      csw_out.leds = (data[3] & 0xff) << 8 | (data[4] & 0xff);
      // ftx_pck[5] = (hid_pck[4] & 0xff);
      // ftx_pck[6] = (hid_pck[3] & 0xff);
    }
      #ifdef HAS_DEBUG
        Serial.println(String("HID leds   : " )+ csw_out.leds);
      #endif
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


