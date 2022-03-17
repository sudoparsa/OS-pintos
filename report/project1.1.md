<div dir="rtl">

تمرین گروهی ۱/۱ - آشنایی با pintos

======================

شماره گروه: 6

-----

> نام و آدرس پست الکترونیکی اعضای گروه را در این قسمت بنویسید.

آرمان بابایی <292arma@gmail.com>

مهدی سلمانی صالح آبادی <m10.salmani@gmail.com> 

پارسا حسینی <sp.hoseiny@gmail.com> 

علیرضا دهقانپور <alirezafarashah@gmail.com>  


## داده ساختار ها
<div dir='ltr'>

```c
struct child_parent_status
  {
    pid_t pid;

    int exit_code;

    bool is_finished;

    struct list_elem elem;

    struct semaphore sema; // init to 0

    int ref_count; // at first it is 2 showing number of threads working with this status

    struct lock lock; // for locking ref_count

  };

struct fn_cps
  {
    char *fn;
    struct child_parent_status *cps;
    bool success;
  };

struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority. */
    struct list_elem allelem;           /* List element for all threads list. */

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */

#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */
#endif

    /* Owned by userprog/syscall.c */
    struct file* file_descriptors[MAX_FILE_DESCRIPTORS];
    struct file* process_file;
    struct list children;
    struct child_parent_status *cps;
    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
  };
``` 
</div>

در این بخش برای نگه داری فایل ها به جای 
struct
معرفی شده در فاز قبلی از 
file *
استفاده کردیم که به صورت مستقیم به فایل دسترسی داشتیم و به اطلاعات دیگر نیازی نبود.

همچنین برای 
child_parent_status
یک متغیر 
bool
برای مشخص شدن تمام شدن ترد قرار دادیم.

علاوه بر این تغیرات یک داده ساختار به نام 
fn_cps
تعریف کردیم که برای ورودی دادن به 
start_process
در فایل 
process.c
استفاده کردیم.

در ساختار ترد نیز یک
file *
جدا برای فایل اجرایی گرفتیم ولی در طراحی قبلی میخواستیم از 
file_descriptors[2]
به این عنوان استفاده کنیم.


## الگوریتم
در بخش پاس دادن آرگومان ها علاوه بر این که به صورت ۴ بایت ۴ بایت 
align
کردیم یک فضای خالی هم اضافه کردیم که به صورت ۱۶ بایتی نیز 
align 
شود.


## مسئولیت هر فرد
به صورت گروهی جلساتی برگزار کردیم و یک فرد در جلسه کد زنی میکرد.
برخی از بخش ها نیز که در جلسات نحوه پیاده سازی آن ها مشخص میشد را به صورت تکی پیاده سازی کردیم.

