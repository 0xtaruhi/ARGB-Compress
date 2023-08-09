#include "encode.h"
#include <memory.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX_LEN 264

extern int g_nTileWidth;
extern int g_nTileHeight;

uint32_t readWord(const uint8_t *p);
uint32_t compare(const uint8_t *p1, const uint8_t *p2, const uint8_t *bound);

typedef struct InputInfo_ {
  const uint8_t *start;
  uint32_t size;

  const uint8_t *(*getStart)(const struct InputInfo_ *self);
  const uint8_t *(*getEnd)(const struct InputInfo_ *self);
  const uint8_t *(*getLimit)(const struct InputInfo_ *self);

  int (*exceedsLimit)(const struct InputInfo_ *self, const uint8_t *p);
} InputInfo;

InputInfo *InputInfo_init(const uint8_t *start, const uint32_t size);
void InputInfo_free(InputInfo *self);
const uint8_t *InputInfo_getStart(const InputInfo *self);
const uint8_t *InputInfo_getEnd(const InputInfo *self);
const uint8_t *InputInfo_getLimit(const InputInfo *self);
int InputInfo_exceedsLimit(const InputInfo *self, const uint8_t *p);

typedef struct HashTable_ {
  uint8_t key_size_;
  uint32_t size_;
  uint32_t *table;

  uint32_t (*get)(const struct HashTable_ *self, uint32_t key);
  void (*set)(struct HashTable_ *self, uint32_t key, uint32_t value);
  uint16_t (*hashFunc)(const struct HashTable_ *self, uint32_t key);
} HashTable;

HashTable *HashTable_init(int key_size,
                          uint16_t (*hashFunc)(const HashTable *, uint32_t));
void HashTable_free(HashTable *self);
uint32_t HashTable_get(const HashTable *self, uint32_t key);
void HashTable_set(HashTable *self, uint32_t key, uint32_t value);
uint16_t HashTable_normalHashFunc(const HashTable *self, uint32_t key);

typedef struct OutputInfo_ {
  uint8_t *start;
  uint8_t *end;
  uint8_t *current;

  int32_t (*getSize)(const struct OutputInfo_ *self);
  void (*dumpLiterals)(struct OutputInfo_ *self, uint32_t runs,
                       const uint8_t *src);
  void (*dumpMatch)(struct OutputInfo_ *self, uint32_t length,
                    uint32_t distance);
} OutputInfo;

OutputInfo *OutputInfo_init(uint8_t *start);
void OutputInfo_free(OutputInfo *self);
int32_t OutputInfo_getSize(const OutputInfo *self);
void OutputInfo_dumpLiterals(OutputInfo *self, uint32_t runs,
                             const uint8_t *src);
void OutputInfo_dumpMatch(OutputInfo *self, uint32_t length, uint32_t distance);

/// COMMON FUNCTIONS
uint32_t readWord(const uint8_t *p) { return *(uint32_t *)p; }

uint32_t compare(const uint8_t *p1, const uint8_t *p2, const uint8_t *bound) {
  const uint8_t *start = p1;
  if (readWord(p1) == readWord(p2)) {
    p1 += 4;
    p2 += 4;
  }
  while (p2 < bound) {
    if (*p1++ != *p2++) {
      break;
    }
  }
  return p1 - start;
}

/// INPUT INFO
InputInfo *InputInfo_init(const uint8_t *start, const uint32_t size) {
  InputInfo *self = (InputInfo *)malloc(sizeof(InputInfo));
  self->start = start;
  self->size = size;

  self->getStart = InputInfo_getStart;
  self->getEnd = InputInfo_getEnd;
  self->getLimit = InputInfo_getLimit;
  self->exceedsLimit = InputInfo_exceedsLimit;

  return self;
}

void InputInfo_free(InputInfo *self) { free(self); }

const uint8_t *InputInfo_getStart(const InputInfo *self) { return self->start; }

const uint8_t *InputInfo_getEnd(const InputInfo *self) {
  return self->start + self->size;
}

const uint8_t *InputInfo_getLimit(const InputInfo *self) {
  return self->start + self->size - 13;
}

int InputInfo_exceedsLimit(const InputInfo *self, const uint8_t *p) {
  return p > self->getLimit(self);
}

/// HASH TABLE
HashTable *HashTable_init(int key_size,
                          uint16_t (*hashFunc)(const HashTable *, uint32_t)) {
  HashTable *self = (HashTable *)malloc(sizeof(HashTable));
  self->key_size_ = key_size;
  self->size_ = 1 << key_size;
  self->table = (uint32_t *)malloc(sizeof(uint32_t) * self->size_);

  self->get = HashTable_get;
  self->set = HashTable_set;
  self->hashFunc = hashFunc;
  memset(self->table, 0, sizeof(uint32_t) * self->size_);
  return self;
}

void HashTable_free(HashTable *self) {
  free(self->table);
  free(self);
}

uint32_t HashTable_get(const HashTable *self, uint32_t key) {
  return self->table[key & (self->size_ - 1)];
}

void HashTable_set(HashTable *self, uint32_t key, uint32_t value) {
  self->table[key & (self->size_ - 1)] = value;
}

uint16_t HashTable_normalHashFunc(const HashTable *self, uint32_t key) {
  uint16_t mask = self->size_ - 1;
  return ((key ^ 0x9E3779B9) >> (32 - self->key_size_)) & mask;
}

/// OUTPUT INFO
OutputInfo *OutputInfo_init(uint8_t *start) {
  OutputInfo *self = (OutputInfo *)malloc(sizeof(OutputInfo));
  self->start = start;
  self->end = start;
  self->current = start;

  self->getSize = OutputInfo_getSize;
  self->dumpLiterals = OutputInfo_dumpLiterals;
  self->dumpMatch = OutputInfo_dumpMatch;
  return self;
}

void OutputInfo_free(OutputInfo *self) { free(self); }

int32_t OutputInfo_getSize(const OutputInfo *self) {
  return self->current - self->start;
}

void OutputInfo_dumpLiterals(OutputInfo *self, uint32_t runs,
                             const uint8_t *src) {
  while (runs >= 32) {
    *self->current++ = 31;
    memcpy(self->current, src, 32);
    self->current += 32;
    runs -= 32;
    src += 32;
  }
  if (runs > 0) {
    *self->current++ = runs - 1;
    memcpy(self->current, src, runs);
    self->current += runs;
  }
}

void OutputInfo_dumpMatch(OutputInfo *self, uint32_t length,
                          uint32_t distance) {
  --distance;
  if (length > MAX_LEN - 2) {
    while (length > MAX_LEN - 2) {
      *self->current++ = (7 << 5) + (distance >> 8);
      *self->current++ = MAX_LEN - 2 - 7 - 2;
      *self->current++ = (distance & 255);
      length -= MAX_LEN - 2;
    }
  }
  if (length < 7) {
    *self->current++ = (length << 5) + (distance >> 8);
    *self->current++ = (distance & 255);
  } else {
    *self->current++ = (7 << 5) + (distance >> 8);
    *self->current++ = length - 7;
    *self->current++ = (distance & 255);
  }
}

int encode(unsigned char *pTile, int *pTileSize, const unsigned char *pClrBlk) {
  const int input_length = g_nTileHeight * g_nTileWidth * 4;

  InputInfo *input_info = InputInfo_init(pClrBlk, input_length);

  OutputInfo *output_info = OutputInfo_init(pTile);
  HashTable *hash_table = HashTable_init(12, HashTable_normalHashFunc);

  const uint8_t *anchor = input_info->getStart(input_info);
  const uint8_t *ip = input_info->getStart(input_info);

  const uint8_t *limit = input_info->getLimit(input_info);

  ip += 2;
  while (ip < limit) {
    const uint8_t *ref;
    uint32_t distance;

    uint32_t seq;
    do {
      seq = readWord(ip) & 0xffffff;
      uint16_t hash = hash_table->hashFunc(hash_table, seq);
      ref =
          input_info->getStart(input_info) + hash_table->get(hash_table, hash);
      hash_table->set(hash_table, hash, ip - input_info->getStart(input_info));
      distance = ip - ref;

      if (ip >= limit) {
        break;
      }
      ++ip;
    } while (seq != (readWord(ref) & 0xffffff));

    if (ip >= limit) {
      break;
    }
    --ip;

    if (ip > anchor) {
      output_info->dumpLiterals(output_info, ip - anchor, anchor);
    }

    uint32_t length =
        compare(ref + 3, ip + 3, input_info->getEnd(input_info) - 4);
    output_info->dumpMatch(output_info, length, distance);

    ip += length;
    seq = readWord(ip);
    uint16_t hash = hash_table->hashFunc(hash_table, seq & 0xffffff);
    hash_table->set(hash_table, hash, ip++ - input_info->getStart(input_info));
    seq >>= 8;
    hash = hash_table->hashFunc(hash_table, seq);
    hash_table->set(hash_table, hash, ip++ - input_info->getStart(input_info));

    anchor = ip;
  }

  uint32_t left = input_info->getEnd(input_info) - anchor;
  output_info->dumpLiterals(output_info, left, anchor);
  *pTileSize = output_info->getSize(output_info);

  // Clean up.
  HashTable_free(hash_table);
  OutputInfo_free(output_info);
  InputInfo_free(input_info);

  return 0; // return normally
}
