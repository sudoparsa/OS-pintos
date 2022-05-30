#include <debug.h>
#include <string.h>
#include "cache.h"
#include "filesys/filesys.h"


#define NOTFOUND -1 


struct cache_stat
{
  size_t hit_count;
  size_t miss_count;
};

static struct cache_stat cache_stat;

void
cache_init (void)
{
  lock_init(&cache_list_lock);
  list_init(&cache_LRU);
  
  for (int i=0; i < CACHE_SIZE; i++)
    {
      lock_init(&cache[i].cache_lock);
      cache[i].dirty = false;
      cache[i].valid = false;
      list_push_back (&cache_LRU, &(cache[i].cache_elem));
    }

  cache_stat = (struct cache_stat) {0, 0};
}

void 
cache_shutdown (struct block *fs_device)
{
  for (int i=0; i<CACHE_SIZE; i++)
    {
      if (cache[i].valid == true && cache[i].dirty == true)
         flush_block (fs_device, &cache[i]);
    }
}

void
flush_block (struct block *fs_device, struct cache_block *LRU_block)
{
  block_write(fs_device, LRU_block->sector_num, LRU_block->data);
  LRU_block->dirty = false;
}


int
get_cache_index(block_sector_t sector)
{
  for (int i=0; i < CACHE_SIZE; i++)
    if(cache[i].valid && cache[i].sector_num == sector)
      return i;
  return NOTFOUND;
}


struct cache_block *
get_cache_block (struct block *fs_device, block_sector_t sector)
{
  int index = get_cache_index (sector);
  struct cache_block* LRU_block;
  lock_acquire (&cache_list_lock);
  if (index == NOTFOUND)
    {
      ++ cache_stat.miss_count;
      LRU_block = list_entry (list_pop_front (&cache_LRU), struct cache_block, cache_elem);
      lock_acquire (&LRU_block->cache_lock);

      if (LRU_block->valid && LRU_block->dirty)
        flush_block (fs_device, LRU_block);

      block_read (fs_device, sector, LRU_block->data);
      LRU_block->valid = true;
      LRU_block->dirty = false;
      LRU_block->sector_num = sector;
      list_push_back (&cache_LRU, &LRU_block->cache_elem);

      lock_release (&LRU_block->cache_lock);
    }
  else
    {
      ++ cache_stat.hit_count;
      list_remove (&cache[index].cache_elem);
      list_push_back (&cache_LRU, &cache[index].cache_elem);
      LRU_block = &cache[index];
    }

  lock_release (&cache_list_lock);
  
  return LRU_block;
}


void
cache_read (struct block *fs_device, block_sector_t sector_idx, void *buffer, off_t offset, int chunk_size)
{
  ASSERT (fs_device != NULL);
  ASSERT (offset >= 0 && chunk_size >= 0 && (offset + chunk_size) <= BLOCK_SECTOR_SIZE);
  ASSERT(sector_idx < block_size(fs_device));

  
  struct cache_block* cb = get_cache_block(fs_device, sector_idx);
  lock_acquire(&cb->cache_lock);
  memcpy(buffer, &(cb->data[offset]), chunk_size);

  lock_release(&cb->cache_lock);
}


void cache_write (struct block *fs_device, block_sector_t sector_idx, void *buffer, off_t offset, int chunk_size)
{        
  ASSERT (fs_device != NULL);
  ASSERT (offset >= 0 && chunk_size >= 0 && (offset + chunk_size) <= BLOCK_SECTOR_SIZE);


  struct cache_block* cb = get_cache_block (fs_device, sector_idx);
  lock_acquire(&cb->cache_lock);
  memcpy(&(cb->data[offset]), buffer, chunk_size);
  cb->dirty = true;
  lock_release(&cb->cache_lock);
}


void
cache_invalidate (struct block *fs_device)
{
  cache_shutdown (fs_device);
  for (int i=0; i<CACHE_SIZE; i++)
  {
    cache[i].valid = false;
    cache[i].dirty = false;
  }

  cache_stat = (struct cache_stat) {0, 0};
}


size_t
cache_hit_count (void)
{
  return cache_stat.hit_count;
}

size_t
cache_miss_count (void)
{
  return cache_stat.miss_count;
}
