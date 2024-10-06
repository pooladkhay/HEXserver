// This code is intended for single-threaded use only.

#include <arpa/inet.h>
#include <liburing.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#define Q_LEN 64
#define PORT 8081
#define BUF_LEN 512

int ret, nbytes;
char buf[BUF_LEN];
struct iovec iov;

uint16_t port = PORT;
int server_fd;
struct sockaddr_in server_addr, client_addr;
struct msghdr msg;

struct io_uring ring;
struct io_uring_sqe *sqe;
struct io_uring_cqe *cqe;

void sigint_handler(int signo) {
  io_uring_queue_exit(&ring);
  close(server_fd);
  exit(0);
}

static int setup_socket() {
  server_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_fd == -1) {
    perror("socket:");
    return -1;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);

  ret = bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (ret != 0) {
    perror("bind:");
    close(server_fd);
    return -1;
  }

  // setup msg struct
  msg.msg_name = &client_addr;
  msg.msg_namelen = sizeof(client_addr);
  {
    iov.iov_base = &buf;
    iov.iov_len = sizeof(buf);
  }
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  return 0;
}

static int io_uring_init() {
  struct io_uring_params params;

  memset(&params, 0, sizeof(params));

  // params.cq_entries = Q_LEN * 8;
  params.flags = IORING_SETUP_COOP_TASKRUN |
                 IORING_SETUP_SINGLE_ISSUER; // | IORING_SETUP_CQSIZE |
                                             // IORING_SETUP_SUBMIT_ALL;

  return io_uring_queue_init_params(Q_LEN, &ring, &params);
}

static int uring_prep_recvmsg_submit() {

  sqe = io_uring_get_sqe(&ring);
  if (!sqe) {
    fprintf(stderr, "io_uring_get_sqe: %s\n",
            strerror(-ret)); // SQ full
    return -1;
  }

  io_uring_prep_recvmsg(sqe, server_fd, &msg, 0);

  ret = io_uring_submit(&ring);
  if (ret < 0) {
    fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));
    return -1;
  }

  return 0;
}

static int uring_process_cqe() {
  // change to non-blocking for io_uring_peek_cqe
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

  nbytes = cqe->res;

  if (nbytes > 0) {
    if (nbytes < BUF_LEN) {
      buf[nbytes] = '\0';
    } else {
      buf[BUF_LEN - 1] = '\0';
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip,
              INET_ADDRSTRLEN); // Assuming client is using IPv4
    int client_port = ntohs(client_addr.sin_port);
    printf("------------\n");
    printf("Read '%d' bytes from %s:%d\n", nbytes, client_ip, client_port);
    printf("Message: %s\n", buf);
  }

  io_uring_cqe_seen(&ring, cqe);

  return 0;
}

int main(int argc, char *argv[]) {
  signal(SIGINT, sigint_handler);

  memset(&ring, 0, sizeof(ring));
  memset(&server_addr, 0, sizeof(server_addr));
  memset(&client_addr, 0, sizeof(client_addr));
  memset(&buf, 0, sizeof(buf));
  memset(&iov, 0, sizeof(iov));
  memset(&msg, 0, sizeof(msg));

  if (argc > 2 && strcmp(argv[1], "-p") == 0) {
    port = atoi(argv[2]);
    if (port < 1 || port > 65535) {
      printf("Port number must be between 1 and 65535.\n");
      port = PORT;
    }
  }

  if (setup_socket() < 0) {
    perror("setup_socket:");
    return -1;
  }
  printf("Listening on port: %d\n", port);

  ret = io_uring_init();
  if (ret < 0) {
    fprintf(stderr, "io_uring_init: %s\n", strerror(-ret));
    goto cleanup;
  }

  while (1) {
    ret = uring_prep_recvmsg_submit();
    if (ret < 0) {
      fprintf(stderr, "uring_prep_recvmsg_submit: %s\n", strerror(-ret));
      goto cleanup;
    }

    printf("------------\n");
    printf("Waiting for clients...\n");
    ret = uring_process_cqe();
    if (ret < 0) {
      fprintf(stderr, "uring_process_cqe: %s\n", strerror(-ret));
      goto cleanup;
    }
  }

cleanup:
  io_uring_queue_exit(&ring);
  close(server_fd);
  return (ret < 0) ? -1 : 0;
}
