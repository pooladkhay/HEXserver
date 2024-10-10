/*-----------------------------------------------+
|                    HEADER                      |
+----------------+---------------+---------------+
|  MAGIC_NUMBER  |  PAYLOAD_LEN  |  BLOCK_COUNT  |
|  (2 Bytes)     |  (2 Bytes)    |  (1 Byte)     |
+----------------+---------------+---------------+
|                    PAYLOAD                     |
+------------------------------------------------+
|                    Block 1                     |
|  +-------------+------------+---------------+  |
|  |  SCORE      |  NAME_LEN  |  NAME         |  |
|  |  (2 Bytes)  |  (1 Byte)  |  (Variable)   |  |
|  +-------------+------------+---------------+  |
+------------------------------------------------+
|                    Block 2                     |
|  +-------------+------------+---------------+  |
|  |  SCORE      |  NAME_LEN  |  NAME         |  |
|  |  (2 Bytes)  |  (1 Byte)  |  (Variable)   |  |
|  +-------------+------------+---------------+  |
+------------------------------------------------+
|                    Block n                     |
|                      ...                       |
+------------------------------------------------+

+------------------------------------------------+
| Notes:                                         |
| - `NAME`s are null-terminated.                 |
| - `NAME_LEN` does not include null character.  |
+------------------------------------------------*/

#ifndef PROTO_H_
#define PROTO_H_

#include <stdint.h>

typedef struct _decoded_block {
  char *name;
  uint16_t score;
  uint8_t name_len;
} decoded_block_t;

// Writes up to `decoded_blocks_len` blocks to `decoded_blocks`.
// Return the number of blocks written, or `-1` on error with `errno` set.
// `decoded_blocks` is only valid as long as `raw_data` is valid. Accessing
// `decoded_blocks` after `raw_data` is invalid results in undefined behavior.
int decode(uint8_t *raw_data, int raw_data_max_len,
           decoded_block_t *decoded_blocks, int decoded_blocks_max_len);

#endif
