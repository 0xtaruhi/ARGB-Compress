#include <memory.h>

int decode(const unsigned char *pTile, int nTileSize, unsigned char *pClrBlk) {
  memcpy(pClrBlk, pTile, nTileSize);
  return 0;
}
