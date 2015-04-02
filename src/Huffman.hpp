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

#ifndef DEF_HUFFMAN
#define DEF_HUFFMAN

#include <algorithm>
#include <bitset>
#include <cstdint>

#include "BitArray.hpp"

struct Node {
    Node* left = nullptr;
    Node* right = nullptr;
    char value = 0;

    ~Node() {
        delete left;
        delete right;
    }
};

class Huffman {
    private:
        BitArray bitArray = nullptr;
        int blockLength = 0;
        int position = 0;
        Node* tree = nullptr;

        uint16_t const MASK = 0b1000000000000000;

    public:
        Huffman(unsigned char* initialData, int compressedBlockLength);
        
        Huffman(Huffman const&) = delete;

        ~Huffman();

        Huffman& operator=(Huffman const&) = delete;

        unsigned char* decode();

    private:
        void addNode(uint8_t value, int length, uint16_t bits);

        void copyBits(uint16_t& bits, uint8_t byte, int count);

        void createTree();

        uint16_t getBits(int count);
};

#endif
