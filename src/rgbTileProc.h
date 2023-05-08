/* rgbTileProc.h
*  implementation of TILE COMPRESSION
*/
#ifndef _RGBTILEPROC_H_
#define _RGBTILEPROC_H_

void tileSetSize(int nTileWidth, int nTileHeight);

/* compress ARGB data to tile
*  param:
*    pClrBlk      -- IN, pixel's ARGB data
*    pTile        -- OUT, tile data
*    pTileSize    -- OUT, tile's bytes
*  return:
*    0  -- succeed
*   -1  -- failed
*/
int argb2tile(const unsigned char *pClrBlk, unsigned char *pTile, int *pTileSize);

/* decompress tile data to ARGB
*  param:
*    pTile        -- IN, tile data
*    pTileSize    -- IN, tile's bytes
*    pClrBlk      -- OUT, pixel's ARGB data
*  return:
*    0  -- succeed
*   -1  -- failed
*/
int tile2argb(const unsigned char* pTile, int nTileSize, unsigned char* pClrBlk);

#endif
