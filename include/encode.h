#ifndef ENCODE_H
#define ENCODE_H

#ifdef __cplusplus
extern "C" {
#endif

/// @param pTile Pointer point to encoded data.
/// @param nTileSize The size of the encoded data(Byte).
/// @param pClrBlk Pointer point to original data.
int encode(unsigned char *pTile, int *pTileSize, const unsigned char *pClrBlk);

#ifdef __cplusplus
}
#endif

#endif // ENCODE_H
