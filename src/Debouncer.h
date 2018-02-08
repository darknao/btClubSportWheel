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

#ifndef _DEBOUNCER_H_
#define _DEBOUNCER_H_

#include <inttypes.h>

class Debouncer
{
  public:
    Debouncer();

    void interval(uint16_t interval_millis);
    uint8_t get(uint8_t value);

  protected:
    unsigned long previous_millis;
    uint16_t interval_millis;
    uint8_t value;

  private:
    inline void setValue(uint8_t value) {this->value = value;}
};

#endif