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

#include "Debouncer.h"
#include "Arduino.h"

Debouncer::Debouncer()
    : previous_millis(0)
    , interval_millis(50)
    , value(0)
{}

void Debouncer::interval(uint16_t interval_millis)
{
    this->interval_millis = interval_millis;
}

uint8_t Debouncer::get(uint8_t current_value)
{
    if(current_value != value) {
        if ( millis() - previous_millis >= interval_millis ) {
            previous_millis = millis();
            setValue(current_value);
        }
    }

    return value;
}