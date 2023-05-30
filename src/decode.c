#include <memory.h>

#include "fastlz.h"

int decode(const unsigned char *pTile, int nTileSize, unsigned char *pClrBlk) {
  fastlz_decompress(pTile, nTileSize, pClrBlk, 8 * 8 * 4);
  return 0;
}
