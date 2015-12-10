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

#if F_CPU >= 20000000

#define USB_DESC_LIST_DEFINE
#include "usb_desc.h"
#ifdef NUM_ENDPOINTS
#include "usb_names.h"
#include "kinetis.h"
#include "avr_functions.h"

// USB Descriptors are binary data which the USB host reads to
// automatically detect a USB device's capabilities.  The format
// and meaning of every field is documented in numerous USB
// standards.  When working with USB descriptors, despite the
// complexity of the standards and poor writing quality in many
// of those documents, remember descriptors are nothing more
// than constant binary data that tells the USB host what the
// device can do.  Computers will load drivers based on this data.
// Those drivers then communicate on the endpoints specified by
// the descriptors.

// To configure a new combination of interfaces or make minor
// changes to existing configuration (eg, change the name or ID
// numbers), usually you would edit "usb_desc.h".  This file
// is meant to be configured by the header, so generally it is
// only edited to add completely new USB interfaces or features.



// **************************************************************
//   USB Device
// **************************************************************

#define LSB(n) ((n) & 255)
#define MSB(n) (((n) >> 8) & 255)

// USB Device Descriptor.  The USB host reads this first, to learn
// what type of device is connected.
static uint8_t device_descriptor[] = {
        18,                                     // bLength
        1,                                      // bDescriptorType
        0x00, 0x02,                             // bcdUSB
        0,                                      // bDeviceClass
        0,                                      // bDeviceSubClass
        0,                                      // bDeviceProtocol
        EP0_SIZE,                               // bMaxPacketSize0
        LSB(VENDOR_ID), MSB(VENDOR_ID),         // idVendor
        LSB(PRODUCT_ID), MSB(PRODUCT_ID),       // idProduct
        0x01, 0x02,                             // bcdDevice
        1,                                      // iManufacturer
        2,                                      // iProduct
        3,                                      // iSerialNumber
        1                                       // bNumConfigurations
};

// These descriptors must NOT be "const", because the USB DMA
// has trouble accessing flash memory with enough bandwidth
// while the processor is executing from flash.



// **************************************************************
//   HID Report Descriptors
// **************************************************************

// Each HID interface needs a special report descriptor that tells
// the meaning and format of the data.




static uint8_t joystick_report_desc[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x04,                    // USAGE (Joystick)
    0xa1, 0x01,                    // COLLECTION (Application)

        // 18 Buttons (24bits)
        0x05, 0x09,                    //   USAGE_PAGE (Button)
            0x19, 0x01,                    //   USAGE_MINIMUM (Button 1)
            0x29, 0x12,                    //   USAGE_MAXIMUM (Button 18)
            0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
            0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
            0x75, 0x01,                    //   REPORT_SIZE (1)
            0x95, 0x12,                    //   REPORT_COUNT (18)
            0x81, 0x02,                    //   INPUT (Data,Var,Abs)
              //   padding ( 18 + 6 )
            0x95, 0x01,                    //   REPORT_COUNT (1)
            0x75, 0x06,                    //   REPORT_SIZE (6)
            0x81, 0x01,                    //   INPUT (Cnst,Ary,Abs)

        // left stick axis (16bits)
        0x05, 0x01,                    //   USAGE_PAGE (Generic Desktop)
            0x09, 0x30,                    //   USAGE (X)
            0x09, 0x31,                    //   USAGE (Y)
            0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
            0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
            0x75, 0x08,                    //   REPORT_SIZE (8)
            0x95, 0x02,                    //   REPORT_COUNT (2)
            0x81, 0x02,                    //   INPUT (Data,Var,Abs)
              //   no padding ( 16 )

        // Hat switch 
        0x05, 0x01,                    //   USAGE_PAGE (Generic Desktop)
            0x09, 0x39,                    //   USAGE (Hat)
            0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
            0x25, 0x07,                    //   LOGICAL_MAXIMUM (7)
            0x35, 0x00,                    //   Physical Minimum (0)
            0x46, 0x3B, 0x01,              //   Physical Maximum (315)
            0x65, 0x14,                    //   Unit (20)
            0x75, 0x04,                    //   REPORT_SIZE (4)
            0x95, 0x01,                    //   REPORT_COUNT (1)
            0x81, 0x42,                    //   INPUT (Data,Var,Abs)
              //   padding ( 4 + 4 )
            0x95, 0x35,                    //   REPORT_COUNT (1)
            0x75, 0x04,                    //   REPORT_SIZE (4)
            0x81, 0x01,                    //   INPUT (Cnst,Ary,Abs)


    // 4 LEDs
    
    0x05, 0x0a,                    //   USAGE_PAGE (Ordinals)
    0x09, 0x01,                    //   USAGE (Instance 1)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x05, 0x08,                    //     USAGE_PAGE (LEDs)
    0x09, 0x4b,                    //     USAGE (Generic Indicator)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x91, 0x02,                    //     OUTPUT (Data,Var,Abs)
    0xc0,                          //   END_COLLECTION
    0x09, 0x02,                    //   USAGE (Instance 2)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x05, 0x08,                    //     USAGE_PAGE (LEDs)
    0x09, 0x4b,                    //     USAGE (Generic Indicator)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x91, 0x02,                    //     OUTPUT (Data,Var,Abs)
    0xc0,                          //   END_COLLECTION
    0x09, 0x03,                    //   USAGE (Instance 3)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x05, 0x08,                    //     USAGE_PAGE (LEDs)
    0x09, 0x4b,                    //     USAGE (Generic Indicator)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x91, 0x02,                    //     OUTPUT (Data,Var,Abs)
    0xc0,                          //   END_COLLECTION
    0x09, 0x04,                    //   USAGE (Instance 4)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x05, 0x08,                    //     USAGE_PAGE (LEDs)
    0x09, 0x4b,                    //     USAGE (Generic Indicator)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x91, 0x02,                    //     OUTPUT (Data,Var,Abs)
    0xc0,                          //   END_COLLECTION
      //   padding ( 4 + 4 )
    0x95, 0x0d,                    //   REPORT_COUNT (13)
    0x75, 0x04,                    //   REPORT_SIZE (4)
    0x91, 0x01,                    // OUTPUT (Cnst,Ary,Abs)
      //  no padding ( 64 )
    0xc0                           // END_COLLECTION
    
    /*
    0x05, 0x0a,                    //   USAGE_PAGE (Ordinals)
    0x09, 0x01,                    //   USAGE (Instance 1)
    0xa1, 0x01,                    //   COLLECTION (Application)
    //0x85, 0x01,                      // REPORT_ID (1) 
    0x05, 0x08,                    //     USAGE_PAGE (LEDs)
    0x09, 0x4b,                    //     USAGE (Generic Indicator)
    0x75, 0x01,                    //     REPORT_SIZE (8)
    0x95, 0x07,                    //     REPORT_COUNT (7)
    0x91, 0x02,                    //     OUTPUT (Data,Var,Abs)
    0xc0,                          //   END_COLLECTION

    0x91, 0x02,                    // OUTPUT (Cnst,Ary,Abs)
      //  no padding ( 64 )
    0xc0                           // END_COLLECTION
    */
};


// **************************************************************
//   USB Configuration
// **************************************************************

// USB Configuration Descriptor.  This huge descriptor tells all
// of the devices capbilities.
static uint8_t config_descriptor[CONFIG_DESC_SIZE] = {
        // configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10
        9,                                      // bLength;
        2,                                      // bDescriptorType;
        LSB(CONFIG_DESC_SIZE),                 // wTotalLength
        MSB(CONFIG_DESC_SIZE),
        NUM_INTERFACE,                          // bNumInterfaces
        1,                                      // bConfigurationValue
        0,                                      // iConfiguration
        0xC0,                                   // bmAttributes (self powered)
        50,                                     // bMaxPower (100mA)



// Sliders + Buttons
        // interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
        9,                                      // bLength
        4,                                      // bDescriptorType
        JOYSTICK_INTERFACE,                     // bInterfaceNumber
        0,                                      // bAlternateSetting
        2,                                      // bNumEndpoints
        0x03,                                   // bInterfaceClass (0x03 = HID)
        0x00,                                   // bInterfaceSubClass
        0x00,                                   // bInterfaceProtocol
        4,                                      // iInterface
        // HID interface descriptor, HID 1.11 spec, section 6.2.1
        9,                                      // bLength
        0x21,                                   // bDescriptorType
        0x11, 0x01,                             // bcdHID
        0,                                      // bCountryCode
        1,                                      // bNumDescriptors
        0x22,                                   // bDescriptorType
        LSB(sizeof(joystick_report_desc)),      // wDescriptorLength
        MSB(sizeof(joystick_report_desc)),
        // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
        7,                                      // bLength
        5,                                      // bDescriptorType
        JOYSTICK_ENDPOINT | 0x80,               // bEndpointAddress
        0x03,                                   // bmAttributes (0x03=intr)
        JOYSTICK_SIZE, 0,                       // wMaxPacketSize
        JOYSTICK_INTERVAL,                      // bInterval
	// TODO: OUT endpoint for LEDs
        // OUT endpoint descriptor
        7,                                      // bLength
        5,                                      // bDescriptorType
        LIGHTS_ENDPOINT | 0x00,                 // bEndpointAddress (OUT)
        0x03,                                   // bmAttributes (0x03=intr)
        LIGHTS_SIZE, 0,                         // wMaxPacketSize
        LIGHTS_INTERVAL,                        // bInterval

};


// **************************************************************
//   String Descriptors
// **************************************************************

// The descriptors above can provide human readable strings,
// referenced by index numbers.  These descriptors are the
// actual string data

/* defined in usb_names.h
struct usb_string_descriptor_struct {
        uint8_t bLength;
        uint8_t bDescriptorType;
        uint16_t wString[];
};
*/

extern struct usb_string_descriptor_struct usb_string_manufacturer_name
        __attribute__ ((weak, alias("usb_string_manufacturer_name_default")));
extern struct usb_string_descriptor_struct usb_string_product_name
        __attribute__ ((weak, alias("usb_string_product_name_default")));
extern struct usb_string_descriptor_struct usb_string_serial_number
        __attribute__ ((weak, alias("usb_string_serial_number_default")));
extern struct usb_string_descriptor_struct usb_string_interface_name
        __attribute__ ((weak, alias("usb_string_interface_name_default")));


struct usb_string_descriptor_struct string0 = {
        4,
        3,
        {0x0409}
};

struct usb_string_descriptor_struct usb_string_manufacturer_name_default = {
        2 + MANUFACTURER_NAME_LEN * 2,
        3,
        MANUFACTURER_NAME
};
struct usb_string_descriptor_struct usb_string_product_name_default = {
	2 + PRODUCT_NAME_LEN * 2,
        3,
        PRODUCT_NAME
};
struct usb_string_descriptor_struct usb_string_serial_number_default = {
        12,
        3,
        {0,0,0,0,0,0,0,0,0,0}
};
struct usb_string_descriptor_struct usb_string_interface_name_default = {
        2 + JOYSTICK_NAME_LEN * 2,
        3,
        JOYSTICK_NAME
};

void usb_init_serialnumber(void)
{
	char buf[11];
	uint32_t i, num;

	__disable_irq();
	FTFL_FSTAT = FTFL_FSTAT_RDCOLERR | FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL;
	FTFL_FCCOB0 = 0x41;
	FTFL_FCCOB1 = 15;
	FTFL_FSTAT = FTFL_FSTAT_CCIF;
	while (!(FTFL_FSTAT & FTFL_FSTAT_CCIF)) ; // wait
	num = *(uint32_t *)&FTFL_FCCOB7;
	__enable_irq();
	// add extra zero to work around OS-X CDC-ACM driver bug
	if (num < 10000000) num = num * 10;
	ultoa(num, buf, 10);
	for (i=0; i<10; i++) {
		char c = buf[i];
		if (!c) break;
		usb_string_serial_number_default.wString[i] = c;
	}
	usb_string_serial_number_default.bLength = i * 2 + 2;
}


// **************************************************************
//   Descriptors List
// **************************************************************

// This table provides access to all the descriptor data above.

const usb_descriptor_list_t usb_descriptor_list[] = {
	//wValue, wIndex, address,          length
	{0x0100, 0x0000, device_descriptor, sizeof(device_descriptor)},
	{0x0200, 0x0000, config_descriptor, sizeof(config_descriptor)},

// Sliders + Buttons
        {0x2200, JOYSTICK_INTERFACE, joystick_report_desc, sizeof(joystick_report_desc)},
        {0x2100, JOYSTICK_INTERFACE, config_descriptor+JOYSTICK_DESC_OFFSET, 9},


        {0x0300, 0x0000, (const uint8_t *)&string0, 0},
        {0x0301, 0x0409, (const uint8_t *)&usb_string_manufacturer_name, 0},
        {0x0302, 0x0409, (const uint8_t *)&usb_string_product_name, 0},
        {0x0303, 0x0409, (const uint8_t *)&usb_string_serial_number, 0},
        {0x0304, 0x0409, (const uint8_t *)&usb_string_interface_name, 0},
        //{0x0305, 0x0409, (const uint8_t *)&usb_string_disp_name, 0},
        //{0x0301, 0x0409, (const uint8_t *)&string1, 0},
        //{0x0302, 0x0409, (const uint8_t *)&string2, 0},
        //{0x0303, 0x0409, (const uint8_t *)&string3, 0},
	{0, 0, NULL, 0}
};


// **************************************************************
//   Endpoint Configuration
// **************************************************************

#if 0
// 0x00 = not used
// 0x19 = Recieve only
// 0x15 = Transmit only
// 0x1D = Transmit & Recieve
//
const uint8_t usb_endpoint_config_table[NUM_ENDPOINTS] =
{
	0x00, 0x15, 0x19, 0x15, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
#endif


const uint8_t usb_endpoint_config_table[NUM_ENDPOINTS] =
{
#if (defined(ENDPOINT1_CONFIG) && NUM_ENDPOINTS >= 1)
	ENDPOINT1_CONFIG,
#elif (NUM_ENDPOINTS >= 1)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT2_CONFIG) && NUM_ENDPOINTS >= 2)
	ENDPOINT2_CONFIG,
#elif (NUM_ENDPOINTS >= 2)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT3_CONFIG) && NUM_ENDPOINTS >= 3)
	ENDPOINT3_CONFIG,
#elif (NUM_ENDPOINTS >= 3)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT4_CONFIG) && NUM_ENDPOINTS >= 4)
	ENDPOINT4_CONFIG,
#elif (NUM_ENDPOINTS >= 4)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT5_CONFIG) && NUM_ENDPOINTS >= 5)
	ENDPOINT5_CONFIG,
#elif (NUM_ENDPOINTS >= 5)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT6_CONFIG) && NUM_ENDPOINTS >= 6)
	ENDPOINT6_CONFIG,
#elif (NUM_ENDPOINTS >= 6)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT7_CONFIG) && NUM_ENDPOINTS >= 7)
	ENDPOINT7_CONFIG,
#elif (NUM_ENDPOINTS >= 7)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT8_CONFIG) && NUM_ENDPOINTS >= 8)
	ENDPOINT8_CONFIG,
#elif (NUM_ENDPOINTS >= 8)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT9_CONFIG) && NUM_ENDPOINTS >= 9)
	ENDPOINT9_CONFIG,
#elif (NUM_ENDPOINTS >= 9)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT10_CONFIG) && NUM_ENDPOINTS >= 10)
	ENDPOINT10_CONFIG,
#elif (NUM_ENDPOINTS >= 10)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT11_CONFIG) && NUM_ENDPOINTS >= 11)
	ENDPOINT11_CONFIG,
#elif (NUM_ENDPOINTS >= 11)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT12_CONFIG) && NUM_ENDPOINTS >= 12)
	ENDPOINT12_CONFIG,
#elif (NUM_ENDPOINTS >= 12)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT13_CONFIG) && NUM_ENDPOINTS >= 13)
	ENDPOINT13_CONFIG,
#elif (NUM_ENDPOINTS >= 13)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT14_CONFIG) && NUM_ENDPOINTS >= 14)
	ENDPOINT14_CONFIG,
#elif (NUM_ENDPOINTS >= 14)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT15_CONFIG) && NUM_ENDPOINTS >= 15)
	ENDPOINT15_CONFIG,
#elif (NUM_ENDPOINTS >= 15)
	ENDPOINT_UNUSED,
#endif
};


#endif // NUM_ENDPOINTS
#endif // F_CPU >= 20 MHz
