#ifndef TYPES_H_
#define TYPES_H_

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define PORT 8081

#define BUF_LEN 1024 * 4

#define MAX_NAME_LEN 32
#define MAX_CONNECTED_USERS 64
#define MAX_HEART_BEAT_TIMEOUT 10.0 // seconds

typedef struct _connected_user {
  char name[MAX_NAME_LEN];
  time_t last_hb;
  uint16_t score;
  uint8_t name_len;
  bool connected;

} connected_user_t;

typedef struct _decoded_user {
  char *name;
  uint16_t score;
  uint8_t name_len;
} decoded_user_t;

#endif
