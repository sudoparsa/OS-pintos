/* Checks growing of hit rate after a file has been accessed.
   To check this, a file is created with random data, and hit
   rate is calculated twice for accessing. The hit rate should
   grow in the second run. */

#include <syscall.h>
#include <random.h>
#include "tests/lib.h"
#include "tests/main.h"

#define BLOCK_COUNT  128
#define BLOCK_SIZE 512
#define FILE_SIZE (BLOCK_COUNT * BLOCK_SIZE)
#define ACCEPTABLE_ERROR  6

char buf[FILE_SIZE];

float benchmark (int fd, size_t size);

void
test_main (void)
{
  CHECK (create ("a", FILE_SIZE), "create \"a\" should not fail.");
  msg ("File creation completed.");
  invalidate_cache ();
  int fd = open("a");
  CHECK (fd > 2, "open \"a\" returned fd which should be greater than 2.");
  random_init (0);
  random_bytes (buf, sizeof buf);
  buf[FILE_SIZE - 1] = '\0';
  int write_bytes = write(fd, buf, FILE_SIZE);
  if (write_bytes != FILE_SIZE)
    fail ("write returned a value other than file size %d. (%d)", FILE_SIZE, write_bytes);
  close (fd);
  msg ("write completed.");
  size_t max_write = BLOCK_COUNT + ACCEPTABLE_ERROR;
  CHECK (cache_write_count () < max_write,
         "Block write count should be less than %d.", max_write);
  CHECK (cache_read_count () < ACCEPTABLE_ERROR,
         "Block read count should be less than acceptable error %d.", ACCEPTABLE_ERROR);

  CHECK (remove ("a"), "Removed \"a\".");
}
