#!/usr/bin/python
# -*- coding: UTF-8 -*-
"""
This script read data comming from a fanatec wheel.
Copyright (C) 2015 darknao
https://github.com/darknao/btClubSportWheel

This file is part of btClubSportWheel.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.


Fanatec Pinout:

     1   2          1.MISO    8.CS
     O   O          2.MOSI    9.
 3 O  O O  O 6      3.GND    10.
7 O   4 5   O 8     4.5v     11.
 9 O       O 10     5.GND    12.
  11 O   O 12       6.SCK    13.
    13 O            7.3v3


Porsche/BMW/Formula connector pinout:

--
|-- 1 5v        4
|-- 2 3v3       7
|-- 3 GND       3
|-- 4 GND       5
|-- 5 MISO      1
|-- 6 MOSI      2
|-- 7 SCK       6
|-- 8 CS        8
--


Fanatec Packet:

headers
 ^  wheel ID?
 |  ^  display
 |  |    ^    revlights
 |  |    |        ^  rumbles
 |  |    |        |     ^                              nothing                                  CRC8
|__|__|________|_____|_____|____________________________________________________________________|__|
 a5 03 FF FF FF FF 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 XX
 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32

display:
1 byte for 7segments + dot

      1
   _______
  |       |
6 |       | 2
  |   7   |
  |-------|
  |       | 3
5 |       |
  |_______| o 8
      4


revlights:
9 leds = 9 bits

    FF       01
1111 1111 0000 0001
  2 to 9          1

rumbles:
1 byte per motor (left & right)

Few samples (incoming packet):

01:14:00 0xa5  0x3  0x0  0x0  0x0  0x0  0x0  0x0 0x48 0x83 0x87 0x43 0x5b 0x9a 0x39 0xa0 0xc9 0xb5 0xd8 0x19 0x72 0x30 0x28 0xfa 0x62 0xf7 0x93  0xc 0xcb 0x98 0xd0 0x12 0x42
01:14:03 0xa5  0x3  0x0 0x80  0x0  0x0  0x0  0x0 0x48 0x83 0x87 0x43 0x5b 0x9a 0x39 0xa0 0xc9 0xb5 0xd8 0x19 0x72 0x30 0x28 0xfa 0x62 0xf7 0x93  0xc 0xcb 0x98 0xd0 0x12 0x58
01:14:04 0xa5  0x3  0x0  0x0  0x0  0x0  0x0  0x0 0x48 0x83 0x87 0x43 0x5b 0x9a 0x39 0xa0 0xc9 0xb5 0xd8 0x19 0x72 0x30 0x28 0xfa 0x62 0xf7 0x93  0xc 0xcb 0x98 0xd0 0x12 0x42
01:14:04 0xa5  0x3 0x10  0x0  0x0  0x0  0x0  0x0 0x48 0x83 0x87 0x43 0x5b 0x9a 0x39 0xa0 0xc9 0xb5 0xd8 0x19 0x72 0x30 0x28 0xfa 0x62 0xf7 0x93  0xc 0xcb 0x98 0xd0 0x12 0x3b
01:14:04 0xa5  0x3  0x0  0x0  0x0  0x0  0x0  0x0 0x48 0x83 0x87 0x43 0x5b 0x9a 0x39 0xa0 0xc9 0xb5 0xd8 0x19 0x72 0x30 0x28 0xfa 0x62 0xf7 0x93  0xc 0xcb 0x98 0xd0 0x12 0x42
"""

import spidev
import time
import random
from bitstring import BitArray
import crcmod


def hexf(byte):
    """ Convert byte to hex string """
    return "%4s" % hex(byte)


def printhex(listOfBytes, compare=None):
    """ Print a list of bytes,
        and optionnaly, show the diff with another list of bytes (compare)"""
    newBytes = map(hexf, listOfBytes)
    if compare is not None:
        if len(compare) == len(listOfBytes):
            for idx in range(len(listOfBytes)):
                if listOfBytes[idx] != compare[idx]:
                    newBytes[idx] = green(newBytes[idx])
        else:
            newBytes = map(green, newBytes)
    hexstring = " ".join(newBytes)
    print '%s %s %s' % (time.strftime('%8X'), hexstring, checkcrc(listOfBytes))


def printascii(listOfBytes):
    """ Print the ascii representation (if possible) of a list of bytes """
    print repr("".join(map(chr, listOfBytes)))


def green(string):
    """ colorize the string in green """
    return "\033[92m%s\033[0m" % string


""" Convert string to hex """
s2h = lambda x: int(x, 16)


def crcs(stri):
    """ return the CRC of a string of hex bytes """
    return crc(map(s2h, stri.split()))


def crc(bytes):
    """ return the CRC of a list of bytes """
    f = crcmod.mkCrcFun(0x131)
    return f("".join(map(chr, bytes)))


def checkcrc(bytes):
    CRC = crc(bytes[:-1])
    if CRC == bytes[-1]:
        return True
    else:
        return False


def buildcrc(data):
    """ Put the CRC at the end of a list of bytes """
    crcbyte = crc(data[:-1])
    data[-1] = crcbyte
    return data


def send_data(data):
    """ Send a fanatec packet to SPI """
    data = buildcrc(data)
    spi.writebytes(data)


if __name__ == '__main__':
    spi = spidev.SpiDev(0, 0)

    oldbytes = []

    # prebuild a fanatec packet
    dataout = [0 for i in range(32)]
    dataout[0] = 0xa5
    dataout[1] = 0x03

    # shutdown everything (leds, display and rumble)
    send_data(dataout)

    while True:
        bytes = spi.readbytes(34)
        raw = b"".join(map(chr, bytes))
        barray = BitArray(bytes=raw)
        barray_s = barray << 1
        # unkbit = barray[0] >> 7 & 0x1
        data_br = barray_s[:-8]
        data = map(ord, data_br.bytes)
        # Print only if something has changed
        if cmp(oldbytes,  data):
            printhex(data, oldbytes)
            oldbytes = data
