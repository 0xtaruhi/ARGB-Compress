#pragma once
#ifndef HUFFMAN_H_
#define HUFFMAN_H_

#include "common.h"

typedef struct HuffmanNode_ {
  Byte value;
  unsigned int freq;
  struct HuffmanNode_ *left;
  struct HuffmanNode_ *right;
} HuffmanNode;

#ifdef cplusplus__
extern "C" {
#endif

HuffmanNode *huffmanNode_new(Byte value, unsigned int freq, HuffmanNode *left,
                             HuffmanNode *right);

HuffmanNode *buildHuffmanTree(unsigned int *freq);

void buildHuffmanTable(HuffmanNode *root, unsigned int code, unsigned int size,
                       unsigned int *table);

void deleteHuffmanTree(HuffmanNode *root);

void createHuffmanTable(HuffmanNode *root, Byte *table);

void huffmanEncode(Byte *data, unsigned int size, HuffmanNode *tree,
                   Byte **encoded_data, unsigned int *encoded_size);

#ifdef cplusplus__
}
#endif

#endif /* HUFFMAN_H_ */
