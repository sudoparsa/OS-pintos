# سیستم‌های عامل - تمرین گروهی دوم

## مشخصات گروه

>> نام، نام خانوادگی و ایمیل خود را در ادامه وارد کنید.

پارسا حسینی <sp.hoseiny@gmail.com>

علیرضا دهقانپور فراشاه <alirezafarashah@gmail.com>

آرمان بابایی <292arma@gmail.com>

مهدی سلمانی صالح‌آبادی <m10.salmani@gmail.com>

## مقدمه

>> اگر نکته‌ای درباره فایل‌های سابمیت شده یا برای TAها دارید، لطفا اینجا بیان کنید.

>> اگر از هر منبع برخط یا غیر برخطی به غیر از مستندات Pintos، متن درس، اسلایدهای درس یا نکات گفته شده در کلاس در تمرین گروهی استفاده کرده‌اید، لطفا اینجا آن(ها) را ذکر کنید.

## ساعت زنگ‌دار

### داده ساختارها

>> پرسش اول: تعریف `struct`های جدید، `struct`های تغییر داده شده، متغیرهای گلوبال یا استاتیک، `typedef`ها یا `enumeration`ها را در اینجا آورده و برای هریک در 25 کلمه یا کمتر توضیح بنویسید.

```c
// devices/timer.h

/* List of threads which has been put to sleep for some ticks. This list
 * should be kept ascending corresponding to tick the thread wants to
 * be woken in.
 */
struct list slept_threads;

/* Used to keep slept_threads consistent during concurrent calls.
 * slept_threads should not be modified without acquiring this lock.
 */
struct lock sleep_lock;
```

```c
// threads/thread.h

struct thread
{
  ...
  /* Used for slept_threads. */
  struct list_elem slept_elem;

  /* This is supposed to be on what tick (since the start of OS)
   * this thread wants to be woken up in.
   */
  int64_t waking_tick;
  ...
};
```

### الگوریتم

>> پرسش دوم: به اختصار آن‌چه هنگام صدا زدن تابع `timer_sleep()` رخ می‌دهد و همچنین اثر `timer interrupt handler` را توضیح دهید.

موارد زیر برای timer_sleep رخ می‌دهند.
* زمانی که ریسه باید بیدار شود (نسبت به شروع سیستم عامل) محاسبه می‌شود.
* قفل ریسه‌های خواب دریافت می‌شود.
* در آخرین محل ممکن به طوری که ترتیب صعودی در لیست حفظ شود، ریسه به لیست slept_threads اضافه می‌شود.
* وقفه‌های سیستم عامل متوقف می‌شوند.
* ریسه بلوکه می‌شود.
* وقفه‌ها فعال می‌شوند.
* قفل ریسه‌های خواب آزاد می‌شود.

در طرف دیگر برای timer interrupt handler باید مطابق زیر برنامه‌ریزی شود.
* با دریافت قفل ریسه‌های خواب، اولین این ریسه‌ها (در صورت وجود) بررسی می‌شود.
* با غیرفعال کردن وقفه‌ها تا زمانی که نیاز به بیدار کردن ریسه‌ها وجود دارد، ریسه‌ها از لیست ریسه‌های خواب خارج و آنبلوکه می‌شوند.
* وقفه‌ها فعال می‌شوند.
* قفل ریسه‌های خواب آزاد می‌شود.

>> پرسش سوم: مراحلی که برای کوتاه کردن زمان صرف‌شده در `timer interrupt handler` صرف می‌شود را نام ببرید.

* صعودی نگاه داشتن لیست ریسه‌های خواب باعث می‌شود حداکثر یک ریسه که نیاز به بیدار شدن ندارد را بررسی کنیم.

### همگام‌سازی

>> پرسش چهارم: هنگامی که چند ریسه به طور همزمان `timer_sleep()` را صدا می‌زنند، چگونه از `race condition` جلوگیری می‌شود؟

* قفل استفاده‌شده برای ریسه‌های خواب جلوی مسابقه را می‌گیرد.

>> پرسش پنجم: هنگام صدا زدن `timer_sleep()` اگر یک وقفه ایجاد شود چگونه از `race condition` جلوگیری می‌شود؟

* توجه کنید که قبل از این که می‌خواهیم تغییری در ریسه‌های خواب بدهیم وقفه‌ها را غیرفعال می‌کنیم، پس وقفه‌های اعلام‌شده تاثیری بر عملکرد این قسمت نمی‌گذارند.

### منطق

>> پرسش ششم: چرا این طراحی را استفاده کردید؟ برتری طراحی فعلی خود را بر طراحی‌های دیگری که مدنظر داشته‌اید بیان کنید.

می‌توانستیم به جای این که لیست را صعودی نگه داریم، لیست را به ترتیب دلخواه نگاه‌داریم و تردی که زودترین زمان اجرا را میان ریسه‌های خوابیده دارد نگه داریم. این ریسه در timer interrupt handler پس از استفاده از ترد فعلی به روز می‌شود.
از لحاظ پیچیدگی، به نظر می‌رسد که به هر حال به ازای اجرا/توقف هر ریسه یعنی یک بار صدا زدن `timer_sleep` یک بار لیست ریسه‌های خواب بررسی می‌شود. به این ترتیب تفاوت چندانی میان بهینه بودن در این دو روش به چشم نمی‌خورد. با این وجود، در شرایط خاص هر یک از این دو روش می‌تواند از لحاظ کارایی به دیگری برتری داشته باشد.

آن‌چه که باعث شد روش کنونی انتخاب شود امکان دسترسی به اطلاعات زمانی ترد‌ها به طور مرتب و منسجم در کاربری‌های آینده بود. به عبارت دیگر، روشی که به عنوان جایگزین پیشنهاد دادیم چندان **تمیز** به نظر نمی‌رسد.

همچنین می‌توانستیم به جای استفاده از لاک، تنها از غیرفعال کردن وقفه‌ها برای جلوگیری از مسابقه استفاده کنیم. توجه کنید که این روش منجر به غیرفعال کردن وقفه‌ها برای مدت زمان O(n) دارد که مطلوب نیست.

## زمان‌بند اولویت‌دار

### داده ساختارها

>> پرسش اول: تعریف `struct`های جدید، `struct`های تغییر داده شده، متغیرهای گلوبال یا استاتیک، `typedef`ها یا `enumeration`ها را در اینجا آورده و برای هریک در ۲۵ کلمه یا کمتر توضیح بنویسید.

```c
// threads/thread.h

...

/* Base value for lock's priority. */
# define BASE_PRIORITY -1

/* Maximum depth for donation. */
# define NESTED_DONATION_DEPTH 10

...

struct thread
{
    ...

    /* Thread's original priority. */
    int base_priority;

    /* Thread's effective priority after donation. */
    int effective_priority;

    /* All locks a thread holds, ordered by the highest thread priority. 
     * Used to set thread's effective priority. 
     */
    struct list held_locks;

    /* Keeps track of the lock that has been blocking the thread, if there is any.
     * Used in recursive donations. 
     */
    struct lock *wait_lock;

    ...
};

```

```c
// threads/synch.h

struct lock
{
    ...

    /* Highest priority of threads which holds this lock
     * or has been blocked by this lock.
     * Initialized value is BASE_PRIORITY.
     */
    int priority;

    /* Elem for held locks list. */
    struct list_elem elem;

    ...
};

```




>> پرسش دوم: داده‌ساختارهایی که برای اجرای `priority donation` استفاده شده‌است را توضیح دهید. (می‌توانید تصویر نیز قرار دهید)

داده ساختارهایی که در قسمت قبل گفته شدند برای
`donation`
استفاده می‌شوند. هنگامی که ترد یک قفل جدید را می‌گیرد، این قفل جدید به لیست قفل‌های آن یعنی
`held_locks`
اضافه می‌شود. متغیر
`elem`
در قفل هم برای قابلیت لیست‌سازی از قفل با توجه به پیاده‌سازی لیست پیوندی
`pintos`
است.

هنگامی که ترد
`T1`
یک قفل جدید را می‌خواهد بگیرد اما قفل از قبل در اختیار یک ترد دیگر
(`T2`)
باشد، ترد
`T1`
بلاک می‌شود. حالا ابتدا برای ترد
`T1`
مقدار
`wait_lock`
ست می‌شود، و بعد شرایط
`donation`
چک می‌شود. یعنی چک می‌شود که آیا اولویت
`T1`
بیشتر از
`T2`
هست یا نه. اگر بیشتر بود، مقدار
`T2->effective_priority`
برابر مقدار
`T1->effective_priority`
ست می‌شود. سپس برای مدیریت دونیشن‌های تو در تو، به
`T2->wait_lock`
را بررسی می‌کنیم. اگر این قفل وجود داشت، باید عملیات دونیشن برای ترد صاحب این قفل هم بررسی شود، لذا دوباره این کار تکرار می‌شود. برای جلوگیری از خطاهای احتمالی این کار حداکثر به اندازه
`NESTED_DONATION_DEPTH`
بار تکرار می‌شود. برای مثال فرض کنید ترد
`T1`
قفل
`L1`
را داشته باشد و با قفل
`L2`
بلاک شده باشد، و ترد
`T2`
قفل
`L2`
را داشته باشد و با قفل
`L1`
بلاک شده باشد. در این صورت بدون وجود
`NESTED_DONATION_DEPTH`
الگوریتم ممکن است در حلقه بی نهایت بیافتد.

### الگوریتم

>> پرسش سوم: چگونه مطمئن می‌شوید که ریسه با بیشترین اولویت که منتظر یک قفل، سمافور یا `condition variable` است زودتر از همه بیدار می‌شود؟

دقت کنید که در پیاده‌سازی سمافور، قفل و
`condition variable`
از تابع
`sema_up`
استفاده می‌شود. برای همین با تغییر این تابع که اولویت را چک بکند مشکل تمام این منابع همزمان حل می‌شود.
یعنی تنها کافی است در تابع
`sema_up`
ترد با بیشترین اولویت از لیست
`waiters`
انتخاب و آنبلاک شود.

>> پرسش چهارم: مراحلی که هنگام صدازدن `lock_acquire()` منجر به `priority donation` می‌شوند را نام ببرید. دونیشن‌های تو در تو چگونه مدیریت می‌شوند؟

فرض کنید که ترد
`T1`،
می‌خواهد قفل
`L1`
را
`acquire`
کند. در ادامه مراحل
`donation`
و مدیریت برای حالت تو در تو را توضیح می‌دهیم.

در ابتدا تمام وقفه‌ها غیرفعال می‌شوند. حالا چک می‌کنیم که آیا 
`L1`
در اختیار ترد دیگری هست یا خیر. اگر قفل آزاد باشد که اصلا
`donation`
نداریم. تنها
`sema_down`
صدا زده می‌شود و سپس
`T1`
به عنوان
`holder`
آن ست شده که یعنی ترد قفل را گرفته است. اما اگر
`L1`
در اختیار ترد دیگری مثل
`T2`
بود، مقادیر
`T1->effective_priority`
و
`T2->effective_priority`
مقایسه می‌شوند. در صورتی که
`T1->effective_priority`
کمتر باشد یعنی باز هم
`donation`
نداریم، پس دوباره
`sema_down`
صدا شده و سپس
`T1`
به عنوان
`holder`
قفل ست می‌شود. اما در صورتی که
`T1->effective_priority`
بیشتر بود، یعنی
`donation`
داریم. پس در این حالت اول اولویت
`T2`
را برابر اولویت
`T1`
قرار می‌دهیم. یعنی:

`T2->effective_priority = T1->effective_priority`

و حالا 
`sema_down`
صدا شده و سپس
`T1`
به عنوان
`holder`
قفل ست می‌شود.

برای حل مشکل
`donation`
تو در تو، در صورتی که
`T2->wait_lock`
وجود داشت و ترد
`T3`
صاحب آن بود، مرحله بالا این بار با
`T2`
و
`T3`
تکرار می‌شود. اینکار به دلیل مشکلی که در بخش دوم گفته شد حداکثر به اندازه
`NESTED_DONATION_DEPTH`
بار تکرار می‌شود.

در انتها وقفه‌ها را دوباره فعال می‌کنیم.

>> پرسش پنجم: مراحلی که هنگام صدا زدن `lock_release()` روی یک قفل که یک ریسه با اولویت بالا منتظر آن است، رخ می‌دهد را نام ببرید.

دوباره فرض کنید که ترد
`T1`،
می‌خواهد قفل
`L1`
را
`release`
کند. در ادامه مراحل را برای آن توضیح می‌دهیم.

باز هم اول وقفه‌ها را غیرفعال می‌کنیم. بعد مقدار
`holder`
برای
`L1`
را نال می‌گذاریم. حالا
`sema_up`
صدا زده شده که باعث می‌شود قفل بتواند توسط
`waiter`
بعدیش گرفته شود.
سپس اولین خانه لیست
`held_locks`
که بزرگترین اولویت هم است را چک می‌کنیم. در صورتی که برابر
`BASE_PRIORITY`
بود مقدار اولویت را به حالت اولیه برمی‌گردانیم یعنی:

`T1->effective_priority = T1->base_priority`

و در غیر این صورت:

`T1->effective_priority = held_locks[0]->priority`

در انتها وقفه‌ها را دوباره فعال می‌کنیم.

### همگام‌سازی

>> پرسش ششم: یک شرایط احتمالی برای رخداد `race condition` در `thread_set_priority` را بیان کنید و توضیح دهید که چگونه پیاده‌سازی شما از رخداد آن جلوگیری می‌کند. آیا می‌توانید با استفاده از یک قفل از رخداد آن جلوگیری کنید؟

هنگامی که در قسمت قبل مقدار اولویت برای تردها آپدیت می‌شود، ممکن است خود ترد هم بخواهد اولویتش را عوض کند. برای همین ممکن است هنگام آپدیت کردن اولویت به مقدار نامعتبری ست شود. برای حل این مشکل قبل از هر کاری وقفه‌ها را غیرفعال می‌کنیم.

### منطق

>> پرسش هفتم: چرا این طراحی را استفاده کردید؟ برتری طراحی فعلی خود را بر طراحی‌های دیگری که مدنظر داشته‌اید بیان کنید.

در این طراحی سعی کردیم بهینه ترین و ساده ترین روش ها را داشته باشیم.
برای مثال برای priority scheduling در سمافور لیست waiters را در ابتدا میخواستیم sort شده نگه داریم و هر دفعه ترد با بیشترین اولویت را 
unblock کنیم ولی چون ممکن بود یک ترد اولویت آن تفییر کرده باشد نسبت به حالتی که در ابتدا در این لیست قرار داده شده است
تصمیم گرفتیم هر بار از کل لیست با پیمایش آن ترد با بیشترین اولویت را یافت کنیم و آن را unblock کنیم تا 
scheduler
آن را انتخاب کند.

## سوالات افزون بر طراحی

>> پرسش هشتم: در کلاس سه صفت مهم ریسه‌ها که سیستم عامل هنگامی که ریسه درحال اجرا نیست را ذخیره می‌کند، بررسی کردیم:‍‍ `program counter` ، ‍‍‍`stack pointer` و `registers`. بررسی کنید که این سه کجا و چگونه در `Pintos` ذخیره می‌شوند؟ مطالعه ‍`switch.S` و تابع ‍`schedule` در فایل `thread.c` می‌تواند مفید باشد.

در تابع 
schedule
 با فراخوانی 
 switch_threads
اشاره گرها به ساختار ترد فعلی و بعدی و آدرس بازگشت در پشته ی هسته مخصوص آن ترد ذخیره میشود.


```

switch_threads:

	pushl %ebx
	pushl %ebp
	pushl %esi
	pushl %edi

.globl thread_stack_ofs
	mov thread_stack_ofs, %edx

	movl SWITCH_CUR(%esp), %eax
	movl %esp, (%eax,%edx,1)

	movl SWITCH_NEXT(%esp), %ecx
	movl (%ecx,%edx,1), %esp

	popl %edi
	popl %esi
	popl %ebp
	popl %ebx
        ret
.endfunc

```
مقدار
ثبات ها در ترد فعلی در پشته قرار داده میشوند.
‍‍
```
	pushl %ebx
	pushl %ebp
	pushl %esi
	pushl %edi
```

در ادامه کد به وسیله 
macro 
ی زیر آدرس عضوری که به پشته ی استک اشاره میکند نسبت به 
ساختار strcut thread
 محاسبه میشود.

 برای این بخش از منبع زیر اسفتاده کردیم.

 https://stackoverflow.com/questions/13723422/why-this-0-in-type0-member-in-c

```
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *) 0)->MEMBER)
```

مقدار خروجی محاسبات بالا در رجیستر 
edx
ذخیره میشود.

در حال حاضر پشته در وضیعت زیر است:
```
edi <- stack pointer

esi <- stack pointer + 4

ebp <- stack pointer + 8

ebx <- stack pointer + 12

return address <- stack pointer + 16

cur <- stack pointer + 20

next <- stack pointer + 24
```
در دو خط زیر در ابتدا مقدار خانه با آدرس 
SWITCH_CUR + esp
را که همان 
اشاره گر به ساختار ترد کنونی است را ذخیره میکنیم.


سپس این مقدار را با
 offset
 بخش قبل جمع میکنیم تاآدرس
current_thread -> stack
 بدست آید و سپس 
 esp
 را در آن قرار میدهیم.

 ```
	movl SWITCH_CUR(%esp), %eax
	movl %esp, (%eax,%edx,1)

 ```

در دو خط زیر مانند قبل اینبار مقدار 
next_thread -> stack
را در رجیستر 
esp
قرار میدهیم.

```
	movl SWITCH_NEXT(%esp), %ecx
	movl (%ecx,%edx,1), %esp

```

در اینجا 
stack pointer
به پشته هسته مخصوص ترد جدید اشاره میکند که از قبل میدانیم مقادیر رجسیترهای مربوط به ترد در پشته مخصوص آن ذخیره شده است.
در هنگام بازگشت ۴ رجیستر ترد و همچنین آدرس بازگشت را از ترد بر میداریم.

```
	popl %edi
	popl %esi
	popl %ebp
	popl %ebx
    ret
```



>> پرسش نهم: وقتی یک ریسه‌ی هسته در ‍`Pintos` تابع `thread_exit` را صدا می‌زند، کجا و به چه ترتیبی صفحه شامل پشته و `TCB` یا `struct thread` آزاد می‌شود؟ چرا این حافظه را نمی‌توانیم به کمک صدازدن تابع ‍`palloc_free_page` داخل تابع ‍`thread_exit` آزاد کنیم؟


 در بخش زیر در 
 thread_exit
 ترد را از لیست همه تردها حذف و حالت آن را تغییر میدهد.

 ```
  intr_disable ();
  list_remove (&thread_current()->allelem);
  thread_current ()->status = THREAD_DYING;
  schedule ();
  NOT_REACHED ();
 ```

حال در schedule
بعد از این که ترد جابجا
(switch)
شد 
تابع 
thread_schedule_tail(prev)
در ترد جدید اضافه میشود.

در این تابع در انتهایش خطوط زیر اجرا میشوند که ترد قبلی اگر در وضعیت
THREAD_DYING
بود آن ترد را از حافظه هسته پاک میکند.

```
  if (prev != NULL && prev->status == THREAD_DYING && prev != initial_thread)
    {
      ASSERT (prev != cur);
      palloc_free_page (prev);
    }
```
اما علت آن که آزادسازی را در 
thread_exit
انجام نمیدهیم این است که هنوز در اجرای ترد فعلی قرار داریم این آزادسازی موجب ایجاد مشکل خواهد شد.

این توضیحات در خود کد نیز به صورت کامنت قرار دارند.

```
 /* If the thread we switched from is dying, destroy its struct
     thread.  This must happen late so that thread_exit() doesn't
     pull out the rug under itself.  (We don't free
     initial_thread because its memory was not obtained via
     palloc().) 
*/
```

>> پرسش دهم: زمانی که تابع ‍`thread_tick` توسط `timer interrupt handler` صدا زده می‌شود، در کدام پشته اجرا می‌شود؟

در پشته هسته نه پشته مخصوص به تردها در هسته به این علت که این یک وقفه است و ما در 
 kernel_mode
قرار داریم.


>> پرسش یازدهم: یک پیاده‌سازی کاملا کاربردی و درست این پروژه را در نظر بگیرید که فقط یک مشکل درون تابع ‍`sema_up()` دارد. با توجه به نیازمندی‌های پروژه سمافورها(و سایر متغیرهای به‌هنگام‌سازی) باید ریسه‌های با اولویت بالاتر را بر ریسه‌های با اولویت پایین‌تر ترجیح دهند. با این حال پیاده‌سازی ریسه‌های با اولویت بالاتر را براساس اولویت مبنا `Base Priority` به جای اولویت موثر ‍`Effective Priority` انتخاب می‌کند. اساسا اهدای اولویت زمانی که سمافور تصمیم می‌گیرد که کدام ریسه رفع مسدودیت شود، تاثیر داده نمی‌شود. تستی طراحی کنید که وجود این باگ را اثبات کند. تست‌های `Pintos` شامل کد معمولی در سطح هسته (مانند متغیرها، فراخوانی توابع، جملات شرطی و ...) هستند و می‌توانند متن چاپ کنند و می‌توانیم متن چاپ شده را با خروجی مورد انتظار مقایسه کنیم و اگر متفاوت بودند، وجود مشکل در پیاده‌سازی اثبات می‌شود. شما باید توضیحی درباره این که تست چگونه کار می‌کند، خروجی مورد انتظار و خروجی واقعی آن فراهم کنید.

برای اینکه این تست را پیاده سازی کنیم 4 ترد را مشابه زیر پیاده سازی میکنیم.
(شبه کد)

```
semaphore sema;
lock Lock;
int main()
  {
    Lock = lock_init ()
    sema = sema_init (0)
    set_prioriry (1)
    thread_create (A, priority=2)
    thread_yield ()
    thread_create (B, priority=3)
    thread_yield ()
    thread_create (C, priority=4)
    thread_yield ()
    sema_up (sema)
  }
```

```
void A()
  {
    lock_acquire (Lock)
    sema_down (sema)
    printf("A\n")
    sema_up (sema)
    lock_release (Lock)
  }
  
```

```
void B()
  {
    sema_down (sema)
    printf("B\n")
    sema_up (sema)
  }
```

```
void C()
  {
    lock_acquire (Lock)
    lock_release (Lock)
  }
```

توضیحات:
در این تست ابتدا تابع 
main
فراخوانی میشود. سپس ترد
A
ساخته میشود و با استفاده از تابع 
thread_yeild
و با فرض 
priority scheduling
ترد 
A
اجرا میشود.
(چون اولویت بالاتری از ترد اصلی دارد.)
پس از به دست آوردن قفل تعریف شده در ترد اصلی، ترد A توسط 
sema_down
بلاک میشود.
پس از اجرای مجدد ترد اصلی به طریق مشابه ترد B ساخته میشود و مانند ترد A توسط
sema_down
بلاک میشود.
سپس با ادامه اجرای ترد اصلی ترد 
C
ساخته میشود که میخواهد قفلی را که ترد A به دست آورده بود به دست آورد.
این کار باعث اهدای اولویت 3 به ترد A
میشود.
دقت کنید در این حالت ترد C نیز بلاک میشود.
حال در ادامه اجرای ترد اصلی تابع
sema_up
فراخوانی میشود.

تا اینجا تست با خروجی غلط مانند تست با خروجی درست عمل میکند ولی در این جا چون براساس
`Base Priority`
تردها در سمافور آنبلاک میشنود پس اول ترد 
B
اجرا میشود و مقدار `B`
را چاپ میکند و پس از فراخوانی sema_up
توسط این ترد، 
ترد A
اجرا میشود و مقدار `A` نیز چاپ میشود.
در حالیکه در حالت درست چون براساس
`Effective Priority`
ترد ها در sema_up
آنبلاک میشوند شاهد خروجی 
`A`
و سپس 
`B`
خواهیم بود.

خروجی مورد انتظار:
```
A
B
```

خروجی واقعی:
```
B
A
```

## سوالات نظرسنجی

پاسخ به این سوالات دلخواه است، اما به ما برای بهبود این درس در ادامه کمک خواهد کرد. نظرات خود را آزادانه به ما بگوئید—این سوالات فقط برای سنجش افکار شماست. ممکن است شما بخواهید ارزیابی خود از درس را به صورت ناشناس و در انتهای ترم بیان کنید.

>> به نظر شما، این تمرین گروهی، یا هر کدام از سه وظیفه آن، از نظر دشواری در چه سطحی بود؟ خیلی سخت یا خیلی آسان؟

>> چه مدت زمانی را صرف انجام این تمرین کردید؟ نسبتا زیاد یا خیلی کم؟

>> آیا بعد از کار بر روی یک بخش خاص از این تمرین (هر بخشی)، این احساس در شما به وجود آمد که اکنون یک دید بهتر نسبت به برخی جنبه‌های سیستم عامل دارید؟

>> آیا نکته یا راهنمایی خاصی وجود دارد که بهتر است ما آنها را به توضیحات این تمرین اضافه کنیم تا به دانشجویان ترم های آتی در حل مسائل کمک کند؟

>> متقابلا، آیا راهنمایی نادرستی که منجر به گمراهی شما شود وجود داشته است؟

>> آیا پیشنهادی در مورد دستیاران آموزشی درس، برای همکاری موثرتر با دانشجویان دارید؟

این پیشنهادات میتوانند هم برای تمرین‌های گروهی بعدی همین ترم و هم برای ترم‌های آینده باشد.

>> آیا حرف دیگری دارید؟