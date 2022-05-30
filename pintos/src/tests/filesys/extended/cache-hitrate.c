/* Checks growing of hit rate after a file has been accessed.
   To check this, a file is created with random data, and hit
   rate is calculated twice for accessing. The hit rate should
   grow in the second run. */

#include <syscall.h>
#include <random.h>
#include "tests/lib.h"
#include "tests/main.h"

#define FILE_SIZE  10240
#define BLOCK_SIZE 512

char buf[FILE_SIZE];

void
test_main (void)
{
  CHECK (create ("a", FILE_SIZE), "create \"a\" should not fail.");
  int fd = open("a");
  CHECK (fd > 2, "open \"a\" returned fd which should be greater than 2.");
  random_init (0);
  random_bytes (buf, sizeof buf);
  buf[FILE_SIZE - 1] = '\0';
  CHECK (FILE_SIZE == write(fd, buf, FILE_SIZE), "write should write all the data to file.");
  close (fd);
  invalidate_cache ();
  msg ("File creation completed.");

  fd = open("a");
  for (size_t i = 0; i < FILE_SIZE / BLOCK_SIZE; i ++)
  {
    int read_bytes = read (fd, buf, BLOCK_SIZE);
    if (read_bytes != BLOCK_SIZE)
      fail ("read should read a whole block, not %d.", read_bytes);
  }
  close(fd);

  uint32_t hit = cache_hit ();
  uint32_t miss = cache_miss ();

  msg ("First run cache hit rate calculated.");

  fd = open("a");
  for (size_t i = 0; i < FILE_SIZE / BLOCK_SIZE; i ++)
  {
    int read_bytes = read (fd, buf, BLOCK_SIZE);
    if (read_bytes != BLOCK_SIZE)
      fail ("read should read a whole block, not %d.", read_bytes);
  }
  close(fd);

  uint32_t new_hit = cache_hit ();
  uint32_t new_miss = cache_miss ();

  msg ("Second run cumulative cache hit rate calculated.");
  CHECK (new_hit * (hit + miss) > hit * (new_hit + new_miss), "Hit rate should grow.");

  CHECK (remove ("a"), "Removed \"a\".");
}
