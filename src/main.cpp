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

#include <fstream>
#include <iconv.h>
#include <iostream>
#include <sys/stat.h>
#include <vector>

bool adjustFilename(std::string& filename);
char* decompress(unsigned char* content, int size, std::size_t& decompressedSize);
std::string getFileDirectory(std::string const& filename);
void mkdirTree(std::string const& directory);
void unarchive(std::string const& archiveFilename);

int main(int argc, char* argv[]) {
    if(argc < 2) {
        std::cerr << "Usage: uncbv file [file...]" << std::endl;
    }
    else {
        int argumentCount(argc - 1);
        for(int i(0) ; i < argumentCount ; i++) {
            unarchive(argv[1 + i]);
        }
    }
    return 0;
}

bool adjustFilename(std::string& filename) {
    std::size_t index(0);
    bool haveBackslash(false);
    bool createDirectory(false);
    do {
        index = filename.find('\\');
        haveBackslash = index != filename.npos;
        if(haveBackslash) {
            createDirectory = true;
            filename[index] = '/';
        }
    } while(haveBackslash);
    return createDirectory;
}

char* decompress(unsigned char* content, int fileSize, std::size_t& decompressedSize) {
    std::vector<char> bytes;

    int shiftingBytes(1);
    int16_t bytes2;
    char* bytes2Pointer = reinterpret_cast<char*>(&bytes2);
    unsigned int high(0);
    int low(0);
    int offset(0);
    unsigned int size(0);
    std::size_t currentPosition(0);

    for(int i(1) ; i < fileSize ; i++) {
        shiftingBytes >>= 1;
        if(0 == shiftingBytes) {
            shiftingBytes = 0x8000;
            bytes2Pointer[0] = content[i];
            bytes2Pointer[1] = content[i + 1];
            i += 2;
        }
        if(0 == (bytes2 & shiftingBytes)) {
            bytes.push_back(content[i]);
        }
        else {
            high = content[i] >> 4;
            low = content[i] & 0xF;
            if(0 == high) {
                //Run-length decoding.
                size = low + 3;
                for(unsigned int j(0) ; j < size ; j++) {
                    bytes.push_back(content[i + 1]);
                }
            }
            else if(1 == high) {
                //Run-length decoding with bigger size.
                size = low + (content[i + 1] << 4) + 0x13;
                for(unsigned int j(0) ; j < size ; j++) {
                    bytes.push_back(content[i + 2]);
                }
                i++;
            }
            else {
                //Copy content already seen in the file.
                //Get the offset and the length.
                offset = (content[i + 1] << 4) + low + 3;
                if(2 == high) {
                    size = content[i + 2] + 0x10;
                    i++;
                }
                else {
                    size = high;
                }
                currentPosition = bytes.size();
                for(unsigned int j(0) ; j < size ; j++) {
                    bytes.push_back(bytes[currentPosition - offset + j]);
                }
            }
            i++;
        }
    }

    char* rawBytes = new char[bytes.size()];
    std::copy(bytes.begin(), bytes.end(), rawBytes);
    decompressedSize = bytes.size();
    return rawBytes;
}

std::string getFileDirectory(std::string const& filename) {
    std::size_t index(filename.rfind('/'));
    if(filename.npos != index) {
        return filename.substr(0, index);
    }
    return filename;
}

void mkdirTree(std::string const& directory) {
    //TODO: create the subdirectories if needed.
    mkdir(directory.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

void unarchive(std::string const& archiveFilename) {
    std::ifstream file(archiveFilename, std::ios_base::in | std::ios_base::binary);
    if(not file) {
        std::cout << "Failed to read file." << std::endl;
    }
    else {
        uint8_t bytes8[8];
        file.read(reinterpret_cast<char*>(bytes8), 0x8);
        unsigned int fileCount = bytes8[2];
        unsigned int fileBlock = bytes8[4];
        char bytesFilename[fileBlock];
        std::vector<std::string> filenames;
        for(unsigned int i(0) ; i < fileCount ; i++) {
            file.read(bytesFilename, fileBlock);
            //The filename is encoded in ISO-8859-1. A conversion to UTF-8 is needed.
            char filename[fileBlock];
            iconv_t converter = iconv_open("UTF-8", "ISO-8859-1");
            char* inBuffer(bytesFilename);
            std::size_t inSize = fileBlock;
            char* outBuffer(filename);
            std::size_t outSize = fileBlock * 2;
            iconv(converter, &inBuffer, &inSize, &outBuffer, &outSize);
            iconv_close(converter);
            filenames.push_back(filename);
            std::string& lastFilename = filenames.back();
            if(adjustFilename(lastFilename)) {
                mkdirTree(getFileDirectory(lastFilename));
            }
        }

        for(unsigned int i(0) ; i < fileCount ; i++) {
            uint16_t bytes2[1];
            file.read(reinterpret_cast<char*>(bytes2), 0x2);
            unsigned int fileSize = bytes2[0];

            file.read(reinterpret_cast<char*>(bytes2), 0x2);
            std::cout << "Unknown byte (" << filenames.at(i) << "): " << bytes2[0] << std::endl;

            unsigned char* fileContent = new unsigned char[fileSize];
            file.read(reinterpret_cast<char*>(fileContent), fileSize);

            std::ofstream outputFile(filenames.at(i));
            std::size_t decompressedSize(0);
            char* content(nullptr);
            if(0 != (fileContent[0] & 2)) {
                std::cout << "What to do?" << std::endl;
                continue;
            }
            if(0 == fileContent[0]) {
                //The file is not compressed.
                content = reinterpret_cast<char*>(fileContent + 1);
                decompressedSize = fileSize - 1;
            }
            else if(1 == fileContent[0]) {
                //The file is compressed.
                content = decompress(fileContent, fileSize, decompressedSize);
                delete[] content;
            }
            else {
                std::cout << "Error: " << int(fileContent[0]) << std::endl;
            }
            outputFile.write(content, decompressedSize);
            outputFile.close();

            delete[] fileContent;
        }
    }
    file.close();
}
