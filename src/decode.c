#include <memory.h>
#include <string.h>

int decode(const unsigned char *pTile, int nTileSize, unsigned char *pClrBlk) {
  const unsigned char *pTileEnd = pTile + nTileSize - 2;

  unsigned int headByte = (*pTile++) & 31;

  while (1) {
    if (headByte < 32) {
      headByte++;
      memcpy(pClrBlk, pTile, headByte);
      pClrBlk += headByte;
      pTile += headByte;
    } else {
      unsigned int nByte = (headByte >> 5) - 1;
      if (nByte == 6) {
        nByte += *pTile;
        pTile++;
      }
      nByte += 3;
      unsigned int nOffset = (headByte & 31) << 8;
      const unsigned char *pCopyPos = pClrBlk - nOffset - *pTile - 1;
      pTile++;

      {
        unsigned int nCount = nByte;
        unsigned char *pDest = pClrBlk;
        const unsigned char *pSrc = pCopyPos;
        while (nCount--) {
          *pDest++ = *pSrc++;
        }
      }

      pClrBlk += nByte;
    }

    if (pTile > pTileEnd)
      break;

    headByte = *pTile++;
  }

  return 0;
}
