/* rgbTileProc.cpp
 *  implementation of TILE COMPRESSION
 */
#include "rgbTileProc.h"
#include <assert.h>
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
  // *pTileSize = g_nTileWidth * g_nTileHeight * 4;
  // memcpy(pTile, pClrBlk, *pTileSize);
  // return 0;
  return encode(pTile, pTileSize, pClrBlk);
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
