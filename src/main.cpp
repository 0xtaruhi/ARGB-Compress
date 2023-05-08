/* Compress and Decompress image data
*/
#include <cstring>
#include <iostream>
#include <fstream>
#include <math.h>
#include "defines.h"
#include "rgbTileProc.h"

#define APP_VERSION     "1.0.0"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

typedef struct _TileCompressionInfo {
    int tilePosition;
    int tileSize;
} TileCompressionInfo;

/*
* compress ARGB data to TILE
*/
int compressARGB(char const * inFileName, char const * outFileName) {
    int     ret = ERROR_OK;
    int     width, height, nrChannels;
    unsigned char *data = stbi_load(inFileName, &width, &height, &nrChannels, STBI_rgb_alpha);
    if (data == NULL) {
        std::cout << "cannot open file: " << inFileName << std::endl;
        return ERROR_INPUT_FILE;
    }
    std::cout << "image info: width = " << width 
        << ", height = " << height
        << ", channels = " << nrChannels
        << std::endl;

    const int TILE_WIDTH = 8;
    const int TILE_HEIGHT = 8;
    int numRows = height / TILE_HEIGHT;
    int numColumns = width / TILE_WIDTH;
    const int BYTES_PER_PIXEL = 4;
    int rowStride = width * BYTES_PER_PIXEL; // 4 bytes per pixel
    unsigned char pARGB[TILE_WIDTH * TILE_HEIGHT * BYTES_PER_PIXEL] = { 0u };
    unsigned char pCompressed[TILE_WIDTH * TILE_HEIGHT * BYTES_PER_PIXEL] = { 0u };

    tileSetSize(TILE_WIDTH, TILE_HEIGHT);

    unsigned char * pCompressionBuffer = new unsigned char [numRows * numColumns * TILE_WIDTH * TILE_HEIGHT * BYTES_PER_PIXEL];
    if (NULL == pCompressionBuffer) {
        return ERROR_CUSTOM;
    }
    TileCompressionInfo *pTCInfos = new TileCompressionInfo [numRows * numColumns];
    if (NULL == pTCInfos) {
        delete[] pCompressionBuffer;
        return ERROR_CUSTOM;
    }

    int tileRowIndex = 0;
    int tileColumnIndex = 0;
    int totalBitsAfterCompression = 0;
    int posInCompressionBuffer = 0;
    unsigned char* pClr = NULL;

    for (tileRowIndex = 0; tileRowIndex < numRows; tileRowIndex++) {
        for (tileColumnIndex = 0; tileColumnIndex < numColumns; tileColumnIndex++) {
            int tileIndex = tileRowIndex * numColumns + tileColumnIndex;
            pClr = pARGB;
            for (int i = 0; i < TILE_HEIGHT; i++) {
                for (int j = 0; j < TILE_WIDTH; j++) {
                    int row = tileRowIndex * TILE_HEIGHT + i;
                    int col = tileColumnIndex * TILE_WIDTH + j;
                    int pixelDataOffset = rowStride * row + col * BYTES_PER_PIXEL;
                    pClr[0] = data[pixelDataOffset];     // b
                    pClr[1] = data[pixelDataOffset + 1]; // g
                    pClr[2] = data[pixelDataOffset + 2]; // r
                    pClr[3] = data[pixelDataOffset + 3]; // a
                    pClr += 4;
                }
            }
            pTCInfos[tileIndex].tilePosition = posInCompressionBuffer;

            // compress
            argb2tile(pARGB, pCompressionBuffer + posInCompressionBuffer, &pTCInfos[tileIndex].tileSize);
            posInCompressionBuffer += pTCInfos[tileIndex].tileSize;
        }
    }
    std::cout << "compression ratio = " << (float)posInCompressionBuffer / (float)(width * height * BYTES_PER_PIXEL) * 100 << "%" << std::endl;

    // save compressed data to JLCD file
    std::ofstream ofs;
    ofs.open(outFileName, std::ios::binary | std::ios::out);

    int tileCount = numRows * numColumns;
    int fileHeaderSize = 24;
    int tileInfoSize = tileCount * 8;
    int dataOffsetInFile = fileHeaderSize + tileInfoSize;
    if (ofs.is_open()) {
        ofs.write("JLCD", 4);
        ofs.write(reinterpret_cast<const char *>(&width), 4);
        ofs.write(reinterpret_cast<const char *>(&height), 4);
        ofs.write(reinterpret_cast<const char *>(&TILE_WIDTH), 4);
        ofs.write(reinterpret_cast<const char *>(&TILE_HEIGHT), 4);
        ofs.write(reinterpret_cast<const char *>(&tileCount), 4);
        // tile data offset + len
        for (tileRowIndex = 0; tileRowIndex < numRows; tileRowIndex++) {
            for (tileColumnIndex = 0; tileColumnIndex < numColumns; tileColumnIndex++) {
                int tileIndex = tileRowIndex * numColumns + tileColumnIndex;
                ofs.write(reinterpret_cast<const char *>(&pTCInfos[tileIndex].tilePosition), 4);
                ofs.write(reinterpret_cast<const char *>(&pTCInfos[tileIndex].tileSize), 4);
            }
        }
        ofs.flush();
        // all tile data
        ofs.write(reinterpret_cast<const char *>(pCompressionBuffer), posInCompressionBuffer);
        ofs.close();
    } else {
        std::cout << "fail to open output file(" << outFileName << ")" << std::endl;
    }

    stbi_image_free(data);
    delete [] pCompressionBuffer;
    delete [] pTCInfos;
    return ERROR_OK;
}

/*
* decompress TILE data to ARGB
*/
int decompressARGB(char const * compressedFileName, char const * outFileName) {
    std::ifstream ifs;
    ifs.open(compressedFileName, std::ios::binary | std::ios::in);

    if (!ifs.is_open()) {
        std::cout << "fail to open output file: " << compressedFileName << std::endl;
        return ERROR_OUTPUT_FILE;
    }

    char readBuffer[1024];
    // read "JLCD"
    ifs.read(readBuffer, 4);
    if (strncmp(readBuffer, "JLCD", 4) != 0) {
        ifs.close();
        std::cout << "ERROR: INVALID tile file: " << compressedFileName << std::endl;
        return ERROR_INVALID_INPUT_FILE;
    }

    const int BYTES_PER_PIXEL = 4;
    int imgWidth, imgHeight, tileWidth, tileHeight, tileCount;
    // read image width
    ifs.read(reinterpret_cast<char*>(&imgWidth), 4);
    ifs.read(reinterpret_cast<char*>(&imgHeight), 4);
    ifs.read(reinterpret_cast<char*>(&tileWidth), 4);
    ifs.read(reinterpret_cast<char*>(&tileHeight), 4);
    ifs.read(reinterpret_cast<char*>(&tileCount), 4);

    std::cout << "imgWidth = " << imgWidth 
        << ", imgHeight = " << imgHeight
        << ", tileWidth = " << tileWidth
        << ", tileHeight = " << tileHeight
        << std::endl;

    int tileRowCount = imgHeight / tileHeight;
    int tileColumnCount = imgWidth / tileWidth;
    int tileDataStartPos = 24 + 8 * tileCount;
    unsigned char *pDecompressedARGB = new unsigned char [imgWidth * imgHeight * 4];
    unsigned char pTempDecompressionBuffer[1024];

    for (int row = 0; row < tileRowCount; row++) {
        for (int col = 0; col < tileColumnCount; col++) {
            int tileIndex = row * tileColumnCount + col;
            int tileInfoOffset = 24 + 8 * tileIndex;
            int tileDataOffset = 0;
            int tileDataBytes = 0;
            ifs.seekg(tileInfoOffset);
            ifs.read(reinterpret_cast<char *>(&tileDataOffset), 4);
            ifs.read(reinterpret_cast<char *>(&tileDataBytes), 4);
			ifs.seekg(tileDataStartPos + tileDataOffset);
            ifs.read(readBuffer, tileDataBytes);
            // decompress
            tile2argb((unsigned char*)readBuffer, tileDataBytes, pTempDecompressionBuffer);

            for (int i = 0; i < tileHeight; i++) {
                for (int j = 0; j < tileWidth; j++) {
                    int globalRow = row * tileHeight + i;
                    int globalCol = col * tileWidth + j;
                    int indexInTile = i * tileWidth + j;
                    memcpy(&pDecompressedARGB[(globalRow * imgWidth + globalCol) * BYTES_PER_PIXEL],
                            &pTempDecompressionBuffer[indexInTile * BYTES_PER_PIXEL],
                            4);
                }
            }
        }
    }
    ifs.close();

    // save decompressed image to output file
    stbi_write_bmp(outFileName, imgWidth, imgHeight, STBI_rgb_alpha, reinterpret_cast<char const *>(pDecompressedARGB));

    delete[] pDecompressedARGB;
    return ERROR_OK;
}

/*
* compare two bmp files
*/
int compareBMP(char const* file1, char const* file2)
{
	int		ret = ERROR_OK;
    int     width, height, nrChannels;
    int     w2, h2, ch2;
    unsigned char* data1 = stbi_load(file1, &width, &height, &nrChannels, STBI_rgb_alpha);
    if (NULL == data1) {
        std::cout << "cannot open file1: " << file1 << std::endl;
        return ERROR_INPUT_FILE;
    }

    unsigned char* data2 = stbi_load(file2, &w2, &h2, &ch2, STBI_rgb_alpha);
    if (NULL == data2) {
        stbi_image_free(data1);
        std::cout << "cannot open file2: " << file2 << std::endl;
        return ERROR_OUTPUT_FILE;
    }
    std::cout << "file1 size: " << width << "x" << height << " (" << nrChannels << " channels)" << std::endl;
    std::cout << "file2 size: " << w2 << "x" << h2 << " (" << ch2 << " channels)" << std::endl;

    if (width != w2 || height != h2 || nrChannels != ch2) {
        std::cout << "the two files's size not equal" << std::endl;
    }
    else {
        unsigned int* pImg1 = reinterpret_cast<unsigned int*>(data1);
        unsigned int* pImg2 = reinterpret_cast<unsigned int*>(data2);
        int errCnt = 0;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                if (*pImg1 != *pImg2) {
                    ++errCnt;
                }
                ++pImg1;
                ++pImg2;
            }
        }
        if (errCnt > 0) {
        	ret = ERROR_CUSTOM;
            std::cout << "there are " << errCnt << " different pixels" << std::endl;
        }
        else {
            std::cout << "the two files are same" << std::endl;
        }
    }

    stbi_image_free(data1);
    stbi_image_free(data2);
    return ret;
}


int main(int argc, char *argv[]) {
    int     func = 0;
    char const* inFileName = NULL;
    char const* outFileName = NULL;
    int     IsNewBuff = 0;
    int     ret = ERROR_OK;

#define USAGE   "USAGE: fblcd.out [--version] [-{en,de,cp} infile outfile]"

    if (argc < 2) {
        std::cout << USAGE << std::endl;
        std::cout << "parameters: " << std::endl;
        std::cout << "  --version              show author's info" << std::endl;
        std::cout << "  -en infile outfile     encode(compress) `infile` to `outfile`" << std::endl;
        std::cout << "                           `infile` should be BMP file" << std::endl;
        std::cout << "                           `outfile` is TILE file" << std::endl;
        std::cout << "                           if `outfile` parameter is not specified, it will assigned to `infile` and add JLCD suffix" << std::endl;
        std::cout << "  -de infile outfile     decode(decompress) `infile` to `outfile`" << std::endl;
        std::cout << "                           `infile` should be TILE file" << std::endl;
        std::cout << "                           `outfile` is BMP file" << std::endl;
        std::cout << "                           if `outfile` parameter is not specified, it will assigned to `infile` and add BMP suffix" << std::endl;
        std::cout << "  -cp infile outfile     compare `infile` and `outfile`, pixel by pixel" << std::endl;
        return ERROR_PARAM_NOT_ENOUGH;
    }

    if (strcmp(argv[1], "--version") == 0) {
        std::cout << USAGE << std::endl;
        // !!! REPLACE THE NAME TO YOUR TEAM'S
        std::cout << "made by: xxx (for example: Jingjia Electronics Co.,ltd)" << std::endl;
        std::cout << "version: " << APP_VERSION << std::endl;
        return ERROR_PARAM_NOT_ENOUGH;
    }
    else if (strcmp(argv[1], "-en") == 0) {
        func = 1;
    }
    else if (strcmp(argv[1], "-de") == 0) {
        func = 2;
    }
    else if (strcmp(argv[1], "-cp") == 0) {
        func = 3;
    }
    else {
        return ERROR_INVALID_PARAM;
    }

    if (argc < 3) {
        std::cout << "ERROR: parameter is not enough!" << std::endl;
        return ERROR_PARAM_NOT_ENOUGH;
    }
    inFileName = argv[2];
    if (argc >= 4) {
        outFileName = argv[3];
    }
    else {
        IsNewBuff = 1;
        outFileName = new char[strlen(inFileName) + 6];
        sprintf((char*)outFileName, "%s.%s", inFileName, (1==func) ? "jlcd" : "bmp");
        std::cout << "output file is not assigned, we assign it to: " << outFileName << std::endl;
    }

    if (func == 1) {
        // compress
        ret = compressARGB(inFileName, outFileName);
    } else if (func == 2) {
        // decompress
        ret = decompressARGB(inFileName, outFileName);
    }
    else {
        ret = compareBMP(inFileName, outFileName);
    }

    std::cout << "result = " << ret << std::endl;
    if (IsNewBuff) {
        delete[] outFileName;
        outFileName = NULL;
    }
    return ret;
}
