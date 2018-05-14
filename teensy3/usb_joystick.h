/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2013 PJRC.COM, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef USBjoystick_h_
#define USBjoystick_h_

#include "usb_desc.h"

#if defined(JOYSTICK_INTERFACE)

#include <inttypes.h>

// C language implementation
#ifdef __cplusplus
extern "C" {
#endif
int usb_joystick_send(void);
extern uint8_t usb_joystick_data[32];
int usb_lights_recv(void *buffer, uint32_t timeout);
int usb_lights_available(void);

#ifdef __cplusplus
}
#endif

// C++ interface
#ifdef __cplusplus
class usb_joystick_class
{
    private:
        static uint8_t manual_mode;

    public:
        void begin(void) { }
        void end(void) { }
        void button(uint8_t button, bool val) {
            if (--button >= 88) return;
            if (button >= 80) {
                if (val) usb_joystick_data[10] |= (0x1 << (button-80));
                else usb_joystick_data[10] &= ~(0x1 << (button-80));
            } else if (button >= 72) {
                if (val) usb_joystick_data[9] |= (0x1 << (button-72));
                else usb_joystick_data[9] &= ~(0x1 << (button-72));
            } else if (button >= 64) {
                if (val) usb_joystick_data[8] |= (0x1 << (button-64));
                else usb_joystick_data[8] &= ~(0x1 << (button-64));
            } else if (button >= 56) {
                if (val) usb_joystick_data[7] |= (0x1 << (button-56));
                else usb_joystick_data[7] &= ~(0x1 << (button-56));
            } else if (button >= 48) {
                if (val) usb_joystick_data[6] |= (0x1 << (button-48));
                else usb_joystick_data[6] &= ~(0x1 << (button-48));
            } else if (button >= 40) {
                if (val) usb_joystick_data[5] |= (0x1 << (button-40));
                else usb_joystick_data[5] &= ~(0x1 << (button-40));
            } else if (button >= 32) {
                if (val) usb_joystick_data[4] |= (0x1 << (button-32));
                else usb_joystick_data[4] &= ~(0x1 << (button-32));
            } else if (button >= 24) {
                if (val) usb_joystick_data[3] |= (0x1 << (button-24));
                else usb_joystick_data[3] &= ~(0x1 << (button-24));
            } else if (button >= 16) {
                if (val) usb_joystick_data[2] |= (0x1 << (button-16));
                else usb_joystick_data[2] &= ~(0x1 << (button-16));
            } else if (button >= 8) {
                if (val) usb_joystick_data[1] |= (0x1 << (button-8));
                else usb_joystick_data[1] &= ~(0x1 << (button-8));
            } else {
                if (val) usb_joystick_data[0] |= (0x1 << (button));
                else usb_joystick_data[0] &= ~(0x1 << (button));
            }
            if (!manual_mode) usb_joystick_send();
        }

        void X(unsigned int val) {
            usb_joystick_data[11] = val & 0xFF;
            if (!manual_mode) usb_joystick_send();
        }

        void Y(unsigned int val) {
            usb_joystick_data[12] = val & 0xFF;
            if (!manual_mode) usb_joystick_send();
        }

        void clutch1(unsigned int val) {
            usb_joystick_data[13] = val & 0xFF;
            if (!manual_mode) usb_joystick_send();
        }

        void clutch2(unsigned int val) {
            usb_joystick_data[14] = val & 0xFF;
            if (!manual_mode) usb_joystick_send();
        }

        inline void hat(int val) {
            usb_joystick_data[15] = val & 0xFF;
            if (!manual_mode) usb_joystick_send();
        }

        void setWheel(unsigned int val) {
            usb_joystick_data[29] = val & 0xFF;
            if (!manual_mode) usb_joystick_send();
        }

        void useManualSend(bool mode) {
            manual_mode = mode;
        }
        void send_now(void) {
            usb_joystick_send();
        }

        int available(void) {return usb_lights_available(); }
        int recv(void *buffer, uint16_t timeout) { return usb_lights_recv(buffer, timeout); }

};
extern usb_joystick_class Joystick;

#endif // __cplusplus

#endif // JOYSTICK_INTERFACE

#endif // USBjoystick_h_

