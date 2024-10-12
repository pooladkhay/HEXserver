// This code is intended for single-threaded use only.

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "io.h"
#include "proto.h"

#define PORT 8081
#define BUF_LEN 1024 * 4
#define MAX_PAYLOAD_BLOCK_COUNT 128
#define MAX_NAME_LEN 32
#define MAX_CONNECTED_USERS 64

typedef struct _connected_user {
  char name[MAX_NAME_LEN];
  uint16_t score;
  uint8_t name_len;
  bool connected;
} connected_user_t;

connected_user_t users[MAX_CONNECTED_USERS];

uint8_t buf[BUF_LEN];
decoded_block_t blocks[MAX_PAYLOAD_BLOCK_COUNT];

io_ctx_t io_ctx;

void sigint_handler(int signo) { exit(io_cleanup()); }

int main(int argc, char *argv[]) {
  int nbytes;
  uint16_t port = PORT;

  signal(SIGINT, sigint_handler);

  if (argc > 2 && strcmp(argv[1], "-p") == 0) {
    port = atoi(argv[2]);
    if (port < 1 || port > 65535) {
      printf("Port number must be between 1 and 65535.\n");
      port = PORT;
    }
  }

  memset(&users, 0, sizeof(users));
  memset(&blocks, 0, sizeof(blocks));
  memset(buf, 0, BUF_LEN);

  io_ctx.buf = buf;
  io_ctx.buf_len = BUF_LEN;
  io_ctx.port = port;

  if (io_init(IO_TYPE_IOURING, &io_ctx) != 0)
    exit(-1);

  while (1) {
    printf("------------\n");
    printf("Listening on port: %d\n", port);
    printf("Waiting for clients...\n");
    printf("------------\n");

    nbytes = io_recvmsg(&io_ctx);
    if (nbytes <= 0)
      continue;

    int blocks_count =
        decode(io_ctx.buf, nbytes, blocks, MAX_PAYLOAD_BLOCK_COUNT);
    if (blocks_count == -1) {
      perror("decode failed");
      continue;
    }

    printf("block_count: %d\n", blocks_count);
    for (int i = 0; i < blocks_count; i++) {
      // printf("-----\n");
      // printf("name: %s\n", blocks[i].name);
      // printf("name_len: %d\n", blocks[i].name_len);
      // printf("score: %d\n", blocks[i].score);

      bool found = false;

      for (int j = 0; j < MAX_CONNECTED_USERS; j++) {
        if (!users[j].connected || users[j].name_len != blocks[i].name_len)
          continue;

        if (strncmp(users[j].name, blocks[i].name, users[j].name_len) == 0) {
          users[j].score = blocks[i].score;
          found = true;
        }
      }

      if (!found) {
        for (int j = 0; j < MAX_CONNECTED_USERS; j++) {
          if (!users[j].connected) {
            if (blocks[i].name_len + SIZE_OF_NULL_CHAR > MAX_NAME_LEN) {
              printf("name len is too large.\n");
              break;
            }
            users[j].connected = true;
            users[j].score = blocks[i].score;
            users[j].name_len = blocks[i].name_len;
            strncpy(users[j].name, blocks[i].name,
                    blocks[i].name_len + SIZE_OF_NULL_CHAR);
            break;
          }
        }
      }
    }

    // print connected users and send them to the client.
    for (int i = 0; i < MAX_CONNECTED_USERS; i++) {
      if (!users[i].connected)
        continue;

      printf("-----\n");
      printf("name: %s\n", users[i].name);
      printf("name_len: %d\n", users[i].name_len);
      printf("score: %d\n", users[i].score);

      // send logic
    }
  }

  return 0;
}
