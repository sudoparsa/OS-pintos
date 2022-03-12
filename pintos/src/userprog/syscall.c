#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "devices/input.h"


#define MAX_SYSCALL_ARGUMENTS     10


#define CHECK_ARGS(args, count, ...) \
if (!check_arguments((args), (count), __VA_ARGS__)) EXIT_WITH_ERROR

#define EXIT_WITH_ERROR \
{ \
  printf ("%s: exit(%d)\n", thread_current ()->name, -1); \
  thread_exit (); \
  return; \
}

static void syscall_handler (struct intr_frame *);

static void write_syscall (struct intr_frame *, uint32_t*, struct thread*);
static void read_syscall (struct intr_frame *, uint32_t*, struct thread*);
static void filesize_syscall (struct intr_frame *, uint32_t*, struct thread*);
static void seek_syscall (struct intr_frame *, uint32_t*, struct thread*);
static void tell_syscall (struct intr_frame *, uint32_t*, struct thread*);




void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static bool
is_user_mapped_memory(const void *address)
{
  if (is_user_vaddr(address))
    return (pagedir_get_page(thread_current ()->pagedir, address) != NULL);
  return false;
}

/* Checks if arguments for a system call are valid. It
 * first checks that ARRAY has ARG_COUNT members in user memory
 * (it doesn't check for only stack) and then for every argument,
 * it checks that if it's an address, it is in user space.
 *
 * The function call check_arguments(arr, 3, true, false, true)
 * checks for a system call which has 3 arguments which first and
 * last one of them is an address.
 */
static bool
check_arguments(uint32_t* array, uint32_t arg_count, uint32_t is_address, ...)
{
  if (arg_count > MAX_SYSCALL_ARGUMENTS)
    return false;

  if (!is_user_mapped_memory(array) || !is_user_mapped_memory((void *)(array + arg_count + 1) - 1))
    return false;

  if (is_address && !is_user_mapped_memory((void *) array[1]))
    return false;

  va_list args;
  va_start (args, is_address);
  for (size_t i = 2; i <= arg_count; i ++) {
    bool should_check_address = va_arg (args, int);
    if (should_check_address && !is_user_mapped_memory((void *)array[i]))
      return false;
  }
  va_end (args);

  return true;
}

/* Checks if file descriptor FD is a valid, non-NULL
   descriptor in thread T. */
static bool
check_fd(struct thread *t, int fd)
{
  if (fd > MAX_FILE_DESCRIPTORS || fd < 0)
    return false;

  if (t->file_descriptors[fd] == NULL)
    return false;

  return true;
}

static void
syscall_handler (struct intr_frame *f)
{
  if (!is_kernel_vaddr(f) || !is_kernel_vaddr((void *)(f + 1) - 1))
    EXIT_WITH_ERROR;

  uint32_t* args = ((uint32_t*) f->esp);

  /*
   * The following print statement, if uncommented, will print out the syscall
   * number whenever a process enters a system call. You might find it useful
   * when debugging. It will cause tests to fail, however, so you should not
   * include it in your final submission.
   */

  /*printf("System call number: %d\n", args[0]);*/

  CHECK_ARGS(args, 0, false);
  struct thread * trd = thread_current ();

  switch (args[0])
    {
    case SYS_EXIT:                   //  Terminate this process.
      CHECK_ARGS(args, 1, false);
      f->eax = args[1];
      printf ("%s: exit(%d)\n", trd->name, args[1]);
      thread_exit ();
      break;
    case SYS_HALT:                   //  Halt the operating system.
    case SYS_EXEC:                   //  Start another process.
    case SYS_WAIT:                   //  Wait for a child process to die.
      break;
    case SYS_CREATE:                 //  Create a file.
      CHECK_ARGS (args, 2, true, false);
      f->eax = filesys_create ((const char *) args[1], args[2]);
      break;
    case SYS_REMOVE:                 //  Delete a file.
      CHECK_ARGS (args, 1, true);
      f->eax = filesys_remove ((const char *) args[1]);
      break;
    case SYS_OPEN:                   //  Open a file.
      {
        CHECK_ARGS (args, 1, true);
        int fd = thread_get_free_file_descriptor (trd);
        f->eax = fd;
        if (fd > 0)
          {
            trd->file_descriptors[fd] = filesys_open ((const char *) args[1]);
            if (trd->file_descriptors[fd] == NULL)
              f->eax = -1;
          }
        break;
      }
    case SYS_FILESIZE:               //  Obtain a file's size. 
      filesize_syscall (f, args, trd);
      break;
    case SYS_READ:                   //  Read from a file.
      read_syscall (f, args, trd);
      break;
    case SYS_WRITE:                  //  Write to a file.
      write_syscall (f, args, trd);
      break;
    case SYS_SEEK:                   //  Change position in a file.
      seek_syscall (f, args, trd);
      break; 
    case SYS_TELL:                   //  Report current position in a file.
      tell_syscall (f, args, trd);
      break;
    case SYS_CLOSE:                  //  Close a file.
      {
        CHECK_ARGS (args, 1, false);
        /* Fail when closing a wrong fd. */
        if (!check_fd(trd, args[1]) || args[1] < 3)
          EXIT_WITH_ERROR;
        file_close(trd->file_descriptors[args[1]]);
        trd->file_descriptors[args[1]] = NULL;
        break;
      }
    case SYS_PRACTICE:               //  Returns arg incremented by 1
      check_arguments(args, 1, false);
      f->eax = args[1] + 1;
      break;
    default:
      break;
    }
}

static size_t
getbuf (char *buffer, size_t length)
{
  size_t i = 0;
  char c;

  for (; i < length; i++)
    {
      c = input_getc ();
      if (c == '\n' || c == '\r')
        {
          buffer[i] = '\0';
          return i + 1;
        }
      buffer[i] = c;
    }
  return i;
}

static void 
read_syscall (struct intr_frame *f, uint32_t* args, struct thread* trd)
{
  CHECK_ARGS (args, 3, false, true, false);

  int fd = (int) args[1];
  char *buffer = (char *) args[2];
  int length = (int) args[3];

  if (!fd)
    {
      f->eax = getbuf (buffer, length);
      return;
    }

  /* Fail when reading a wrong fd or standard output */
  if (!check_fd(trd, fd) || fd == 1)
      EXIT_WITH_ERROR;

  f->eax = file_read (trd->file_descriptors[fd], buffer, length);
}

static void 
write_syscall (struct intr_frame *f, uint32_t* args, struct thread* trd)
{
  CHECK_ARGS (args, 3, false, true, false);

  int fd = (int) args[1];
  char *buffer = (char *) args[2];
  int length = (int) args[3];

  if (fd == 1 || fd == 2)
    {
      putbuf (buffer, length);
      f->eax = length;
      return;
    }

  /* Fail when writing a wrong fd or standard input */
  if (!check_fd(trd, fd) || !fd)
      EXIT_WITH_ERROR;

  f->eax = file_write (trd->file_descriptors[fd], buffer, length);
}

static void 
filesize_syscall (struct intr_frame *f, uint32_t* args, struct thread* trd)
{
  CHECK_ARGS(args, 1, false);

  int fd = (int) args[1];

  /* Fail when want size of a wrong fd or standard input or standard output */
  if (!check_fd(trd, fd) || fd == 0 || fd == 1)
      EXIT_WITH_ERROR;

  f->eax = file_length (trd->file_descriptors[fd]);
}

static void 
tell_syscall (struct intr_frame *f, uint32_t* args, struct thread* trd)
{
  CHECK_ARGS(args, 1, false);

  int fd = args[1];

  /* Fail when tell of a wrong fd or standard input or standard output */
  if (!check_fd(trd, fd) || fd == 0 || fd == 1)
      EXIT_WITH_ERROR;
  
  f->eax = file_tell (trd->file_descriptors[fd]);
}

static void 
seek_syscall (struct intr_frame *f, uint32_t* args, struct thread* trd)
{
  CHECK_ARGS(args, 2, false, false);

  int fd = args[1];
  int position = args[2];

  /* Fail when seek of a wrong fd or standard input or standard output */
  if (!check_fd(trd, fd) || fd == 0 || fd == 1)
      EXIT_WITH_ERROR;
  
  file_seek (trd->file_descriptors[fd], position);
  f->eax = 0;
}
