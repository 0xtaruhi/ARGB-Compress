#ifndef DECODE_H
#define DECODE_H

#ifdef __cplusplus
extern "C" {
#endif

/// @param pTile Pointer point to encoded data.
/// @param nTileSize The size of the encoded data(Byte).
/// @param pClrBlk Pointer point to decoded data. This address is
/// allocated by caller
int decode(const unsigned char *pTile, int nTileSize, unsigned char *pClrBlk);

#ifdef __cplusplus
}
#endif

#endif // DECODE_H
