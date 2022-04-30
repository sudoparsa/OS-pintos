<div dir="rtl">

# گزارش تمرین گروهی


شماره گروه: 6

-----

> نام و آدرس پست الکترونیکی اعضای گروه را در این قسمت بنویسید.

آرمان بابایی <292arma@gmail.com>

مهدی سلمانی صالح آبادی <m10.salmani@gmail.com> 

پارسا حسینی <sp.hoseiny@gmail.com> 

علیرضا دهقانپور <alirezafarashah@gmail.com>  



## بخش اول (ساعت زنگ دار بهینه)

### داده ساختار ها
-----
```c
// devices/thread.c

/* List of threads which has been put to sleep for some ticks. This list
 * should be kept ascending corresponding to tick the thread wants to
 * be woken in.
 */
struct list slept_threads;
```

```c
// threads/thread.h

struct thread
{
  ...
  /* This is supposed to be on what tick (since the start of OS)
   * this thread wants to be woken up in.
   */
  int64_t waking_tick;
  ...
};
```

نسبت به طراحی اولیه لاک مربوط به افزودن به `slept_threads` را حذف کردیم.
چون این لاک قرار بود در موقع interrupt استفاده شود ولی همانطور که در داک و کد مشخص شده است
در هنگام interrupt نمیوان از لاک استفاده کرد. 
(با assertion)

همچنین `list_elem` که مربوط به ذخیره کردن ترد در `slept_threads` بود را حذف کردیم.
`(struct list_elem slept_elem)`
چون نکته مهم این است که هر ترد در هر لحظه حداکثر در یکی از  
`ready_list` یا `slept_list` یا
 لیست `waiters`
یک سمافور است.


تابع `thread_compareby_ticks` برای مقایسه تیک بیدار شدن دو ترد در هنگام استفاده از دستور
`list_insert_ordered` در هنگام افزودن ترد به `slept_threads`
زده شده است.

تابع `thread_sleep` زمان بیدار شدن برای ترد کنونی را در ساختار ترد کنونی ذخیره میکند.
سپس ترد را در لیست تردهای خواب بر اساس زمان بیدار شدنش وارد میکند و آن را بلاک میکند.

تابع `check_slept_threads` نیز با هر تیک و صدا زده شدن timer interrupt handler
بررسی میکند که تردهای خواب باید بیدار شوند یا نه و اگر باید بیدار شوند آنها را از 
`slept_threads`
حذف میکنیم و آنها را آنبلاک میکینم.


## مسئولیت هر فرد
کد بخش های اول و دوم را به صورت گروهی به صورت حضوری و آنلاین آقایان سلمانی و دهقانپور و حسینی زده اند.
آقای بابایی نیز در طراحی این دو بخش دخیل بودند.
 بخش مربوط به آزمایشگاه را نیز آقای بابایی زده اند. 

 
