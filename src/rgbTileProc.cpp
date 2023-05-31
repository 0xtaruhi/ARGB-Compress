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

int g_nTileWidth = 0;
int g_nTileHeight = 0;

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
  // memcpy(reorderd_clr_blk, pClrBlk,
  //        g_nTileWidth * g_nTileHeight * 4 * sizeof(unsigned char));
  memset(reorderd_clr_blk, 0, g_nTileHeight * g_nTileWidth * 4);
  unsigned char* p = reorderd_clr_blk;

  for (int i = 0; i < g_nTileHeight * g_nTileWidth * 4; ++i) {
    if (i % 2 == 0) {
      *p |= pClrBlk[i] << 4;
    } else {
      *p |= pClrBlk[i];
      ++p;
    }
  }

  uint32_t* p2 = (uint32_t*)reorderd_clr_blk;
  for (int i = 1; i < g_nTileHeight; i += 2) {
    // swap 
    for (int j = 0; j < g_nTileWidth / 2; ++j) {
      uint32_t tmp = p2[i * g_nTileWidth + j];
      p2[i * g_nTileWidth + j] = p2[(i + 1) * g_nTileWidth - j - 1];
      p2[(i + 1) * g_nTileWidth - j - 1] = tmp;
    }
  }

  int result = encode(pTile, pTileSize,reorderd_clr_blk);
  free(reorderd_clr_blk);
  return result;
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
  return decode(pTile, nTileSize, pClrBlk);
}
