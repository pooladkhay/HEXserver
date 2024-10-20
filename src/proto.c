#include <errno.h>
#include <netinet/in.h>
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
#define SIZE_OF_HEADER                                                         \
  (size_t)(sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint8_t))
#define SIZE_OF_BLOCK_HEADER (size_t)(sizeof(uint16_t) + sizeof(uint8_t))

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

// Runs a 16-byte value stored in `dest` through `htons()`. Returns the result
// of the underlying call to `memcpy()`.
void *memcpy_htons(void *dest, const void *src, size_t n);

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

  header->magic_number = ntohs(header->magic_number);
  header->payload_len = ntohs(header->payload_len);

  if (header->magic_number == MAGIC_NUMBER) {
    if (header->block_count == 0 || header->payload_len == 0)
      return 0;

    if ((size_t)header->block_count > (size_t)decoded_users_max_len ||
        (size_t)header->payload_len + SIZE_OF_HEADER > (size_t)raw_data_len) {
      errno = ENOBUFS;
      return -1;
    }

    size_t block_index = SIZE_OF_HEADER;
    for (int i = 0; i < header->block_count; i++) {
      size_t name_index = block_index + SIZE_OF_BLOCK_HEADER;

      block_header_t *bh = (block_header_t *)&raw_data[block_index];
      bh->score = ntohs(bh->score);

      decoded_users[i].score = bh->score;
      decoded_users[i].name_len = bh->name_len;
      decoded_users[i].name = (char *)&raw_data[name_index];

      block_index = name_index + (size_t)bh->name_len + SIZE_OF_NULL_CHAR;

      if (block_index >= (size_t)header->payload_len)
        break;
    }
  } else {
    errno = EPROTO;
    return -1;
  }
  return header->block_count;
}

// Writes up to `buf_len` bytes to `buf`.
// Return the number of bytes written, or `-1` on error with `errno` set.
int encode(uint8_t *buf, int buf_len, connected_user_t *users, int users_len) {

  errno = 0;
  if (buf == NULL || users == NULL) {
    errno = EFAULT;
    return -1;
  }

  if (buf_len < sizeof(connected_user_t) * users_len) {
    errno = ENOBUFS;
    return -1;
  };

  // Header
  uint16_t payload_len = 0;
  uint8_t block_count = 0;

  memcpy_htons(buf, &magic_number, sizeof(uint16_t));
  memcpy_htons(&buf[sizeof(uint16_t)], &payload_len, sizeof(uint16_t));
  memcpy(&buf[sizeof(uint16_t) * 2], &block_count, sizeof(uint8_t));

  // Payload
  size_t index = SIZE_OF_HEADER;
  for (int i = 0; i < users_len; i++) {
    if (!users[i].connected)
      continue;

    memcpy_htons(&buf[index], &users[i].score, sizeof(uint16_t));
    index += sizeof(uint16_t);

    memcpy(&buf[index], &users[i].name_len, sizeof(uint8_t));
    index += sizeof(uint8_t);

    //  maybe strncpy?
    int _s = users[i].name_len + SIZE_OF_NULL_CHAR;
    memcpy(&buf[index], users[i].name, _s);
    index += _s;

    block_count += 1;
  }

  if (block_count != 0) {
    size_t i = index - SIZE_OF_HEADER;

    if (i < 0 || i > 65535) {
      errno = ERANGE;
      return -1;
    }

    payload_len = (uint16_t)(index - SIZE_OF_HEADER);
    printf("payload_len %d\n", payload_len);
    memcpy_htons(&buf[sizeof(uint16_t)], &payload_len, sizeof(uint16_t));
    memcpy(&buf[sizeof(uint16_t) * 2], &block_count, sizeof(uint8_t));
  }

  return index;
}

void *memcpy_htons(void *dest, const void *src, size_t n) {
  if (!dest || !src)
    return 0;

  uint16_t _src = htons(*(uint16_t *)src);
  return memcpy(dest, &_src, n);
}