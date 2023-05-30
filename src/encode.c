#include "encode.h"
#include <memory.h>

extern int g_nTileWidth;
extern int g_nTileHeight;

int encode(unsigned char *pTile, int *pTileSize, const unsigned char *pClrBlk) {
  *pTileSize = g_nTileHeight * g_nTileWidth * 4;
  memcpy(pTile, pClrBlk, *pTileSize);
  return 0;
}
