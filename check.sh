#!/bin/bash

FBLCD=build/fblcd.out
GEN_DIR=gen
RES_DIR=res

# get base name
f=$(basename $1)

echo "Compressed file: $f"
$FBLCD -en $RES_DIR/$f $GEN_DIR/$f.jlcd
echo "Decompressed file: $f.jlcd"
$FBLCD -de $GEN_DIR/$f.jlcd $GEN_DIR/$f.jlcd.bmp
echo "Compare $f"
$FBLCD -cp $RES_DIR/$f $GEN_DIR/$f.jlcd.bmp
