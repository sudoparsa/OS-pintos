#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "filesys/cache.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format)
{

  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  cache_init();
  free_map_init ();

  if (format)
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void)
{
  free_map_close ();
  cache_shutdown (fs_device);
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *path, off_t initial_size, bool directory)
{
  block_sector_t inode_sector = 0;
  struct dir *parent;
  char tail[NAME_MAX + 1];

  bool success = (dir_divide_path (&parent, tail, path)
                  && tail[0] != '\0'
                  && parent != NULL
                  && free_map_allocate (1, &inode_sector)
                  && (directory
                      ?dir_create (inode_sector, 16)
                      :inode_create (inode_sector, initial_size, false))
                  && dir_add (parent, tail, inode_sector));
  if (!success && inode_sector != 0)
    free_map_release (inode_sector, 1);
  dir_close (parent);

  return success;
}
/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *path)
{
  if (strcmp(path, "/") == 0)
  {
    return (struct file *) dir_open_root ();
  }

  char tail[NAME_MAX + 1];
  struct dir *dir = NULL;
  dir_divide_path (&dir, tail, path);
  struct inode *inode = NULL;

  if (dir != NULL)
    dir_lookup (dir, tail, &inode);
  dir_close (dir);

  if (inode == NULL)
  {
    return NULL;
  }

  struct file* res;
  if (inode_isdir (inode))
  {
    res = (struct file *) dir_open (inode);
    return res;
  }
  else
    return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *path)
{
  char tail[NAME_MAX + 1];
  struct dir *dir = NULL;
  dir_divide_path (&dir, tail, path);
  
  bool success = dir != NULL && dir_remove (dir, tail);
  dir_close (dir);

  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}
