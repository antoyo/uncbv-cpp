#include <fstream>
#include <iconv.h>
#include <iostream>
#include <vector>

char* decompress(unsigned char* content, int size, std::size_t& decompressedSize);

int main(int argc, char* argv[]) {
    if(argc < 2) {
        std::cerr << "Usage: uncbv file [file...]" << std::endl;
    }
    else {
        std::ifstream file(argv[1], std::ios_base::in | std::ios_base::binary);
        if(not file) {
            std::cout << "Failed to read file." << std::endl;
        }
        else {
            // 0x9F0 octets.
            uint8_t bytes8[8];
            file.read(reinterpret_cast<char*>(bytes8), 0x8);
            unsigned int fileCount = bytes8[2];
            unsigned int fileBlock = bytes8[4];
            //std::cout << "File count: " << fileCount << std::endl;
            //std::cout << "File block: " << fileBlock << std::endl;
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
            }

            for(unsigned int i(0) ; i < fileCount ; i++) {
                uint16_t bytes2[1];
                file.read(reinterpret_cast<char*>(bytes2), 0x2);
                unsigned int fileSize = bytes2[0];
                //std::cout << "File size: " << fileSize << std::endl;

                file.read(reinterpret_cast<char*>(bytes2), 0x2);
                std::cout << "Unknown byte (" << filenames.at(i) << "): " << bytes2[0] << std::endl;
                uint8_t* bytes2times = reinterpret_cast<uint8_t*>(bytes2);
                printf("0x%x 0x%x\n", bytes2times[0], bytes2times[1]);

                unsigned char* fileContent = new unsigned char[fileSize];
                file.read(reinterpret_cast<char*>(fileContent), fileSize);
                //std::cout << fileContent << std::endl;

                std::ofstream outputFile(filenames.at(i));
                std::size_t decompressedSize(0);
                char* content(nullptr);
                if(0 == fileContent[0]) {
                    content = reinterpret_cast<char*>(fileContent + 1);
                    decompressedSize = fileSize - 1;
                }
                else {
                    content = decompress(fileContent, fileSize, decompressedSize);
                    delete[] content;
                }
                outputFile.write(content, decompressedSize);
                outputFile.close();

                delete[] fileContent;
            }
        }
        file.close();
    }
    return 0;
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

    std::cout << "Unused byte: " << int(content[0]) << std::endl;
    for(int i(1) ; i < fileSize ; i++) {
        shiftingBytes >>= 1;
        if(0 == shiftingBytes) {
            shiftingBytes = 0x8000;
            bytes2Pointer[0] = content[i];
            bytes2Pointer[1] = content[i + 1];
            i += 2;
        }
        if((bytes2 & shiftingBytes) == 0) {
            //printf("0x%x 0x%x\n", bytes2, shiftingBytes);
            bytes.push_back(content[i]);
            //std::cout << "Copy " << content[i] << std::endl;
        }
        else {
            //printf("0x%x\n", content[i]);
            high = content[i] >> 4;
            //printf("0x%x\n", high);
            low = content[i] & 0xF;
            //printf("0x%x\n", low);
            if(0 == high) {
                //Run-length decoding.
                //std::cout << "Run-length." << std::endl;
                size = low + 3;
                //std::cout << "Size: " << size << std::endl;
                //std::cout << "Byte: " << content[i + 1] << std::endl;
                for(unsigned int j(0) ; j < size ; j++) {
                    bytes.push_back(content[i + 1]);
                }
            }
            else {
                //Copy content already seen in the file.
                //Get the offset and the length.
                //std::cout << "Compressed." << std::endl;
                offset = (content[i + 1] << 4) + low + 3;
                if(2 == high) {
                    size = content[i + 2] + 0x10;
                    i++;
                }
                else {
                    size = high;
                }
                //std::cout << "Offset " << offset << std::endl;
                //std::cout << "Size " << size << std::endl;
                /*if(offset == 96) {
                    char* rawBytes = new char[bytes.size()];
                    std::copy(bytes.begin(), bytes.end(), rawBytes);
                    std::cout << rawBytes << std::endl;
                    delete[] rawBytes;
                }*/
                currentPosition = bytes.size();
                for(unsigned int j(0) ; j < size ; j++) {
                    bytes.push_back(bytes[currentPosition - offset + j]);
                    //std::cout << "Compressed copy " << bytes[currentPosition - offset + j] << std::endl;
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
