#include <liburing.h>
#include <netinet/in.h>
#include <stdint.h>
#include <sys/types.h>

enum { IO_TYPE_IOURING, IO_TYPE_EPOLL, IO_TYPE_DEFAULT };

typedef struct _io_ctx {
  uint8_t *buf;
  size_t buf_len;
  int req_type;
  uint16_t port;
} io_ctx_t;

int io_init(int io_type, io_ctx_t *ctx);

// This function blocks until a message is received. It reads up to
// `ctx->buf_len` bytes to `ctx->buf`. Returns the number of bytes read or -1
// for errors.
int io_recvmsg(io_ctx_t *ctx);

int io_sendmsg(io_ctx_t *ctx, int nbytes);

// This function blocks until a message is received. On return, it sets
// `ctx->req_type` to either 'R' (indicating arrival of a new message) or 'S'
// (indicating success in sending a message). It returns number of bytes
// received or sent.
int io_wait(io_ctx_t *ctx);

int io_cleanup();
