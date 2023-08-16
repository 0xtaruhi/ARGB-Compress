/* rgbTileProc.cpp
 *  implementation of TILE COMPRESSION
 */
#include "rgbTileProc.h"
#include <assert.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <math.h>
#include <memory.h>
#include <stdio.h>

#include "decode.h"
#include "encode.h"

#include "statistics.h"

#include "IntegratedSimulator.hpp"

int g_nTileWidth = 0;
int g_nTileHeight = 0;
// uint64_t cycles = 0;

void tileSetSize(int nTileWidth, int nTileHeight) {
  g_nTileWidth = nTileWidth;
  g_nTileHeight = nTileHeight;
}

/* compress ARGB data to tile
 *  param:
 *    pClrBlk      -- IN, pixel's ARGB data
 *    pTile        -- OUT, tile data
 *    pTileSize    -- OUT, tile's bytes
 *  return:
 *    0  -- succeed
 *   -1  -- failed
 */
int argb2tile(const unsigned char *pClrBlk, unsigned char *pTile,
              int *pTileSize) {
  assert(g_nTileWidth > 0 && g_nTileHeight > 0);

  unsigned char *reorderd_clr_blk = (unsigned char *)malloc(
      g_nTileWidth * g_nTileHeight * 4 * sizeof(unsigned char));
  memset(reorderd_clr_blk, 0, g_nTileHeight * g_nTileWidth * 4);
  unsigned char *p = reorderd_clr_blk;

  for (int i = 0; i < g_nTileHeight * g_nTileWidth * 4; ++i) {
    if (i % 2 == 0) {
      *p |= pClrBlk[i] << 4;
    } else {
      *p |= pClrBlk[i] & 15;
      ++p;
    }
  }

  for (int i = 0; i < g_nTileHeight * g_nTileWidth * 4; ++i) {
    if (i % 2 == 0) {
      *p |= pClrBlk[i] & 0xf0;
    } else {
      *p |= pClrBlk[i] >> 4;
      ++p;
    }
  }

  int result = encode(pTile, pTileSize, reorderd_clr_blk);

  auto &simulator = cat::IntegratedSimulator::getInstance();
  // auto simulator = cat::IntegratedSimulator();
  simulator.setEncodeLength(g_nTileHeight * g_nTileWidth * 4);

  for (int i = 0; i < g_nTileHeight * g_nTileWidth; ++i) {
    simulator.writeUnencodedMemory(
        i * 4, reinterpret_cast<const uint32_t *>(reorderd_clr_blk)[i]);
  }

  // for (int i = 0; i < 4096; ++i) {
  //   simulator.writeHashMemory(i * 4, 0);
  // }

  simulator.reset();
  simulator.runEncode();

  // cycles += simulator.getEncodeCyclesNum();

  auto& statistics = Statistics::getInstance();
  statistics.addData(simulator.getEncodeCyclesNum(), 0,
                     simulator.getEncodedLength());

  auto encoded_length = simulator.getEncodedLength();

  for (int i = 0; i < (encoded_length - 1) / 4 + 1; ++i) {
    uint32_t data = simulator.readUndecodedMemory(i * 4);
    memcpy(pTile + i * 4, &data, 4);
  }

  *pTileSize = encoded_length;

  free(reorderd_clr_blk);
  // return result;
  return 0;
}

/* decompress tile data to ARGB
 *  param:
 *    pTile        -- IN, tile data
 *    pTileSize    -- IN, tile's bytes
 *    pClrBlk      -- OUT, pixel's ARGB data
 *  return:
 *    0  -- succeed
 *   -1  -- failed
 */
int tile2argb(const unsigned char *pTile, int nTileSize,
              unsigned char *pClrBlk) {
  g_nTileWidth = 8;
  g_nTileHeight = 8;

  unsigned char *reorderd_clr_blk = (unsigned char *)malloc(
      g_nTileWidth * g_nTileHeight * 4 * sizeof(unsigned char));
  // memset(reorderd_clr_blk, 0, g_nTileWidth * g_nTileHeight * 4);
  // int result = decode(pTile, nTileSize, reorderd_clr_blk);
  int result = 0;

  auto &sim = cat::IntegratedSimulator::getInstance();
  sim.setEncodedLength(nTileSize);
  for (int i = 0; i < (nTileSize - 1) / 4 + 1; ++i) {
    uint32_t data;
    memcpy(&data, pTile + i * 4, 4);
    sim.writeUndecodedMemory(i * 4, data);
  }

  sim.reset();
  sim.runDecode();

  for (int i = 0; i < g_nTileWidth * g_nTileHeight; ++i) {
    uint32_t data = sim.readUnencodedMemory(i * 4);
    memcpy(reorderd_clr_blk + i * 4, &data, 4);
  }

  // cycles += sim.getDecodeCyclesNum();
  auto& statistics = Statistics::getInstance();
  statistics.addData(0, sim.getDecodeCyclesNum(), 0);

  unsigned char *p = reorderd_clr_blk;
  unsigned int half_size = g_nTileWidth * g_nTileHeight * 4 / 2;

  for (int i = 0; i < g_nTileWidth * g_nTileHeight * 4; ++i) {
    if (i % 2 == 0) {
      pClrBlk[i] = (*p >> 4) | (*(p + half_size) & 0xf0);
    } else {
      pClrBlk[i] = (*p & 15) | (*(p + half_size) << 4);
      ++p;
    }
  }
  free(reorderd_clr_blk);
  return result;
}
