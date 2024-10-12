# HEXserver

This is a single-threaded UDP server that utilizes `io_uring` for IO operations.

Clients ([HEXvaders](https://github.com/pooladkhay/HEXvaders)) communicate with the server using a custom binary protocol described in [`proto.h`](./src/proto.h).

⚠️ This is a work-in-progress and not implemented in the game yet.
