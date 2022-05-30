#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "filesys/cache.h"
#include "threads/synch.h"


/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

#define DIRECT_BLOCK_NO 123
#define INDIRECT_BLOCK_NO 128
#define UNALLOCATED 0


struct inode_disk *get_inode_disk (const struct inode *);
static bool inode_disk_allocate (struct inode_disk *disk_inode, off_t length);
static bool allocate_sector (block_sector_t *sector_idx);
static bool allocate_indirect (block_sector_t sector_idx, size_t num_sectors_to_allocate);
static bool inode_disk_deallocate (struct inode *inode);
static bool deallocate_indirect (block_sector_t sector_num, size_t num_sectors_to_allocate);
/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    /* `start` is removed as file blocks are saved in the added variables now. */ 
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */

    bool is_dir;                        /* True if the file is a directory. */

    block_sector_t direct[DIRECT_BLOCK_NO];         /* Direct blocks of inode. */
    block_sector_t indirect;            /* Indirect blocks of inode. */
    block_sector_t double_indirect;     /* Double indirect blocks of indoe. */
    /* `unused` is removed as it's being used. :D */
  };

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct lock f_lock;                 /* Synchronization between users of inode. */
  };

struct inode_disk *
get_inode_disk (const struct inode *inode)
{
  struct inode_disk *id = malloc ( sizeof *id);
  cache_read (fs_device, inode->sector, (void *)id, 0, BLOCK_SECTOR_SIZE);
  return id;
}

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos)
{
  ASSERT (inode != NULL);

  struct inode_disk *id = get_inode_disk (inode);
  block_sector_t res = -1;
  if (pos >= id->length)
    {
      free (id);
      return res;
    }

  
  off_t block_idx = pos / BLOCK_SECTOR_SIZE;
  if (pos < DIRECT_BLOCK_NO * BLOCK_SECTOR_SIZE)
    {
      res = id->direct[block_idx];
      free (id);
      return res;
    }

  else if (pos < (DIRECT_BLOCK_NO + INDIRECT_BLOCK_NO) * BLOCK_SECTOR_SIZE)
    {
      block_sector_t indirect_blocks[INDIRECT_BLOCK_NO];
      cache_read (fs_device, id->indirect, &indirect_blocks, 0, BLOCK_SECTOR_SIZE);
      block_idx = block_idx - DIRECT_BLOCK_NO;
      res =  indirect_blocks[block_idx];
      free (id);
      return res;
    }
  else
    {
      block_sector_t double_indirect_blocks[INDIRECT_BLOCK_NO];
      cache_read (fs_device, id->double_indirect, &double_indirect_blocks, 0, BLOCK_SECTOR_SIZE);
      off_t indirect_block_idx = (block_idx - DIRECT_BLOCK_NO - INDIRECT_BLOCK_NO) / INDIRECT_BLOCK_NO;
      off_t idx = (block_idx - DIRECT_BLOCK_NO - INDIRECT_BLOCK_NO) % INDIRECT_BLOCK_NO;
      cache_read (fs_device, double_indirect_blocks[indirect_block_idx], &double_indirect_blocks, 0, BLOCK_SECTOR_SIZE);
      res = double_indirect_blocks[idx];
      free (id);
      return res;
    }
}


/* Initializes the inode module. */
void
inode_init (void)
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length, bool is_dir)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      disk_inode->is_dir = is_dir;
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      if (inode_disk_allocate (disk_inode, length))
        {
          cache_write (fs_device, sector, disk_inode, 0, BLOCK_SECTOR_SIZE);
          success = true;
        }
      free (disk_inode);
    }
  return success;
}

static bool
inode_disk_allocate (struct inode_disk *disk_inode, off_t length)
{
  if (length < 0)
    return false;
  size_t num_sectors_to_allocate = bytes_to_sectors (length);
  size_t i;
  for (i = 0; i < num_sectors_to_allocate && i < DIRECT_BLOCK_NO; i++)
    if (!disk_inode->direct[i])
      if (!allocate_sector (&disk_inode->direct[i]))
        return false;
  num_sectors_to_allocate -= i;
  if (num_sectors_to_allocate == 0)
    return true;
  
  if (!disk_inode->indirect)
    if (!allocate_sector (&disk_inode->indirect))
      return false;

  if (!allocate_indirect(disk_inode->indirect,num_sectors_to_allocate))
    return false;
  if (num_sectors_to_allocate > INDIRECT_BLOCK_NO)
    num_sectors_to_allocate -= INDIRECT_BLOCK_NO;
  else
    return true;

  if (num_sectors_to_allocate > INDIRECT_BLOCK_NO*INDIRECT_BLOCK_NO)
    num_sectors_to_allocate = INDIRECT_BLOCK_NO*INDIRECT_BLOCK_NO;


  if (disk_inode->double_indirect == UNALLOCATED)
    if(!allocate_sector(&disk_inode->double_indirect))
      return false;
  block_sector_t dblocks[INDIRECT_BLOCK_NO];
  cache_read (fs_device, disk_inode->double_indirect, &dblocks, 0, BLOCK_SECTOR_SIZE);

  size_t chunk_size;
  size_t required_indirect_sectors = DIV_ROUND_UP (num_sectors_to_allocate, INDIRECT_BLOCK_NO);
  for (i = 0; i < required_indirect_sectors; i++)
    {
      if (num_sectors_to_allocate < INDIRECT_BLOCK_NO)
        chunk_size = num_sectors_to_allocate ;
      else
        chunk_size = INDIRECT_BLOCK_NO;
      if (dblocks[i] == UNALLOCATED)
        if(!allocate_sector (&dblocks[i]))
          return false;
      if (!allocate_indirect (dblocks[i], chunk_size))
        return false;
      num_sectors_to_allocate -= chunk_size;
    }

  cache_write (fs_device, disk_inode->double_indirect, &dblocks, 0, BLOCK_SECTOR_SIZE);

  if (num_sectors_to_allocate <= INDIRECT_BLOCK_NO * INDIRECT_BLOCK_NO)
    return true;
  return false;
  ASSERT (num_sectors_to_allocate == 0);
  return false;
}

bool 
allocate_indirect(block_sector_t sector_idx, size_t num_sectors_to_allocate)
{
  block_sector_t indirect_blocks[INDIRECT_BLOCK_NO];
  cache_read (fs_device, sector_idx, &indirect_blocks, 0, BLOCK_SECTOR_SIZE);
  for (size_t i = 0; i < num_sectors_to_allocate && i < INDIRECT_BLOCK_NO; i++)
    if (indirect_blocks[i] == UNALLOCATED)
      if (!allocate_sector (&indirect_blocks[i]))
        return false;
  cache_write (fs_device, sector_idx, &indirect_blocks, 0, BLOCK_SECTOR_SIZE);
  return true;
}

static bool
allocate_sector (block_sector_t *sector_idx)
{
  static char zeros[BLOCK_SECTOR_SIZE];
  if (free_map_allocate(1, sector_idx))
    {
      cache_write (fs_device, *sector_idx, zeros, 0, BLOCK_SECTOR_SIZE);
      return true;
    }
  return false;
}



/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e))
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector)
        {
          inode_reopen (inode);
          return inode;
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  lock_init (&inode->f_lock);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Returns INODE's remove status. */
bool
inode_get_removed (const struct inode *inode)
{
  return inode->removed;
}

/* Returns INODE DISK's is_dir. */
bool
inode_disk_isdir (const struct inode_disk *disk_inode)
{
  return disk_inode->is_dir;
}

bool
inode_isdir (const struct inode *inode)
{
  if (inode == NULL)
    return false;
  struct inode_disk *disk_inode = get_inode_disk(inode);
  if (disk_inode == NULL)
    return false;
  bool result = inode_disk_isdir (disk_inode);
  free(disk_inode);
  return result;
}


/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode)
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);

      /* Deallocate blocks if removed. */
      if (inode->removed)
        {
          free_map_release (inode->sector, 1);
          inode_disk_deallocate(inode);
        }


      free (inode);
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode)
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset)
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;

  lock_acquire (&inode->f_lock);

  while (size > 0)
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      cache_read (fs_device, sector_idx, (void *)(buffer + bytes_read),
                  sector_ofs, chunk_size);
 

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }

  lock_release (&inode->f_lock);

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;

  if (inode->deny_write_cnt)
    return 0;

  lock_acquire (&inode->f_lock);

  if (byte_to_sector (inode, offset + size-1) == (size_t) -1)
   {
    struct inode_disk *id = get_inode_disk (inode);
    if (!inode_disk_allocate (id, offset+size))
    {
      free (id);
      return bytes_written;
    }
    
    id->length = size + offset;
    cache_write (fs_device, inode_get_inumber (inode), (void *) id, 0,BLOCK_SECTOR_SIZE);
    free (id);
   }
  while (size > 0)
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      cache_write (fs_device, sector_idx, (void *)(buffer + bytes_written),
                   sector_ofs, chunk_size);

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }

  lock_release (&inode->f_lock);

  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode)
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode)
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  struct inode_disk *id = get_inode_disk(inode);
  off_t length = id->length;
  free(id);
  return length;
}
static bool
inode_disk_deallocate (struct inode *inode)
{
  struct inode_disk *disk_inode = get_inode_disk (inode);

  if (disk_inode == NULL)
    return true;

  size_t num_sectors_to_allocate = bytes_to_sectors (disk_inode->length);
  size_t i;
  for (i = 0; i < num_sectors_to_allocate && i < DIRECT_BLOCK_NO; i++)
    free_map_release (disk_inode->direct[i],1);
  num_sectors_to_allocate -= i;
  if (num_sectors_to_allocate == 0)
    {
      free (disk_inode);
      return true;
    }

  if (!deallocate_indirect (disk_inode->indirect, num_sectors_to_allocate))
    return false;
  i = num_sectors_to_allocate < INDIRECT_BLOCK_NO ? num_sectors_to_allocate : INDIRECT_BLOCK_NO;
  num_sectors_to_allocate -= i;
  if (num_sectors_to_allocate == 0)
    {
      free(disk_inode);
      return true;
    }

  block_sector_t blocks[INDIRECT_BLOCK_NO];
  cache_read (fs_device, disk_inode->double_indirect, &blocks, 0, BLOCK_SECTOR_SIZE);

  if (num_sectors_to_allocate > INDIRECT_BLOCK_NO * INDIRECT_BLOCK_NO)
    return false;

  size_t no_blocks = DIV_ROUND_UP (num_sectors_to_allocate, INDIRECT_BLOCK_NO);
  for (i = 0; i < no_blocks; i++)
    {
      block_sector_t double_indirect_blocks[INDIRECT_BLOCK_NO];
      cache_read (fs_device, blocks[i], &double_indirect_blocks, 0, BLOCK_SECTOR_SIZE);
      size_t j;
      for (j = 0; j < num_sectors_to_allocate && j < INDIRECT_BLOCK_NO; j++)
        free_map_release (double_indirect_blocks[j], 1); 
      free_map_release (blocks[i], 1);
      num_sectors_to_allocate -= j;
    }
  free_map_release (disk_inode->double_indirect, 1);
  free (disk_inode);
  return true;
}

static bool deallocate_indirect (block_sector_t sector_num, size_t num_sectors_to_allocate)
{
  block_sector_t indirect_blocks[INDIRECT_BLOCK_NO];
  cache_read (fs_device, sector_num, &indirect_blocks, 0, BLOCK_SECTOR_SIZE);
  for (size_t i = 0; i < num_sectors_to_allocate && i < INDIRECT_BLOCK_NO; i++)
    free_map_release (indirect_blocks[i], 1);
  free_map_release (sector_num, 1);
  return true;
}
struct lock *inode_lock (struct inode *inode)
{
  ASSERT (inode);
  return &(inode->f_lock);
}




