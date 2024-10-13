/*-----------------------------------------------+
|                   MSG HEADER                   |
+----------------+---------------+---------------+
|  MAGIC_NUMBER  |  PAYLOAD_LEN  |  BLOCK_COUNT  |
|  (2 Bytes)     |  (2 Bytes)    |  (1 Byte)     |
+----------------+---------------+---------------+
|                   MSG PAYLOAD                  |
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

#include "types.h"
#include <stdint.h>

#define SIZE_OF_NULL_CHAR 1
#define MAX_MSG_BLOCK_COUNT 128
// #define MAX_MSG_PAYLOAD_LEN ??

// Writes up to `decoded_users_max_len` users to `decoded_users`.
// Return the number of users written, or `-1` on error with `errno` set.
// `decoded_users` is only valid as long as `raw_data` is valid. Accessing
// `decoded_users` after `raw_data` is invalid results in undefined behavior.
int decode(uint8_t *raw_data, int raw_data_len, decoded_user_t *decoded_users,
           int decoded_users_max_len);

#endif
