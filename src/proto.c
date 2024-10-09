#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "proto.h"

#define MAGIC_NUMBER 0x02AA
#define SIZE_OF_NULL_CHAR 1

// These sizes are defined as the sum of individual members to
// avoid struct padding issues and `__attribute__((packed))`
#define SIZE_OF_HEADER sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint8_t)
#define SIZE_OF_BLOCK_HEADER sizeof(uint16_t) + sizeof(uint8_t)

typedef struct _header {
  uint16_t magic_number;
  uint16_t payload_len;
  uint8_t block_count;
} header_t;

typedef struct _block_header {
  uint16_t score;
  uint8_t name_len;
} block_header_t;

// Writes up to `decoded_blocks_len` blocks to `decoded_blocks`.
// Return the number of blocks written, or `-1` on error with `errno` set.
// `decoded_blocks` is only valid as long as `raw_data` is valid. Accessing
// `decoded_blocks` after `raw_data` is invalid results in undefined behavior.
int decode(uint8_t *raw_data, decoded_block_t *decoded_blocks,
           int decoded_blocks_len) {

  errno = 0;
  if (raw_data == NULL) {
    errno = EFAULT;
    return -1;
  }
  if (decoded_blocks == NULL) {
    errno = EFAULT;
    return -1;
  }

  header_t *header = (header_t *)raw_data;

  if (header->magic_number == MAGIC_NUMBER) {
    if (header->block_count == 0 || header->payload_len == 0)
      return 0;

    int block_index = SIZE_OF_HEADER;
    for (int i = 0; i < header->block_count; i++) {
      int name_index = block_index + SIZE_OF_BLOCK_HEADER;

      block_header_t *bh = (block_header_t *)&raw_data[block_index];
      decoded_blocks[i].score = bh->score;
      decoded_blocks[i].name_len = bh->name_len;
      decoded_blocks[i].name = (char *)&raw_data[name_index];

      block_index = name_index + bh->name_len + SIZE_OF_NULL_CHAR;

      if (block_index >= header->payload_len)
        break;
    }
  } else {
    errno = EILSEQ;
    return -1;
  }
  return header->block_count;
}
