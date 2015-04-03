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
#include <mcrypt.h>
#include <sys/stat.h>
#include <vector>

#include "Huffman.hpp"

//FIXME: does not work when the password is more than 8 characters.
//TODO: find a better way to check if the password of encrypted archive is wrong.
//TODO: fix memory issue.
//TODO: improve performance.

bool adjustFilename(std::string& filename);
std::string createKey(std::string const& password);
char* decompress(unsigned char* content, int size, std::size_t& decompressedSize);
std::string decrypt(std::string const& archiveFilename, std::string const& password);
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

std::string createKey(std::string const& password) {
    std::string key(password);
    while(key.size() < 8) {
        key.append(key);
    }
    key = key.substr(0, 8);
    return key;
}

char* decompress(unsigned char* content, int fileSize, std::size_t& decompressedSize) {
    char* bytes = new char[65520];

    int shiftingBytes{1};
    int16_t bytes2;
    char* bytes2Pointer = reinterpret_cast<char*>(&bytes2);
    unsigned int high{0};
    int low{0};
    int offset{0};
    unsigned int size{0};
    std::size_t currentIndex{0};

    for(int i{0} ; i < fileSize ; i++) {
        shiftingBytes >>= 1;
        if(0 == shiftingBytes) {
            shiftingBytes = 0x8000;
            bytes2Pointer[0] = content[i];
            bytes2Pointer[1] = content[i + 1];
            i += 2;
        }
        if(0 == (bytes2 & shiftingBytes)) {
            bytes[currentIndex] = content[i];
            currentIndex++;
        }
        else {
            high = content[i] >> 4;
            low = content[i] & 0xF;
            if(0 == high) {
                //Run-length decoding.
                size = low + 3;
                for(unsigned int j(0) ; j < size ; j++) {
                    bytes[currentIndex] = content[i + 1];
                    currentIndex++;
                }
            }
            else if(1 == high) {
                //Run-length decoding with bigger size.
                size = low + (content[i + 1] << 4) + 0x13;
                for(unsigned int j(0) ; j < size ; j++) {
                    bytes[currentIndex] = content[i + 2];
                    currentIndex++;
                }
                i++;
            }
            else {
                //Copy content already seen in the file (backward reference).
                //Get the offset and the length.
                offset = (content[i + 1] << 4) + low + 3;
                if(2 == high) {
                    size = content[i + 2] + 0x10;
                    i++;
                }
                else {
                    size = high;
                }
                std::size_t currentPosition{currentIndex};
                for(unsigned int j(0) ; j < size ; j++) {
                    bytes[currentIndex] = bytes[currentPosition - offset + j];
                    currentIndex++;
                }
            }
            i++;
        }
    }

    decompressedSize = currentIndex;
    return bytes;
}

std::string decrypt(std::string const& archiveFilename, std::string const& password) {
    std::string key{createKey(password)};
    char algorithm[]{"des"};
    char mode[]{"ecb"};
    MCRYPT td = mcrypt_module_open(algorithm, nullptr, mode, nullptr);
    if(MCRYPT_FAILED == td) {
        std::cerr << "Cannot open DES mcrypt module." << std::endl;
        return "";
    }
    char rawKey[8];
    std::copy(key.begin(), key.end(), rawKey);
    int result = mcrypt_generic_init(td, rawKey, 8, nullptr);
    if(result < 0) {
        mcrypt_perror(result);
        return "";
    }

    std::string decryptedFilename{archiveFilename};
    decryptedFilename.back() = 'v';

    std::ifstream inputFile{archiveFilename};
    std::ofstream outputFile{decryptedFilename};

    int const MAX_LENGTH{4096};
    char buffer[MAX_LENGTH];

    inputFile.read(buffer, MAX_LENGTH);
    std::streamsize length{inputFile.gcount()};
    mdecrypt_generic(td, buffer, length);
    if(8 == buffer[0]) {
        outputFile.write(buffer, length);
    }
    else {
        std::cerr << "Wrong password." << std::endl;
        return "";
    }

    while(not inputFile.eof()) {
        inputFile.read(buffer, MAX_LENGTH);
        length = inputFile.gcount();
        mdecrypt_generic(td, buffer, length);
        outputFile.write(buffer, length);
    }

    mcrypt_generic_end(td);
    inputFile.close();
    outputFile.close();
    return decryptedFilename;
}

std::string getFileDirectory(std::string const& filename) {
    std::size_t index(filename.rfind('/'));
    if(filename.npos != index) {
        return filename.substr(0, index);
    }
    return filename;
}

void mkdirTree(std::string const& directory) {
    int index{-1};
    do {
        index++;
        index = directory.find('/', index);
        mkdir(directory.substr(0, index).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    } while(index != std::string::npos);
}

void unarchive(std::string const& archiveFilename) {
    std::string decryptedFilename;
    if(archiveFilename.substr(archiveFilename.rfind(".") + 1) == "cbz") {
        std::cout << "Password:" << std::endl;
        std::string password;
        std::cin >> password;
        decryptedFilename = decrypt(archiveFilename, password);
    }
    else {
        decryptedFilename = archiveFilename;
    }
    std::ifstream file(decryptedFilename, std::ios_base::in | std::ios_base::binary);
    if(not file) {
        std::cout << "Failed to read file." << std::endl;
    }
    else {
        uint8_t bytes8[8];
        file.read(reinterpret_cast<char*>(bytes8), 0x8);
        unsigned int fileCount = (bytes8[3] << 8) + bytes8[2];
        unsigned int fileBlock = bytes8[4];
        char bytesFilename[fileBlock];
        std::vector<std::string> filenames;
        std::vector<int64_t> fileSizes;
        for(unsigned int i(0) ; i < fileCount ; i++) {
            file.read(bytesFilename, fileBlock);

            //Get the total decompressed file size of the current file.
            int32_t* fileSizePointer(reinterpret_cast<int32_t*>(bytesFilename + 136));
            
            fileSizes.push_back(*fileSizePointer);

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
            std::size_t totalFileSize(fileSizes.at(i));
            std::ofstream outputFile(filenames.at(i));
            for(std::size_t currentPosition(0) ; currentPosition < totalFileSize ; ) {
                uint16_t bytes2{0};
                file.read(reinterpret_cast<char*>(&bytes2), 0x2);
                unsigned int fileSize = bytes2;

                file.read(reinterpret_cast<char*>(&bytes2), 0x2); //TODO: unknown bytes.

                unsigned char* fileContent = new unsigned char[fileSize];
                file.read(reinterpret_cast<char*>(fileContent), fileSize);

                std::size_t decompressedSize(0);
                char* content(nullptr);
                unsigned char flags = fileContent[0];
                int delta{1};
                bool huffmanEncoded{0 != (flags & 2)};
                if(huffmanEncoded) {
                    delta = 0;
                    uint8_t byte{fileContent[1]};
                    uint16_t blockDecompressedSize = byte << 8;
                    byte = fileContent[2];
                    blockDecompressedSize += byte;
                    Huffman huffman(fileContent + 3, blockDecompressedSize);
                    unsigned char* decodedContent = huffman.decode();
                    delete[] fileContent;
                    fileContent = decodedContent;
                    fileSize = blockDecompressedSize;
                }
                if(0 != (flags & 1)) {
                    //The file is compressed.
                    content = decompress(fileContent + delta, fileSize - delta, decompressedSize);
                    outputFile.write(content, decompressedSize);
                    delete[] content;
                }
                else {
                    //The file is not compressed.
                    if(huffmanEncoded) {
                        content = reinterpret_cast<char*>(fileContent);
                        decompressedSize = fileSize;
                    }
                    else {
                        content = reinterpret_cast<char*>(fileContent + 1);
                        decompressedSize = fileSize - 1;
                    }
                    outputFile.write(content, decompressedSize);
                }

                delete[] fileContent;
                currentPosition += decompressedSize;
            }
            outputFile.close();
        }
    }
    file.close();
}
