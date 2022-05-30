<div dir="rtl">

## گزارش تمرین گروهی ۳


شماره گروه: 6

-----

> نام و آدرس پست الکترونیکی اعضای گروه را در این قسمت بنویسید.

آرمان بابایی <292arma@gmail.com>

مهدی سلمانی صالح آبادی <m10.salmani@gmail.com> 

پارسا حسینی <sp.hoseiny@gmail.com> 

علیرضا دهقانپور <alirezafarashah@gmail.com>  

# بافرکش
## داده ساختار ها
<div dir='ltr'>

```c
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
``` 
بعد از جلسه طراحی، تصمیم گرفتیم از بین
`LRU`
و
`second chance`
الگوریتم
`LRU`
را برای جایگزین کردن بلاک در کش استفاده کنیم. برای همین لیستی نگه داشتیم.


## الگوریتم
برای پیاده‌سازی این بخش مشابه داک طراحی عمل کردیم. فایل‌های
`cache.c`
و
`cache.h`
مربوط به این قسمت هستنند. تابع
`get_cache_block`
قسمت اصلی این بخش است که پیاده‌سازی شده است. این  تابع شماره
`sector‍`
و یک بولین که نشاندهنده خواندن یا نوشتن است را ورودی می‌گیرد.
اگر این بلاک در کش بود، اول جای بلاک را در لیست با توجه به روش
`LRU`
آپدیت می‌کند و آن بلاک را هم خروجی می‌دهد.
اما اگر این بلاک در کش نبود، باید از حافظه اصلی آورده شود، و با یک بلاک در کش جایگزین شود.
همچنین این بلاک که قرار است از کش حذف شود، باید اگر
`dirty`
شده مقدار بلاک اصلی حافظه آپدیت شود.

نکته دیگری که هنگام بررسی تست‌های
`persistence`
متوجه شدیم، این است که ممکن بود سیستم خاموش شود. برای همین تابع
`cache_shutdown`
را اضافه کردیم که قبل از خاموش شدن تغییرات در حافظه اصلی هم اعمال شوند.

## تست‌های پیاده‌سازی شده
برای تست‌ها ابتدا ۵ تا
`syscall`
اضافه کردیم که بتوان کش را
`invalidate`
کرد و تعداد
`read, write, hit, miss`
را گزارش کرد.

تست
`cache-hitrate`: 
یک فایل ۱۰ گیگابایتی درست کردیم و داده درون آن ریخته و بلوکی خواندیم.
بعد
`miss, hit`
را به دست آوردیم. این کار را دوباره تکرار کردیم و در انتها چک کردیم که مقدار
‍`miss/hit`
بار دوم کمتر شده باشد. دقت کنید که اگر پیاده‌سازی درست باشد این اتفاق باید بیافتد.

تست
`cache-write`:
در این تست می‌خواهیم این نکته را بررسی کنیم که هروقت می‌خواهیم بلوکی را کامل بنویسیم، اول از حافظه نخوانیم و مستقیم بلوک را بنویسیم.
یک فایل ۶۴ کیلوبایته شامل ۱۲۸ بلاک درست کردیم که از کش بزرگتر است.
حالا این  فایل را در حافظه می‌ریزیم و تعداد
`read, write`
را می‌شماریم.
در انتها چک می‌کنیم که تعداد
`read`
انجام شده زیاد نباشد، که به این معنی است که هنگام ریختن بلاک خواندن بیهوده انجام نداده باشیم.

همین دو تست را در حالت
`persistence`
که سیستم خاموش می‌شد را هم اضافه کردیم.


خروجی تست‌ها

‍‍`cache-hitrate.output`

Copying tests/filesys/extended/cache-hitrate to scratch partition...
Copying tests/filesys/extended/tar to scratch partition...
qemu-system-i386 -device isa-debug-exit -hda /tmp/KYvrjfXQ6m.dsk -hdb tmp.dsk -m 4 -net none -nographic -monitor null
PiLo hda1
Loading............
Kernel command line: -q -f extract run cache-hitrate
Pintos booting with 3,968 kB RAM...
367 pages available in kernel pool.
367 pages available in user pool.
Calibrating timer...  214,630,400 loops/s.
hda: 1,008 sectors (504 kB), model "QM00001", serial "QEMU HARDDISK"
hda1: 199 sectors (99 kB), Pintos OS kernel (20)
hda2: 243 sectors (121 kB), Pintos scratch (22)
hdb: 5,040 sectors (2 MB), model "QM00002", serial "QEMU HARDDISK"
hdb1: 4,096 sectors (2 MB), Pintos file system (21)
filesys: using hdb1
scratch: using hda2
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'cache-hitrate' into the file system...
Putting 'tar' into the file system...
Erasing ustar archive...
Executing 'cache-hitrate':
(cache-hitrate) begin
(cache-hitrate) create "a" should not fail.
(cache-hitrate) open "a" returned fd which should be greater than 2.
(cache-hitrate) write should write all the data to file.
(cache-hitrate) File creation completed.
(cache-hitrate) First run cache hit rate calculated.
(cache-hitrate) Second run cumulative cache hit rate calculated.
(cache-hitrate) Hit rate should grow.
(cache-hitrate) Removed "a".
(cache-hitrate) end
cache-hitrate: exit(0)
Execution of 'cache-hitrate' complete.
Timer: 118 ticks
Thread: 28 idle ticks, 79 kernel ticks, 11 user ticks
hdb1 (filesys): 66 reads, 513 writes
hda2 (scratch): 242 reads, 2 writes
Console: 1420 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...

`cache-hitrate.result`

PASS

`cache-hitrate-persistence.output`

qemu-system-i386 -device isa-debug-exit -hda /tmp/F6w95Cgq93.dsk -hdb tmp.dsk -m 4 -net none -nographic -monitor null
PiLo hda1
Loading............
Kernel command line: -q run 'tar fs.tar /' append fs.tar
Pintos booting with 3,968 kB RAM...
367 pages available in kernel pool.
367 pages available in user pool.
Calibrating timer...  199,884,800 loops/s.
hda: 3,024 sectors (1 MB), model "QM00001", serial "QEMU HARDDISK"
hda1: 199 sectors (99 kB), Pintos OS kernel (20)
hda2: 2,048 sectors (1 MB), Pintos scratch (22)
hdb: 5,040 sectors (2 MB), model "QM00002", serial "QEMU HARDDISK"
hdb1: 4,096 sectors (2 MB), Pintos file system (21)
filesys: using hdb1
scratch: using hda2
Boot complete.
Executing 'tar fs.tar /':
tar: exit(0)
Execution of 'tar fs.tar /' complete.
Appending 'fs.tar' to ustar archive on scratch device...
Timer: 169 ticks
Thread: 59 idle ticks, 76 kernel ticks, 35 user ticks
hdb1 (filesys): 529 reads, 248 writes
hda2 (scratch): 0 reads, 247 writes
Console: 823 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...
Copying tests/filesys/extended/cache-hitrate.tar out of /tmp/F6w95Cgq93.dsk...

`cache-hitrate-persistence.result`

PASS

`cache-write.output`

Copying tests/filesys/extended/cache-write to scratch partition...
Copying tests/filesys/extended/tar to scratch partition...
qemu-system-i386 -device isa-debug-exit -hda /tmp/ijVhOof_X7.dsk -hdb tmp.dsk -m 4 -net none -nographic -monitor null
PiLo hda1
Loading............
Kernel command line: -q -f extract run cache-write
Pintos booting with 3,968 kB RAM...
367 pages available in kernel pool.
367 pages available in user pool.
Calibrating timer...  183,500,800 loops/s.
hda: 1,008 sectors (504 kB), model "QM00001", serial "QEMU HARDDISK"
hda1: 199 sectors (99 kB), Pintos OS kernel (20)
hda2: 241 sectors (120 kB), Pintos scratch (22)
hdb: 5,040 sectors (2 MB), model "QM00002", serial "QEMU HARDDISK"
hdb1: 4,096 sectors (2 MB), Pintos file system (21)
filesys: using hdb1
scratch: using hda2
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'cache-write' into the file system...
Putting 'tar' into the file system...
Erasing ustar archive...
Executing 'cache-write':
(cache-write) begin
(cache-write) create "a" should not fail.
(cache-write) File creation completed.
(cache-write) open "a" returned fd which should be greater than 2.
(cache-write) write completed.
(cache-write) Block write count should be less than 134.
(cache-write) Block read count should be less than acceptable error 6.
(cache-write) Removed "a".
(cache-write) end
cache-write: exit(0)
Execution of 'cache-write' complete.
Timer: 116 ticks
Thread: 34 idle ticks, 70 kernel ticks, 12 user ticks
hdb1 (filesys): 52 reads, 746 writes
hda2 (scratch): 240 reads, 2 writes
Console: 1344 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...

`cache-write.result`

PASS

`cache-write-persistence.output`

qemu-system-i386 -device isa-debug-exit -hda /tmp/r1tE5fI0_c.dsk -hdb tmp.dsk -m 4 -net none -nographic -monitor null
PiLo hda1
Loading............
Kernel command line: -q run 'tar fs.tar /' append fs.tar
Pintos booting with 3,968 kB RAM...
367 pages available in kernel pool.
367 pages available in user pool.
Calibrating timer...  130,662,400 loops/s.
hda: 3,024 sectors (1 MB), model "QM00001", serial "QEMU HARDDISK"
hda1: 199 sectors (99 kB), Pintos OS kernel (20)
hda2: 2,048 sectors (1 MB), Pintos scratch (22)
hdb: 5,040 sectors (2 MB), model "QM00002", serial "QEMU HARDDISK"
hdb1: 4,096 sectors (2 MB), Pintos file system (21)
filesys: using hdb1
scratch: using hda2
Boot complete.
Executing 'tar fs.tar /':
tar: exit(0)
Execution of 'tar fs.tar /' complete.
Appending 'fs.tar' to ustar archive on scratch device...
Timer: 140 ticks
Thread: 38 idle ticks, 79 kernel ticks, 23 user ticks
hdb1 (filesys): 525 reads, 246 writes
hda2 (scratch): 0 reads, 245 writes
Console: 823 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...
Copying tests/filesys/extended/cache-write.tar out of /tmp/r1tE5fI0_c.dsk...

`cache-write-persistence.result`

PASS




# فایل‌های قابل گسترش
## داده ساختار ها
<div dir='ltr'>

```c
#define DIRECT_BLOCK_NO 123
#define INDIRECT_BLOCK_NO 128

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
``` 
این قسمت تفاوتی با طراحی که کردیم ندارد.

## الگوریتم
تغییرات اصلی این بخش در فایل‌های
`inode.c`
و
`inode.h`
هستند.
پیاده‌سازی عینا مشابه طراحی است، که باید طبقه‌بندی چندسطحه داشته باشد.
(به داده ساختار بالا توجه کنید. حالت
`direct`‍،
`indirect`
و
`double indirect`)
توابع
`byte_to_sector`،
`inode_disk_allocate`
و
`inode_disk_deallocate`
بر اساس همین ۳ سطح گفته شده پیاده‌سازی شده‌اند.


# زیرمسیرها
## داده ساختار ها
<div dir='ltr'>

```c
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
``` 

## الگوریتم
این قسمت هم عینا مشابه طراحی که داشتیم پیاده‌سازی شده است.
در واقع
اگر مسیر مطلق داشته باشیم از ریشه شروع میکنیم و به وسیله متد get_next_part که در داک اصلی کد آن آورده شده است نام بخش بعدی به دست می آوریم و تو در تو جلو می رویم. اگر آدرس نسبی باشد به وسیله cwd که در هر ترد تعریف کردیم و آدرس نسبی به آدرس مطلق می رسیم.

## مسئولیت هر فرد
به صورت گروهی جلساتی برگزار کردیم و یک فرد در جلسه کد زنی میکرد.
برخی از بخش ها نیز که در جلسات نحوه پیاده سازی آن ها مشخص میشد را به صورت تکی پیاده سازی کردیم.
در بخش هایی نیز به گروه های دو نفره تقسیم شدیم و به صورت حضوری یا انلاین دو گروه با یکدیگر هماهنگ میشدند.

