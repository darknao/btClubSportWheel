# btClubSportWheel
ClubSportWheel Bluetooth Controller

This firmware can convert any Fanatec CSW Steering Wheel to a standalone USB version, so you can use it with any other wheel base, like an OpenSimWheel (OSW).

It'll also be possible to add a bluetooth module to go wireless and get rid of the USB cable (but this is not implemented yet).

## Supported Hardware
This firmware has been developped and tested on a Teensy 3.1 but should work with any compatible hardware.
The only requirement so far, is at least 1 SPI connectivity available.

The Teensy-LC may be a good choice for this project.

### Shopping List
- Fanatec CSW wheel (obviously)
- [Teensy LC](https://www.pjrc.com/teensy/teensyLC.html) : ~12$
- [Bluegiga WT12 module](https://www.bluegiga.com/en-US/products/wt12-bluetooth--class-2-module/) for Bluetooth connectivity [optional] : ~20$

### Pinout schematics
TBD

## Compatible CSW Wheel
All CSW Wheel should be compatible, but only the Porsche 918 RSR Wheel have been tested so far.

## Sofware/Driver Required
This appear has a HID device on Windows, so no specific drivers are required.

It's is fully compatible with Fanaleds, which is required to get all feedback indicator working (LEDs, display and motors).

## Supported CSW wheel features
- All buttons (12 including the small tuning button)
- 2 paddle shifter
- Analog joystick (2 axis)
- 7-way encoder switch (4 way hat + rotary encoder + push button)
- LED bar (revlights) [Fanaleds]
- LED display [Fanaleds]
- Rumble motors [Fanaleds]

## Building Instruction
This code is based on Teensyduino framework and all required library is included in this repo.

All you need is a arm compiler (apt-get install gcc-arm-none-eabi on ubuntu).

Use 'make' to build the HEX file, then use the Teensy loader to flash the firmware.

