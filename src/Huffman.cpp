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

#include <vector>

#include "Huffman.hpp"

Huffman::Huffman(unsigned char* initialData, int blockDecompressedSize) : bitArray(initialData), blockLength(blockDecompressedSize) {
}

Huffman::~Huffman() {
    delete tree;
}

void Huffman::addNode(uint8_t value, int length, uint16_t bits) {
    if(length > 0) {
        Node* previous = tree;
        Node* node = tree;
        bits <<= 16 - length;
        for(int i{0} ; i < length ; i++) {
            if(0 == (bits & MASK)) {
                node = node->left;
                if(nullptr == node) {
                    node = new Node{};
                    previous->left = node;
                }
            }
            else {
                node = node->right;
                if(nullptr == node) {
                    node = new Node{};
                    previous->right = node;
                }
            }
            previous = node;
            bits <<= 1;
        }
        node->value = value;
    }
}

unsigned char* Huffman::decode() {
    unsigned char* bytes = new unsigned char[65520];
    createTree();
    Node* node = tree;
    uint8_t bit{0};
    std::size_t currentIndex{0};
    for(int end{blockLength * 8} ; position < end ; ) {
        node = tree;
        do {
            bit = getBits(1);
            if(0 == bit) {
                node = node->left;
            }
            else {
                node = node->right;
            }
        } while(nullptr != node->left || nullptr != node->right);
        bytes[currentIndex] = node->value;
        currentIndex++;
    }

    return bytes;
}

void Huffman::createTree() {
    tree = new Node{};
    int length{0};
    uint16_t bits{0};

    int count{0};

    for(uint16_t i{0} ; i < 256 ; i++) {
        length = getBits(4);
        bits = getBits(length);
        addNode(static_cast<uint8_t>(i), length, bits);
        if(length > 0) {
            count++;
        }
    }
}

uint16_t Huffman::getBits(int count) {
    //TODO: improve this to get more than one bit at once.
    //TODO: there is an invalid read of size 1 to fix.
    uint16_t bits{0};
    int i{0};
    for( ; i < count - 1 ; i++) {
        bits += bitArray[position + i];
        bits <<= 1;
    }
    bits += bitArray[position + i];
    position += count;
    return bits;
}
