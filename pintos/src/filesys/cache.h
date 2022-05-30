#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include "off_t.h"
#include "devices/block.h"
#include "threads/synch.h"
#include "lib/kernel/list.h"


#define CACHE_SIZE 64


struct cache_block
  {
    block_sector_t sector_num;
    bool valid;
    bool dirty;
    char data[BLOCK_SECTOR_SIZE];
    struct list_elem cache_elem;
    struct lock cache_lock;
  };


struct cache_block cache[CACHE_SIZE];
struct list cache_LRU;
struct lock cache_list_lock;


void cache_init (void);
void cache_shutdown (struct block *fs_device);
void cache_read (struct block *fs_device, block_sector_t sector_idx, void *buffer, off_t offset, int chunk_size);
void cache_write (struct block *fs_device, block_sector_t sector_idx, void *buffer, off_t offset, int chunk_size);
void flush_block (struct block *fs_device, struct cache_block *LRU_block);
int get_cache_index(block_sector_t sector);
struct cache_block *get_cache_block (struct block *fs_device, block_sector_t sector);
void cache_invalidate (struct block *fs_device);

size_t cache_hit_count (void);
size_t cache_miss_count (void);

#endif /* filesys/cache.h */