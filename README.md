# btClubSportWheel
ClubSportWheel Bluetooth Controller

This firmware can convert any Fanatec CSW Steering Wheel to a **standalone USB version**, so you can use it with any other wheel base, like an OpenSimWheel (OSW).

It'll also be possible to add a bluetooth module to go wireless and get rid of the USB cable (Work in progress).

## Supported Hardware
This firmware has been developped and tested on a **Teensy 3.1** and **Teensy LC** but should work with any other compatible hardware.  
The only requirement so far, is at least 1 SPI connectivity available.

The Teensy-LC is a good choice for this project as it has a small size & price.

### Shopping List
#### USB version
- Fanatec CSW rim (obviously)
- [Teensy LC](https://www.pjrc.com/teensy/teensyLC.html) : **~12$**

#### Bluetooth upgrade
- [Bluegiga WT12 module](http://www.jetney.com/wt12) : **~25$**
- [Adafruit PowerBoost 500 Charger](https://www.adafruit.com/products/1944) : **~15$**
- [LiPo Battery (>=1000mA)](https://www.adafruit.com/categories/138) : **~10$**

Total price for full BT conversion : **~60$**
### Pinout & schematics
#### Fanatec Plug
![Fanatec Round Plug](http://i.imgur.com/yLSG0Jsm.jpg)

1. MISO 
2. MOSI 
3. GND 
4. 5v 
5. GND 
6. SCK 
7. 3v3 
8. CS 

#### Teensy pinout
![Teensy pinout](http://i.imgur.com/5yfoka2.png)

## Compatible CSW Wheel
Formula, BMW M3 GT2 and Porsche 918 rims are confirmed working with this conversion.  
The Universal Hub may also work, but it has not been tested for now.

## Sofware/Driver Required
This is detected has a HID device on Windows, so no specific drivers are required.  
It's is fully compatible with Fanaleds, which is required to get all feedback indicator working (LEDs, display and motors).

## Supported CSW wheel features
- All buttons (12 including the small tuning button)
- 2 paddle shifter
- Analog joystick (2 axis)
- 7-way encoder switch (4 way hat + rotary encoder + push button)
- LED bar (revlights) [Fanaleds]
- LED display [Fanaleds]
- Rumble motors [Fanaleds]

## Precompiled firmware
You'll need the [Teensy Loader](https://www.pjrc.com/teensy/loader.html) to flash these firmwares.

**Teensy 3.1 & 3.2** : [`USB`](firmware/teensy31_USB.hex) | `Bluetooth` (not ready yet)  
**Teensy LC** : [`USB`](firmware/teensyLC_USB.hex) | `Bluetooth` (not ready yet)

## Building Instruction
This code is based on Teensyduino framework and all required library is included in this repo. 
All you need is a arm compiler (apt-get install gcc-arm-none-eabi on ubuntu).  
Use `make` to build the HEX file, then use the Teensy loader to flash the firmware.

