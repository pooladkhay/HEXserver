#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "proto.h"
#include "types.h"

#define MAGIC_NUMBER 0x02AA

// These sizes are defined as the sum of individual members to
// avoid struct padding issues and `__attribute__((packed))`
#define SIZE_OF_HEADER sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint8_t)
#define SIZE_OF_BLOCK_HEADER sizeof(uint16_t) + sizeof(uint8_t)

uint16_t magic_number = MAGIC_NUMBER;

typedef struct _header {
  uint16_t magic_number;
  uint16_t payload_len;
  uint8_t block_count;
} header_t;

// Each block is practically one user
typedef struct _block_header {
  uint16_t score;
  uint8_t name_len;
} block_header_t;

// Writes up to `decoded_users_max_len` users to `decoded_users`.
// Return the number of users written, or `-1` on error with `errno` set.
// `decoded_users` is only valid as long as `raw_data` is valid. Accessing
// `decoded_users` after `raw_data` is invalid results in undefined behavior.
int decode(uint8_t *raw_data, int raw_data_len, decoded_user_t *decoded_users,
           int decoded_users_max_len) {

  errno = 0;
  if (raw_data == NULL || decoded_users == NULL) {
    errno = EFAULT;
    return -1;
  }

  header_t *header = (header_t *)raw_data;

  if (header->magic_number == MAGIC_NUMBER) {
    if (header->block_count == 0 || header->payload_len == 0)
      return 0;

    if (header->block_count > decoded_users_max_len ||
        header->payload_len + SIZE_OF_HEADER > raw_data_len) {
      errno = ENOBUFS;
      return -1;
    }

    int block_index = SIZE_OF_HEADER;
    for (int i = 0; i < header->block_count; i++) {
      int name_index = block_index + SIZE_OF_BLOCK_HEADER;

      block_header_t *bh = (block_header_t *)&raw_data[block_index];
      decoded_users[i].score = bh->score;
      decoded_users[i].name_len = bh->name_len;
      decoded_users[i].name = (char *)&raw_data[name_index];

      block_index = name_index + bh->name_len + SIZE_OF_NULL_CHAR;

      if (block_index >= header->payload_len)
        break;
    }
  } else {
    errno = EPROTO;
    return -1;
  }
  return header->block_count;
}
