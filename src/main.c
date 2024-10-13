// This code is intended for single-threaded use only.

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "io.h"
#include "proto.h"
#include "types.h"

connected_user_t connected_users[MAX_CONNECTED_USERS];

uint8_t io_buf[IO_BUF_LEN];
decoded_user_t decoded_users[MAX_MSG_BLOCK_COUNT];

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

  memset(&connected_users, 0, sizeof(connected_users));
  memset(&decoded_users, 0, sizeof(decoded_users));
  memset(io_buf, 0, IO_BUF_LEN);

  io_ctx.io_buf = io_buf;
  io_ctx.io_buf_len = IO_BUF_LEN;
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

    int decoded_users_count =
        decode(io_ctx.io_buf, nbytes, decoded_users, MAX_MSG_BLOCK_COUNT);
    if (decoded_users_count == -1) {
      perror("decode failed");
      continue;
    }

    printf("block_count: %d\n", decoded_users_count);
    for (int i = 0; i < decoded_users_count; i++) {
      // printf("-----\n");
      // printf("name: %s\n", decoded_users[i].name);
      // printf("name_len: %d\n", decoded_users[i].name_len);
      // printf("score: %d\n", decoded_users[i].score);

      bool found = false;

      for (int j = 0; j < MAX_CONNECTED_USERS; j++) {
        if (!connected_users[j].connected ||
            connected_users[j].name_len != decoded_users[i].name_len)
          continue;

        if (strncmp(connected_users[j].name, decoded_users[i].name,
                    connected_users[j].name_len) == 0) {
          connected_users[j].score = decoded_users[i].score;
          found = true;
        }
      }

      if (!found) {
        for (int j = 0; j < MAX_CONNECTED_USERS; j++) {
          if (!connected_users[j].connected) {
            if (decoded_users[i].name_len + SIZE_OF_NULL_CHAR > MAX_NAME_LEN) {
              printf("name len is too large.\n");
              break;
            }
            connected_users[j].connected = true;
            connected_users[j].score = decoded_users[i].score;
            connected_users[j].name_len = decoded_users[i].name_len;
            strncpy(connected_users[j].name, decoded_users[i].name,
                    decoded_users[i].name_len + SIZE_OF_NULL_CHAR);
            break;
          }
        }
      }
    }

    // print connected users and send them to the client.
    for (int i = 0; i < MAX_CONNECTED_USERS; i++) {
      if (!connected_users[i].connected)
        continue;

      printf("-----\n");
      printf("name: %s\n", connected_users[i].name);
      printf("name_len: %d\n", connected_users[i].name_len);
      printf("score: %d\n", connected_users[i].score);
    }
  }

  return 0;
}
