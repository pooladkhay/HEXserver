
#include <arpa/inet.h>
#include <liburing.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include "io.h"

#define Q_LEN 64

int ret, server_fd;

struct sockaddr_in server_addr;
struct sockaddr_in client_addr;
struct msghdr msg;
struct iovec iov;

struct io_uring ring;
struct io_uring_sqe *sqe;
struct io_uring_cqe *cqe;

static int setup_socket(io_ctx_t *ctx) {
  server_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_fd == -1) {
    perror("socket:");
    return -1;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(ctx->port);

  ret = bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (ret != 0) {
    perror("bind:");
    close(server_fd);
    return -1;
  }

  return 0;
}

static int io_uring_init(io_ctx_t *ctx) {
  struct io_uring_params params;

  memset(&params, 0, sizeof(params));
  params.flags = IORING_SETUP_COOP_TASKRUN | IORING_SETUP_SINGLE_ISSUER;

  return io_uring_queue_init_params(Q_LEN, &ring, &params);
}

static int uring_process_cqe(io_ctx_t *ctx) {
  // change fd to non-blocking for io_uring_peek_cqe
  ret = io_uring_wait_cqe(&ring, &cqe);
  if (ret < 0) {
    perror("io_uring_wait_cqe:");
    return -1;
  }
  if (cqe->res < 0) {
    fprintf(stderr, "io_uring_wait_cqe - async request failed: %s\n",
            strerror(-cqe->res));
    return -1;
  }

  io_uring_cqe_seen(&ring, cqe);

  // remove or --debug
  if (cqe->res > 0) {
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr.sin_port);

    printf("Read '%d' bytes from %s:%d\n", cqe->res, client_ip, client_port);
  }

  return cqe->res;
}

int io_init(int io_type, io_ctx_t *ctx) {
  memset(&ring, 0, sizeof(ring));
  memset(&server_addr, 0, sizeof(server_addr));
  memset(&client_addr, 0, sizeof(client_addr));
  memset(&iov, 0, sizeof(iov));
  memset(&msg, 0, sizeof(msg));

  if (setup_socket(ctx) < 0) {
    perror("setup_socket:");
    return -1;
  }

  // setup msg struct for recvmsg
  msg.msg_name = &client_addr;
  msg.msg_namelen = sizeof(client_addr);
  iov.iov_base = ctx->io_buf;
  iov.iov_len = ctx->io_buf_len;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  ret = io_uring_init(ctx);
  if (ret < 0) {
    fprintf(stderr, "io_uring_init: %s\n", strerror(-ret));
    return -1;
  }

  return 0;
}

// This function blocks until a message is received. It reads up to
// `ctx->buf_len` bytes to `ctx->buf`. Returns the number of bytes read or -1
// for errors.
int io_recvmsg(io_ctx_t *ctx) {
  sqe = io_uring_get_sqe(&ring);
  if (!sqe) {
    fprintf(stderr, "io_uring_get_sqe: %s\n",
            strerror(-ret)); // SQ full
    return -1;
  }

  io_uring_prep_recvmsg(sqe, server_fd, &msg, 0);

  if (io_uring_submit(&ring) < 0) {
    fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));
    return -1;
  }

  ret = uring_process_cqe(ctx);
  if (ret < 0) {
    fprintf(stderr, "uring_process_cqe: %s\n", strerror(-ret));
    return -1;
  }

  return ret;
}

int io_cleanup() {
  io_uring_queue_exit(&ring);
  return close(server_fd);
}
