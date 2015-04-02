/*
 * Copyright (C) 2015  Boucher, Antoni <bouanto@gmail.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#include "BitArray.hpp"

BitArray::BitArray(unsigned char* initialData) : data(initialData) {
}

uint8_t BitArray::operator[](int index) {
    int i{index / 8};
    int shift{index % 8};
    uint8_t bit{data[i]};
    bit <<= shift;
    bit >>= 7;
    return bit;
}
