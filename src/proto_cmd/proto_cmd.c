#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// #include <assert.h>
// #include <fcntl.h>

#define MAX_BUF_LEN 1024 * 2 // udp does not support fragmentation
#define HEADER_SIZE sizeof(uint16_t) * 2 + sizeof(uint8_t)

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s SCORE,NAME_LEN,NAME ...\n\n", argv[0]);
    fprintf(stderr,
            "This command line tool encodes args to the HEXproto "
            "binary format.\nThe encoded data is written to the stdout.\n");
    fprintf(stderr, "\nExample:\n");
    fprintf(stderr, "  %s 11,6,NAFISE 5,2,MJ 10,3,BOB > data.bin\n", argv[0]);
    return 0;
  }

  uint8_t buf[MAX_BUF_LEN];

  // MAGIC_NUMBER
  uint16_t mn = htons(0x02AA);
  memcpy(buf, &mn, sizeof(uint16_t));

  // PAYLOAD_LEN
  uint16_t pl = 0x0000;
  memcpy(&buf[2], &pl, sizeof(uint16_t));

  // BLOCK_COUNT
  buf[4] = 0x00;

  uint8_t count = 0;
  size_t buf_index = HEADER_SIZE;

  for (int i = 1; i < argc; i++) {
    count += 1;
    uint8_t name_len = 0;
    char *c = NULL;

    for (int j = 0; j < 3; j++) {
      switch (j) {
      case 0:
        errno = 0;
        long temp_score = strtol(argv[i], &c, 10);
        if (errno == ERANGE || *c != ',' || temp_score < 0 ||
            temp_score > 65535) {
          fprintf(stderr, "ERROR: Invalid score format at argument %d.\n", i);
          return 1;
        }

        uint16_t score = htons((uint16_t)temp_score);

        if (buf_index + sizeof(score) >= MAX_BUF_LEN) {
          fprintf(stderr, "ERROR: Buffer overflow detected.\n");
          return 1;
        }

        fprintf(stderr, "score: %ld\n", temp_score);

        memcpy(&buf[buf_index], &score, sizeof(score));
        buf_index += sizeof(uint16_t);

        break;
      case 1:
        errno = 0;
        long temp_len = strtol(c + 1, &c, 10);
        if (errno == ERANGE || *c != ',' || temp_len < 0 || temp_len > 255) {
          fprintf(stderr, "ERROR: Invalid score format at argument %d.\n", i);
          return 1;
        }

        if (buf_index + sizeof(name_len) >= MAX_BUF_LEN) {
          fprintf(stderr, "ERROR: Buffer overflow detected.\n");
          return 1;
        }

        name_len = (uint8_t)temp_len;

        fprintf(stderr, "name_len: %u\n", name_len);
        buf[buf_index] = name_len;
        buf_index += sizeof(uint8_t);

        break;
      case 2:
        if (buf_index + name_len + 1 >= MAX_BUF_LEN) {
          fprintf(stderr, "ERROR: Buffer overflow detected.\n");
          return 1;
        }

        memcpy(&buf[buf_index], c + 1, name_len);
        size_t tt = buf_index;

        buf_index += name_len;
        buf[buf_index] = '\0';
        buf_index += 1;
        fprintf(stderr, "name: %s\n", (char *)&buf[tt]);
        break;
      }
    }
    fprintf(stderr, "-------------------\n");
  }

  pl = htons(buf_index - HEADER_SIZE);
  memcpy(&buf[2], &pl, sizeof(uint16_t));

  *(uint8_t *)&buf[4] = count;

  fprintf(stderr, "payload_len: %d\n", htons(*(uint16_t *)&buf[2]));
  fprintf(stderr, "count: %d\n", count);

  ssize_t written = write(STDOUT_FILENO, buf, buf_index);
  if (written < 0) {
    perror("Error writing to stdout");
    return 1;
  } else if (written != buf_index) {
    fprintf(stderr, "Partial write to stdout. Expected %zu, wrote %zd bytes.\n",
            buf_index, written);
    return 1;
  }
}

// Future plan:
// fprintf(stderr, "Usage: %s [OPTIONS]\n\n", argv[0]);

// fprintf(stderr,
//         "This command line tool (en|de)codes from stdin to stdout.\n"
//         "When encoding, the data must be ASCII format in blocks of "
//         "SCORE,NAME_LEN,NAME separated by space or newline.\n"
//         "When decoding, the data must be in binary based on the HEXproto
//         " "format.\n" "Note that all data must be available before
//         (en|de)code process " "can start.\n");
// fprintf(stderr, "\nOptions:\n");
// fprintf(stderr, "  -h, --help        Show this help message and exit\n");
// fprintf(stderr,
//         "  --verbose         Enable verbose mode (detailed output)\n");
// fprintf(stderr, "  -e, --encode      Encode stdin to stdout\n");
// fprintf(stderr, "  -d, --decode      Decode stdin to stdout\n");
// fprintf(stderr, "\nExamples:\n");
// fprintf(stderr, "  %s --encode < data.txt > data.bin\n", argv[0]);
// fprintf(stderr, "  cat data.bin | %s --decode > data.txt\n", argv[0]);
// fprintf(stderr, "  echo '11,6,NAFISE 5,2,MJ 10,3,BOB' | %s -e > data.bin\n",
// argv[0]);
