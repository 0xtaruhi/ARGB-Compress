#include "encode.h"
#include <memory.h>

#include "fastlz.h"

extern int g_nTileWidth;
extern int g_nTileHeight;

int encode(unsigned char *pTile, int *pTileSize, const unsigned char *pClrBlk) {
  *pTileSize = g_nTileHeight * g_nTileWidth * 4;
  int len = fastlz_compress(pClrBlk, *pTileSize, pTile);
  *pTileSize = len;
  return 0;
}
