#include "filesys/directory.h"
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "filesys/file.h"
#include "threads/malloc.h"
#include "threads/thread.h"

/* A directory. */
struct dir
  {
    struct inode *inode;                /* Backing store. */
    off_t pos;                          /* Current position. */
  };

/* A single directory entry. */
struct dir_entry
  {
    block_sector_t inode_sector;        /* Sector number of header. */
    char name[NAME_MAX + 1];            /* Null terminated file name. */
    bool in_use;                        /* In use or free? */
  };


/* Extracts a file name part from *SRCP into PART, and updates *SRCP so that the
next call will return the next file name part. Returns 1 if successful, 0 at
end of string, -1 for a too-long file name part. (from source doc)*/
static int 
get_next_part(char part[NAME_MAX + 1], const char** srcp)
{
  const char* src = *srcp;
  char* dst = part;
  /* Skip leading slashes. If it's all slashes, we're done. */
  while (*src == '/')
    src++;
  if (*src == '\0')
    return 0;

  /* Copy up to NAME_MAX character from SRC to DST. Add null terminator. */
  while (*src != '/' && *src != '\0') 
    {
      if (dst < part + NAME_MAX)
        *dst++ = *src;
      else
        return -1;
      src++;
    }
  *dst = '\0';
  /* Advance source pointer. */
  *srcp = src;
  return 1;
}


/* Reports the parent directory and the name of file/directory
   in `tail`. Note that `tail` must have allocated at least
   `NAME_MAX + 1` bytes of memory. `tail` will change to empty
   string in case of failure. */
bool
dir_divide_path(struct dir **parent, char *tail, const char *path)
{
  const char *t_path = path;

  if (path[0] == '/')
    *parent = dir_open_root ();
  else
  {
    // This is done to prevent PANIC before thread creation.
    if (thread_current ()->cwd == NULL)
      *parent = dir_open_root ();
    else
    {
      if (inode_get_removed(thread_current ()->cwd->inode))
        goto failed;
      *parent = dir_reopen (thread_current ()->cwd);
    }
  }

  *tail = '\0';
  while (true)
  {
    struct inode *next_inode = NULL;
    bool failed_lookup = false;
    if (strlen(tail) > 0)
    {
      if (!dir_lookup (*parent, tail, &next_inode))
        failed_lookup = true;
    }

    int result = get_next_part (tail, &t_path);
    if (result < 0)
      goto failed;
    else if (result == 0)
      break;
    else {
      if (failed_lookup)
        goto failed;
      if (next_inode) {
        if (inode_get_removed (next_inode))
          goto failed;
        struct dir *next_dir = dir_open(next_inode);
        if (next_dir == NULL)
          goto failed;

        dir_close (*parent);
        *parent = next_dir;
      }
    }
  }

  return true;

  failed:
  *tail = '\0';
  *parent = NULL;
  dir_close (*parent);
  return false;
}

/* Opens the directory in path. Returns NULL on failure. */
struct dir *
dir_open_by_path (const char *path)
{
  if (strcmp(path, "/") == 0)
  {
    return dir_open_root ();
  }

  char tail[NAME_MAX + 1];
  struct dir *parent;
  dir_divide_path(&parent, tail, path);

  struct inode *inode;
  if (!dir_lookup (parent, tail, &inode) || inode_get_removed (inode))
  {
    dir_close (parent);
    return NULL;
  }

  return dir_open (inode);
}

static struct dir_entry
get_new_entry (block_sector_t sector, const char *name)
{
  struct dir_entry e;
  e.inode_sector = sector;
  e.in_use = true;
  strlcpy (e.name, name, sizeof e.name);
  return e;
}

/* Check whether the directory is empty or not */
static bool
check_directory (struct dir *dir)
{
  struct dir_entry e;
  off_t ofs;

  for (ofs = 1 * sizeof e; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e)
      if (e.in_use)
        return false;
  return true;
}


/* Creates a directory with space for ENTRY_CNT entries in the
   given SECTOR.  Returns true if successful, false on failure. */
bool
dir_create (block_sector_t sector, size_t entry_cnt)
{
  // this should be checked
  if (!inode_create (sector, (entry_cnt) * sizeof (struct dir_entry), true))
    return false;
  
  struct inode *dir_inode = inode_open (sector);
  struct dir_entry parent_entry = get_new_entry (sector, "..");
  // struct dir_entry current_entry = get_new_entry (sector, ".");

  bool success;
  success = inode_write_at (dir_inode, &parent_entry, 
            sizeof parent_entry, 0) == sizeof parent_entry;

  // success &= inode_write_at (dir_inode, &current_entry, 
  //            sizeof current_entry, sizeof parent_entry) == sizeof current_entry;

  inode_close (dir_inode);
  return success;
}

/* Opens and returns the directory for the given INODE, of which
   it takes ownership.  Returns a null pointer on failure. */
struct dir *
dir_open (struct inode *inode)
{
  struct dir *dir = calloc (1, sizeof *dir);
  if (inode != NULL && dir != NULL)
    {
      dir->inode = inode;
      /* may change because of readdir funcion. */
      dir -> pos = 1 * sizeof (struct dir_entry);
      // dir->pos = 1 * sizeof (struct dir_entry);
      return dir;
    }
  else
    {
      inode_close (inode);
      free (dir);
      return NULL;
    }
}

/* Opens the root directory and returns a directory for it.
   Return true if successful, false on failure. */
struct dir *
dir_open_root (void)
{
  return dir_open (inode_open (ROOT_DIR_SECTOR));
}

/* Opens and returns a new directory for the same inode as DIR.
   Returns a null pointer on failure. */
struct dir *
dir_reopen (struct dir *dir)
{
  ASSERT (dir != NULL);
  return dir_open (inode_reopen (dir->inode));
}

/* Destroys DIR and frees associated resources. */
void
dir_close (struct dir *dir)
{
  if (dir != NULL)
    {
      inode_close (dir->inode);
      free (dir);
    }
}

/* Returns the inode encapsulated by DIR. */
struct inode *
dir_get_inode (struct dir *dir)
{
  ASSERT (dir != NULL);
  return dir->inode;
}

/* Searches DIR for a file with the given NAME.
   If successful, returns true, sets *EP to the directory entry
   if EP is non-null, and sets *OFSP to the byte offset of the
   directory entry if OFSP is non-null.
   otherwise, returns false and ignores EP and OFSP. */
static bool
lookup (const struct dir *dir, const char *name,
        struct dir_entry *ep, off_t *ofsp)
{
  struct dir_entry e;
  size_t ofs;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  for (ofs = 1 * sizeof (struct dir_entry) ; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e)
    if (e.in_use && !strcmp (name, e.name))
      {
        if (ep != NULL)
          *ep = e;
        if (ofsp != NULL)
          *ofsp = ofs;
        return true;
      }
  return false;
}

/* Searches DIR for a file with the given NAME
   and returns true if one exists, false otherwise.
   On success, sets *INODE to an inode for the file, otherwise to
   a null pointer.  The caller must close *INODE. */
bool
dir_lookup (const struct dir *dir, const char *name,
            struct inode **inode)
{
  struct dir_entry e;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);
  if (strcmp (name, "..") == 0)
    {
      inode_read_at (dir->inode, &e, sizeof e, 0);
      *inode = inode_open (e.inode_sector);
    }
  else if (strcmp (name, ".") == 0)
    *inode = inode_reopen (dir->inode);
  else if (lookup (dir, name, &e, NULL))
    *inode = inode_open (e.inode_sector);
  else
    *inode = NULL;

  return *inode != NULL;
}

/* Adds a file named NAME to DIR, which must not already contain a
   file by that name.  The file's inode is in sector
   INODE_SECTOR.
   Returns true if successful, false on failure.
   Fails if NAME is invalid (i.e. too long) or a disk or memory
   error occurs. */
bool
dir_add (struct dir *dir, const char *name, block_sector_t inode_sector)
{
  struct dir_entry e;
  off_t ofs;
  bool success = false;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Check NAME for validity. */
  if (*name == '\0' || strlen (name) > NAME_MAX)
    return false;

  /* Check that NAME is not in use. */
  if (lookup (dir, name, NULL, NULL))
    goto done;

  /* if file does not exist, return false */
  struct inode *dir_inode = inode_open (inode_sector);
  if (dir_inode == NULL)
    return false;

  if (inode_disk_isdir (get_inode_disk (dir_inode)))
    {
      struct dir_entry child_entry;
      child_entry = get_new_entry (inode_get_inumber (dir_get_inode (dir)), "..");
      /* error happened while writing at child directory */
      if (inode_write_at (dir_inode, &child_entry, sizeof child_entry, 0)
            != sizeof child_entry)
        {
          inode_close (dir_inode);
          return false;
        }
    }
    inode_close (dir_inode);

  /* Set OFS to offset of free slot.
     If there are no free slots, then it will be set to the
     current end-of-file.

     inode_read_at() will only return a short read at end of file.
     Otherwise, we'd need to verify that we didn't get a short
     read due to something intermittent such as low memory. */
  for (ofs = 1 * sizeof (struct dir_entry); inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e)
    if (!e.in_use)
      break;

  /* Write slot. */
  // e.in_use = true;
  // strlcpy (e.name, name, sizeof e.name);
  // e.inode_sector = inode_sector;
  e = get_new_entry (inode_sector, name);
  success = inode_write_at (dir->inode, &e, sizeof e, ofs) == sizeof e;

 done:
  return success;
}

/* Removes any entry for NAME in DIR.
   Returns true if successful, false on failure,
   which occurs only if there is no file with the given NAME. */
bool
dir_remove (struct dir *dir, const char *name)
{
  struct dir_entry e;
  struct inode *inode = NULL;
  bool success = false;
  off_t ofs;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Find directory entry. */
  if (!lookup (dir, name, &e, &ofs))
    goto done;

  /* Open inode. */
  inode = inode_open (e.inode_sector);
  if (inode == NULL)
    goto done;

  if (inode_isdir (inode))
    {
      struct dir *t_dir = dir_open (inode);
      bool empty = check_directory (t_dir);
      dir_close (t_dir);
      if (!empty)
        goto done;
    }

  /* Erase directory entry. */
  e.in_use = false;
  if (inode_write_at (dir->inode, &e, sizeof e, ofs) != sizeof e)
    goto done;
  /* Remove inode. */
  inode_remove (inode);
  success = true;

 done:
  inode_close (inode);
  return success;
}

/* Reads the next directory entry in DIR and stores the name in
   NAME.  Returns true if successful, false if the directory
   contains no more entries. */
bool
dir_readdir (struct dir *dir, char name[NAME_MAX + 1])
{
  struct dir_entry e;

  while (inode_read_at (dir->inode, &e, sizeof e, dir->pos) == sizeof e)
    {
      dir->pos += sizeof e;
      if (e.in_use)
        {
          strlcpy (name, e.name, NAME_MAX + 1);
          return true;
        }
    }
  dir->pos = 1 * sizeof (struct dir_entry);
  return false;
}


/* This function converts file descriptor pointers that are
   a directory, to struct dir. Returns NULL in case the input
   isn't a directory. */
struct dir *
dir_from_file (struct file *file)
{
  if (file == NULL)
    return NULL;

  struct inode *inode = file_get_inode(file);

  if (inode == NULL)
    return NULL;

  if (!inode_isdir (inode))
    return NULL;

  return (struct dir *) file;
}
