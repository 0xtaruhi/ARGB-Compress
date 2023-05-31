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
  g_nTileWidth = 8;
  g_nTileHeight = 8;

  unsigned char *reorderd_clr_blk = (unsigned char *)malloc(
      g_nTileWidth * g_nTileHeight * 4 * sizeof(unsigned char));
  // memset(reorderd_clr_blk, 0, g_nTileWidth * g_nTileHeight * 4);
  int result = decode(pTile, nTileSize, reorderd_clr_blk);
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
