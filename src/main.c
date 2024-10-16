// This code is intended for single-threaded use only.

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "io.h"
#include "proto.h"
#include "types.h"

uint8_t buf[BUF_LEN];
io_ctx_t io_ctx;

decoded_user_t decoded_users[PROTO_MAX_BLOCK_COUNT];

connected_user_t connected_users[MAX_CONNECTED_USERS];

void sigint_handler(int signo);
void update_connected_users(int decoded_users_count);
void print_connected_users();

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
  memset(buf, 0, BUF_LEN);

  io_ctx.buf = buf;
  io_ctx.buf_len = BUF_LEN;
  io_ctx.port = port;
  io_ctx.req_type = 0;

  if (io_init(IO_TYPE_IOURING, &io_ctx) != 0)
    exit(-1);

  io_recvmsg(&io_ctx);

  while (1) {
    // --debug
    printf("------------\n");
    printf("Listening on port: %d\n", port);
    printf("Waiting for clients...\n");
    printf("------------\n");

    nbytes = io_wait(&io_ctx);
    if (nbytes <= 0)
      continue;

    int decoded_users_count = 0;
    switch (io_ctx.req_type) {
    case 'R':
      decoded_users_count =
          decode(io_ctx.buf, nbytes, decoded_users, PROTO_MAX_BLOCK_COUNT);
      if (decoded_users_count == -1) {
        perror("decode failed");
        continue;
      }

      if (decoded_users_count == 0) {
        // empty message: heart beat
        // update "last_updated" field
        printf("heart beat msg received\n");
        continue;
      }

      // --debug
      printf("read %d blocks from msg\n", decoded_users_count);

      update_connected_users(decoded_users_count);

      // --debug
      print_connected_users();

      // encode connected users and send them to the user whose request was just
      // processed:

      nbytes =
          encode(io_ctx.buf, BUF_LEN, connected_users, MAX_CONNECTED_USERS);
      if (decoded_users_count <= 0) {
        perror("encode failed");
        continue;
      }

      // --debug
      printf("encoded %d bytes to buf\n", nbytes);

      io_sendmsg(&io_ctx, nbytes);

      break;
    case 'S':
      // maybe retry the send if was not successful
      break;
    }

    io_recvmsg(&io_ctx);
  }

  return 0;
}

void update_connected_users(int decoded_users_count) {
  for (int i = 0; i < decoded_users_count; i++) {
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
}

void print_connected_users() {
  for (int i = 0; i < MAX_CONNECTED_USERS; i++) {
    if (!connected_users[i].connected)
      continue;

    printf("-----\n");
    printf("name: %s\n", connected_users[i].name);
    printf("name_len: %d\n", connected_users[i].name_len);
    printf("score: %d\n", connected_users[i].score);
  }
}

void sigint_handler(int signo) { exit(io_cleanup()); }
