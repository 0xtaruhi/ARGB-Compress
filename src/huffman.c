#include "huffman.h"
#include <stdlib.h>

HuffmanNode *huffmanNode_new(Byte value, unsigned int freq, HuffmanNode *left,
                             HuffmanNode *right) {
  HuffmanNode *node = (HuffmanNode *)(malloc(sizeof(HuffmanNode)));
  node->value = value;
  node->freq = freq;
  node->left = left;
  node->right = right;
  return node;
}

void calculateFrequency(Byte *data, int size, unsigned int *freq) {
  for (int i = 0; i < size; i++) {
    freq[data[i]]++;
  }
}

HuffmanNode *buildHuffmanTree(unsigned int *freq) {
  HuffmanNode **nodes = (HuffmanNode **)malloc(256 * sizeof(HuffmanNode *));
  for (unsigned int i = 0; i < 256; ++i) {
    nodes[i] = huffmanNode_new(i, freq[i], NULL, NULL);
  }
  while (1) {
    // find two min freq
    int min1 = -1, min2 = -1;
    for (unsigned int i = 0; i < 256; ++i) {
      if (nodes[i] != NULL) {
        if (min1 == -1 || nodes[i]->freq < nodes[min2]->freq) {
          min2 = min1;
          min1 = i;
        } else if (min2 == -1 || nodes[i]->freq < nodes[min2]->freq) {
          min2 = i;
        }
      }
    }

    if (min2 == -1) {
      return nodes[min1];
    }

    HuffmanNode *new_node =
        huffmanNode_new(0, freq[min1] + freq[min2], nodes[min1], nodes[min2]);
    nodes[min1] = new_node;
  }
};

void deleteHuffmanTree(HuffmanNode *root) {
  if (root == NULL) {
    return;
  }
  deleteHuffmanTree(root->left);
  deleteHuffmanTree(root->right);
  free(root);
}

void createHuffmanTable_impl(HuffmanNode *huffman_tree, Byte *huffman_table,
                             Byte code, int depth) {
  if (huffman_tree->left == NULL && huffman_tree->right == NULL) {
    huffman_table[huffman_tree->value] = code;
    return;
  }
  if (huffman_tree->left != NULL) {
    createHuffmanTable_impl(huffman_tree->left, huffman_table, code << 1,
                            depth + 1);
  }
  if (huffman_tree->right != NULL) {
    createHuffmanTable_impl(huffman_tree->right, huffman_table, (code << 1) + 1,
                            depth + 1);
  }
}

void createHuffmanTable(HuffmanNode *huffman_tree, Byte *huffman_table) {
  createHuffmanTable_impl(huffman_tree, huffman_table, 0, 0);
}

void huffmanEncode(Byte *data, unsigned int size, HuffmanNode *huffman_tree,
                   Byte **encoded_data, unsigned int *encoded_size) {
  Byte huffman_table[256] = {0};
  createHuffmanTable(huffman_tree, huffman_table);

  // calculate encoded data size
  unsigned int data_size = 0;
  for (unsigned int i = 0; i < size; ++i) {
    data_size += huffman_table[data[i]];
  }

  // allocate memory
  *encoded_data = (Byte *)malloc(data_size / 8 + 1);
  *encoded_size = data_size / 8 + 1;

  // encode
  Byte *p = *encoded_data;
  int offset = 7; //< 0 ~ 7, offset of current byte
  Byte buffer = 0;

  for (unsigned int i = 0; i < size; ++i) {
    Byte code = huffman_table[data[i]];
    for (int j = 0; j < 8; ++j) {
      if (code & (1 << (7 - j))) {
        buffer |= (1 << offset);
      }
      offset--;
      if (offset < 0) {
        *p = buffer;
        p++;
        offset = 7;
        buffer = 0;
      }
    }
  }

  // last byte
  if (offset != 7) {
    *p = buffer;
  }
}
