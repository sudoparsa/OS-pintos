<div dir="rtl">

## گزارش تمرین گروهی


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
``` 
 یک داده ساختار به نام 
fn_cps
تعریف کردیم که برای ورودی دادن به 
start_process
در فایل 
process.c
استفاده کردیم.
در این داده ساختار 
fn 
را به علاوه 
cps
که یک 
child_parent_status
است و برای ترد فرزند که قرار است ساخته شود نیاز است قرار دارد.

```c
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

در ساختار ترد نیز یک
file *
جدا برای فایل اجرایی گرفتیم ولی در طراحی قبلی میخواستیم از 
file_descriptors[2]
به این عنوان استفاده کنیم.
ولی برای راحتی کار با این روش کافی بود اجازه ی نوشتن در این فایل را منع کنیم.
این کار به پاس کردن تست های 
rox
کمک میکند.
به وسیله 
file_deny_write(process_file)
این کار را انجام میدهیم.

## الگوریتم
در بخش پاس دادن آرگومان ها علاوه بر این که به صورت ۴ بایت ۴ بایت 
align
کردیم یک فضای خالی هم اضافه کردیم که به صورت ۱۶ بایتی نیز 
align 
شود.

برای بررسی 
valid
بودن پوینترهای کاربر تابع 
check_arguments
را پیاده سازی کردیم که در آن با توجه به نوع آرگومان که اگر مثلا 
INT 
بود به اندازه ۴ خانه جلو میرویم و بررسی میشود که باعث 
kernel panic
نشود.

برای پاس کردن درست تستهای 
sync
از یک
lock 
به نام 
global_lock
استفاده کردیم.
به این صورت که در هر 
syscall
که به توابع 
filesys
نیاز داشت قفل را بدست میگیریم و پس از عملیات رو فایل قفل را آزاد میکنیم.
در ادامه یک نمونه را مشاهده میکنید

```c
  lock_acquire(&global_lock);
  f->eax = file_read (trd->file_descriptors[fd], buffer, length);
  lock_release(&global_lock);
```
یکی دیگر از بخش های مهم که در پاس کردن تست 
oom
بسیار حائز اهمیت بود آزاد کردن منابع اشغال شده توسط هر ترد بود.
برای این کار با پایان یافتن هر ترد در 
process_exit
منابع ترد فعلی را آزاد میکنیم و همجنین تمامی فایل های مورد استفاده اش را با دستور آماده 
filesys_close
میبندیم.
همچنین برای تمامی فرزندان آن تعداد ref_count
را یک واحد کاهش میدهیم.
اگر ref_count
برابر ۰ باشد فرزند را نیز آزاد میکنیم.
همچنین ورودی 
thread_create
که به صورت 
void*
است و آن را با داده ساختار 
fn_cps
قرار دادیم باید پس از پایان عملیات ساختن و لود کردن آزاد کنیم.

سیستم کال های 
exec
و
wait
به این صورت پیاده سازی شده اند که به وسیله 
semaphore
موجود در هر ترد فرزند ابتدا فرزند را پیدا میکنیم و سمافور آن را 
sema_down
میکنیم.
در پایان هر ترد در 
syscall_exit
آن را 
sema_up
میکنیم که ترد پدر که منتظر بوده است آزاد شود.

به طور کلی در فایل 
thread.c
سعی کرده ایم تغیری ندهیم و به همین منظور فقط 
initialize
کردن لیست فرزندان را در 
init_thread
انجام میدهیم.

## مسئولیت هر فرد
به صورت گروهی جلساتی برگزار کردیم و یک فرد در جلسه کد زنی میکرد.
برخی از بخش ها نیز که در جلسات نحوه پیاده سازی آن ها مشخص میشد را به صورت تکی پیاده سازی کردیم.
در بخش هایی نیز به گروه های دو نفره تقسیم شدیم و به صورت حضوری یا انلاین دو گروه با یکدیگر هماهنگ میشدند.

