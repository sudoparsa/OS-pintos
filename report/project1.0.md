تمرین گروهی ۱/۰ - آشنایی با pintos
======================

شماره گروه: 6
-----
> نام و آدرس پست الکترونیکی اعضای گروه را در این قسمت بنویسید.

آرمان بابایی <292arma@gmail.com>

مهدی سلمانی صالح آبادی <m10.salmani@gmail.com> 

پارسا حسینی <sp.hoseiny@gmail.com> 

علیرضا ددهقانپور <alirezafarashah@gmail.com> 

مقدمات
----------
> اگر نکات اضافه‌ای در مورد تمرین یا برای دستیاران آموزشی دارید در این قسمت بنویسید.


> لطفا در این قسمت تمامی منابعی (غیر از مستندات Pintos، اسلاید‌ها و دیگر منابع  درس) را که برای تمرین از آن‌ها استفاده کرده‌اید در این قسمت بنویسید.

آشنایی با pintos
============
>  در مستند تمرین گروهی ۱۹ سوال مطرح شده است. پاسخ آن ها را در زیر بنویسید.


## یافتن دستور معیوب

۱.
0xc0000008

۲.
eip=0x8048757

۳.
تابع `_start`
```
08048754 <_start>:
   8048754:       83 ec 1c                sub    $0x1c,%esp
-> 8048757:       8b 44 24 24             mov    0x24(%esp),%eax
   804875b:       89 44 24 04             mov    %eax,0x4(%esp)
   804875f:       8b 44 24 20             mov    0x20(%esp),%eax
   8048763:       89 04 24                mov    %eax,(%esp)
   8048766:       e8 35 f9 ff ff          call   80480a0 <main>
   804876b:       89 04 24                mov    %eax,(%esp)
   804876e:       e8 49 1b 00 00          call   804a2bc <exit>

```




۴.
```
void
_start (int argc, char *argv[])
{
exit (main (argc, argv));
}
```



در ابتدا با کم کردن esp به اندازه‌ی 0x1c فضای کافی برای قرار گرفتن ورودی‌ها و آدرس برگشت و همچنین فضای خالی برای انطباق با cache قرار داده می‌شود. (با اضافه شدن چهار بابت آدرس برگشت، مقدار کم شدن برابر 0x20 می‌شود.)
در ادامه با توجه به این که انتقال از حافظه به حافظه ممکن نیست، به کمک واسطه‌ی eax مقدار 0x8 در ابتدای صدازدن تابع یعنی همان argv به 0x4 کنونی یعنی محل قرار دادن این آرگومان برای صدا زدن تابع main قرار می‌گیرد.
در ادامه همین کار برای 0x4 قبلی که همان argc است انجام می‌شود و در محل esp قرار می‌گیرد.
سپس به کمک دستور call تابع main صدا زده می‌شود و خروجی در eax قرار می‌گیرد.
در نهایت خروجی main در esp به عنوان آرگومان تابع exit قرار می‌گیرد و این تابع با دستور call صدا زده می‌شود.

۵.
با توجه به مستندات pintos، محل شروع stack آدرس 0xc0000000 است و آدرس‌های کوچک‌تر از آن در استک هستند. این در حالی‌ست که اگر به esp در لحظه‌ی اجرای دستور نگاه کنیم به مقدار 
`esp=bfffffe4`
می‌رسیم که متوجه می‌شویم در هنگام صدا زده شدن تابع، esp در0xc0000000 بوده. یعنی بر خلاف آن‌چه به تابع وعده داده شده بود، ورودی‌ها در استک قرار نگرفته‌اند و نشانگر استک حرکت نکرده است.

## به سوی crash

۶.
ریسه‌ی main با آدرس0xc000e000 این دستور را اجرا می‌کند. ریسه‌ی دیگری که در وجود دارد idle است که اطلاعات هردوی آن‌ها در زیر آمده است.
```
pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_RUNNING, name = "main", '\000' <repeats 11 times>, stack = 0xc000edec <incomplete sequence \357>, priority = 31, allelem = {prev = 0xc0035910 <all_list>, next = 0xc0104
020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000e020, next = 0xc0035918 <all_list+8>}, elem = {prev = 0x
c0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
```



۷.
```
#0  process_execute (file_name=file_name@entry=0xc0007d50 "do-nothing") at ../../userprog/process.c:32
#1  0xc0020268 in run_task (argv=0xc00357cc <argv+12>) at ../../threads/init.c:288
#2  0xc0020921 in run_actions (argv=0xc00357cc <argv+12>) at ../../threads/init.c:340
#3  main () at ../../threads/init.c:133p
```




```
#1 : 288+>  process_wait (process_execute (task));
#2 : 340+>      a->function (argv);
#3 : 133+>  run_actions (argv);
```



۸.
به ریسه‌های پیشین، ریسه‌ی do-nothing اضافه شده است.
```
pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_BLOCKED, name = "main", '\000' <repeats 11 times>, stack = 0xc000eeac "\001", priority = 31, allelem = {prev = 0xc0035910 <all_list>, next = 0xc0104020}, elem = {prev =
 0xc0037314 <temporary+4>, next = 0xc003731c <temporary+12>}, pagedir = 0x0, magic = 3446325067}
pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000e020, next = 0xc010a020}, elem = {prev = 0xc0035920 <rea
dy_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
pintos-debug: dumplist #2: 0xc010a000 {tid = 3, status = THREAD_RUNNING, name = "do-nothing\000\000\000\000\000", stack = 0xc010afd4 "", priority = 31, allelem = {prev = 0xc0104020, next = 0xc0035918 <all_list+8>}, elem = {prev = 0x
c0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
```



۹.
با قرار دادن breakpoint روی thread_create متوجه می‌شویم تابعی که این ریسه را ایجاد کرده است، process_execute در process.c است.

```
 44|   /* Create a new thread to execute FILE_NAME. */
 45+>  tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);
 46|   if (tid == TID_ERROR)     y);
 48|   return tid;ee_page (fn_cop
 49| }
```




۱۰.
قبل از اجرای تابع load
```
{edi = 0x0, esi = 0x0, ebp = 0x0, esp_dummy = 0x0, ebx = 0x0, edx = 0x0, ecx = 0x0, eax = 0x0, gs = 0x23, fs = 0x23, es = 0x23, ds = 0x23, vec_no = 0x0, error_code = 0x0, frame_pointer = 0x0, eip = 0x0, cs = 0x1b, eflags = 0x20
2, esp = 0x0, ss = 0x23}
```



پس از اجرای تابع load
```
{edi = 0x0, esi = 0x0, ebp = 0x0, esp_dummy = 0x0, ebx = 0x0, edx = 0x0, ecx = 0x0, eax = 0x0, gs = 0x23, fs = 0x23, es = 0x23, ds = 0x23, vec_no = 0x0, error_code = 0x0, frame_pointer = 0x0, eip = 0x8048754, cs = 0x1b, eflags
= 0x202, esp = 0xc0000000, ss = 0x23}
```



۱۱. 

در این تابع در ابتدا مقادیر رجیسترها برای اجرای برنامه کاربر محاسبه و سپس در `intr_frame`  ذخیره می‌شود. سپس این رجیسترها در استک ذخیره می‌شوند تا در `intr_exit`  که در اصل کد برای Interrupt کردن پراسسهاست از آن استفاده شود. (برای switch از حالت kernel به حالت user پس از Interrupt، کرنل به این قسمت پرش می‌کند.) سپس در بخش اسمبلی مقادیر ابتدایی رجیسترها از استک در رجیستر متناظرشان پوش می‌شوند تا در نهایت در دستور iret مقدار eip به آدرس شروع برنامه کاربر تغییر پیدا کند. (iret چند رجیستر دیگر را نیز مقداردهی می‌کند.) رجیستر eip نیز همان program counter در پردازنده x86 است پس دستور بعدی که اجرا می‌شود خط اول برنامه کاربر در user mode است.


۱۲. 

در زیر خروجی دستور `info register` را مشاهده می‌کنید. همانطور که در زیر می‌بینید با `_if` تفاوتی ندارند.
```
eax            0x0      0
ecx            0x0      0
edx            0x0      0
ebx            0x0      0
esp            0xc0000000       0xc0000000
ebp            0x0      0x0
esi            0x0      0
edi            0x0      0
eip            0x8048754        0x8048754
eflags         0x202    [ IF ]
cs             0x1b     27
ss             0x23     35
ds             0x23     35
es             0x23     35
fs             0x23     35
gs             0x23     35
```



۱۳.

```
#0  _start (argc=<unavailable>, argv=<unavailable>) at ../../lib/user/entry.c:9
```




## دیباگ

۱۴.
برای پاس کردن تست مذکور دستور `if_.esp -= 36` را در تابع  `start_process` پس از دستور `load` که در آن مقدار esp تعیین می‌شود قرار می‌دهیم. چرا؟ در بخش استک مربوط به userspace مطابق صفحه 19 مرجع داده شده برابر خواهد بود با:
```
argv[0][...]
stack-align -> PHYS_BASE - 20
argv[0] -> PHYS_BASE - 24
argv -> PHYS_BASE - 28
argc -> PHYS_BASE - 32
dummy return address -> PHYS_BASE - 36
```  

در واقع مطابق با مرجع باید استک به صورت 16 بایتی align باشد. (پایان آدرس هر بلاک باید با c تمام شود.)

۱۵.
خروجی : 12
همانطور که در قسمت قبل گفتیم باید esp به صورت 16 بایتی align باشد. و باید پایان هر آدرس esp به c ختم می‌شود. پس باقی‌مانده esp به 16 همواره باید 12 باشد.

۱۶.

۱۷.

۱۸.

۱۹.

با استفاده از از دستور `info thread` به این نتیجه می‌رسیم که ریسه `main` با آدرس `0xc000e000` این تابع را اجرا کرده است.

در زیر با دستور `dumplist &all_list thread allelem`  تمام ریسه‌ها (استراکتشان) چاپ شده است.
```
pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_RUNNING, name = "main", '\000' <repeats 11 times>, stack = 0xc000eeac "\001", priority = 31, allelem = {prev = 0xc0035910 <all_list>, next = 0xc0104020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000e020, next = 0xc0035918 <all_list+8>}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
```