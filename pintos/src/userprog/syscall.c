
#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "devices/input.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "threads/synch.h"

#define MAX_SYSCALL_ARGUMENTS     10
#define STRING -1
#define VALUE 0

static void syscall_handler (struct intr_frame *);

static void write_syscall (struct intr_frame *, uint32_t*, struct thread*);
static void read_syscall (struct intr_frame *, uint32_t*, struct thread*);
static void filesize_syscall (struct intr_frame *, uint32_t*, struct thread*);
static void seek_syscall (struct intr_frame *, uint32_t*, struct thread*);
static void tell_syscall (struct intr_frame *, uint32_t*, struct thread*);
static void exit_syscall (struct intr_frame *, uint32_t*, struct thread*);
static void open_syscall (struct intr_frame *, uint32_t*, struct thread*);
static void close_syscall (struct intr_frame *, uint32_t*, struct thread*);
static void create_syscall (struct intr_frame *, uint32_t*);
static void remove_syscall (struct intr_frame *, uint32_t*);
static void exec_syscall (struct intr_frame *, uint32_t*);
static void wait_syscall (struct intr_frame *, uint32_t*);
static void practice_syscall (struct intr_frame *, uint32_t*);
static void chdir_syscall (struct intr_frame *, uint32_t*, struct thread*);
static void mkdir_syscall (struct intr_frame *, uint32_t*);
static void inumber_syscall (struct intr_frame *, uint32_t*, struct thread*);

void
syscall_init (void)
{
  lock_init(&global_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static bool
is_user_mapped_memory(const void *address)
{
  bool result = false;
  if (is_user_vaddr(address))
    result = (pagedir_get_page(thread_current ()->pagedir, address) != NULL);
  return result;
}

static bool
check_address (uint32_t addr, int address_size)
{
  if (address_size > 0)
    {
      if (!is_user_mapped_memory(((void *)addr)) ||
       !is_user_mapped_memory(((void *)addr) + address_size - 1))
        return false;
    }
  else if (address_size < 0)
    {
      int offset = 0;
      while (is_user_mapped_memory(((void *)addr) + offset) && *((char *)addr + offset) != 0)
        offset++;
      if (!is_user_mapped_memory(((void *)addr) + offset))
        return false;
    }
  return true;
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
check_arguments(uint32_t* array, uint32_t arg_count, int32_t is_address, ...)
{
  if (arg_count > MAX_SYSCALL_ARGUMENTS)
    {
      thread_current ()->cps->exit_code = -1;
      return false;
    }


  if (!is_user_mapped_memory(array) || !is_user_mapped_memory((void *)(array + arg_count + 1) - 1))
    {
      thread_current ()->cps->exit_code = -1;
      return false;
    }
 
  if (arg_count > 0 && !check_address(array[1], is_address))
    {
      thread_current ()->cps->exit_code = -1;
      return false;
    }

  va_list args;
  va_start (args, is_address);
  for (size_t i = 2; i <= arg_count; i ++)
    {
      int address_size = va_arg (args, int);
      if (!check_address(array[i], address_size))
        {
          thread_current ()->cps->exit_code = -1;
          return false;
        }
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

/* Retrieves inode from file descriptor in thread. The resulting inode
   will be put in `inode`. Returns true on success, and false otherwise. */
static bool
get_inode_from_fd(struct inode **inode, int fd, struct thread *trd)
{
  /* Fail when seek of a wrong fd or standard input or standard output */
  if (!check_fd(trd, fd) || fd == 0 || fd == 1)
    return false;

  struct file* descriptor = trd->file_descriptors[fd];
  *inode = file_get_inode (descriptor);
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

  CHECK_ARGS(args, 0, VALUE);
  struct thread * trd = thread_current ();

  switch (args[0])
    {
      case SYS_EXIT:                   //  Terminate this process.
        exit_syscall (f, args, trd);
        break;
      case SYS_HALT:                   //  Halt the operating system.
        shutdown_power_off ();
        break;
      case SYS_EXEC:                   //  Start another process.
        exec_syscall (f, args);
        break;
      case SYS_WAIT:                   //  Wait for a child process to die.
        wait_syscall (f, args); 
        break;
      case SYS_CREATE:                 //  Create a file.
        create_syscall (f, args);
        break;
      case SYS_REMOVE:                 //  Delete a file.
        remove_syscall (f, args);
        break;
      case SYS_OPEN:                   //  Open a file.
        open_syscall (f, args, trd);
        break;
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
        close_syscall (f, args, trd);
        break;
      case SYS_PRACTICE:               //  Returns arg incremented by 1
        practice_syscall (f, args);
        break;
      case SYS_CHDIR:                  // Change the current directory.
        chdir_syscall (f, args, trd);
        break;
      case SYS_MKDIR:                  // Create a directory.
        mkdir_syscall (f, args);
        break;
      case SYS_READDIR:                // Reads a directory entry.
      case SYS_ISDIR:                  // Tests if a fd represents a directory.
        printf("syscall received\n");
        break;
      case SYS_INUMBER:                // Returns the inode number for a fd.
        inumber_syscall (f, args, trd);
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
exit_syscall (struct intr_frame *f, uint32_t* args, struct thread* trd)
{
  CHECK_ARGS (args, 1, VALUE);

  f->eax = args[1];
  trd->cps->exit_code = args[1];
  printf ("%s: exit(%d)\n", trd->name, args[1]);
  thread_exit ();
}

static void
create_syscall (struct intr_frame *f, uint32_t* args)
{
  CHECK_ARGS (args, 2, STRING, VALUE);

  // lock_acquire(&global_lock);
  f->eax = filesys_create ((const char *) args[1], args[2], false);
  // lock_release(&global_lock);
}

static void
remove_syscall (struct intr_frame *f, uint32_t* args)
{
  CHECK_ARGS (args, 1, STRING);

  // lock_acquire(&global_lock);
  f->eax = filesys_remove ((const char *) args[1]);
  // lock_release(&global_lock);
}

static void
practice_syscall (struct intr_frame *f, uint32_t* args)
{
  CHECK_ARGS (args, 1, VALUE);

  f->eax = args[1] + 1;
}

static void
open_syscall (struct intr_frame *f, uint32_t* args, struct thread* trd)
{
  CHECK_ARGS (args, 1, STRING);

  int fd = thread_get_free_file_descriptor (trd);
  f->eax = fd;
  if (fd > 0)
    {
      // lock_acquire(&global_lock);
      trd->file_descriptors[fd] = filesys_open ((const char *) args[1]);
      // lock_release(&global_lock);
      if (trd->file_descriptors[fd] == NULL)
        f->eax = -1;
    }
}

static void
close_syscall (struct intr_frame *f, uint32_t* args, struct thread* trd)
{
  CHECK_ARGS (args, 1, VALUE);

  /* Fail when closing a wrong fd. */
  if (!check_fd (trd, args[1]) || args[1] < 3)
      EXIT_WITH_ERROR;
  // lock_acquire(&global_lock);
  // if (dir_from_file (trd->file_descriptors[args[1]]) == NULL)
    file_close (trd->file_descriptors[args[1]]);
  // else
  //   dir_close (dir_from_file (trd->file_descriptors[args[1]]));
  // lock_release(&global_lock);
  trd->file_descriptors[args[1]] = NULL;
}

static void
exec_syscall (struct intr_frame *f, uint32_t* args)
{
  CHECK_ARGS (args, 1, STRING);

  f->eax = process_execute ((const char*) args[1]);
}

static void
wait_syscall (struct intr_frame *f, uint32_t* args)
{
  CHECK_ARGS (args, 1, VALUE);

  f->eax = process_wait ((tid_t) args[1]);
}

static void 
read_syscall (struct intr_frame *f, uint32_t* args, struct thread* trd)
{
  CHECK_ARGS (args, 3, VALUE, STRING, VALUE);

  int fd = (int) args[1];
  char *buffer = (char *) args[2];
  int length = (int) args[3];

  if (!fd)
    {
      f->eax = getbuf (buffer, length);
      return;
    }

  /* Fail when reading a wrong fd or standard output */
  if (!check_fd (trd, fd) || fd == 1)
      EXIT_WITH_ERROR;

  // lock_acquire(&global_lock);
  // if (dir_from_file (trd->file_descriptors[fd]) == NULL)
    f->eax = file_read (trd->file_descriptors[fd], buffer, length);
  // else
  // {
  //   // lock_release(&global_lock);
  //   EXIT_WITH_ERROR;
  // }
  // lock_release(&global_lock);
}

static void 
write_syscall (struct intr_frame *f, uint32_t* args, struct thread* trd)
{
  CHECK_ARGS (args, 3, VALUE, STRING, VALUE);

  int fd = (int) args[1];
  const char *buffer = (const char *) args[2];
  int length = (int) args[3];

  if (fd == 1 || fd == 2)
    {
      putbuf (buffer, length);
      f->eax = length;
      return;
    }

  /* Fail when writing a wrong fd or standard input */
  if (!check_fd (trd, fd) || !fd)
      EXIT_WITH_ERROR;

  // lock_acquire(&global_lock);
  if (dir_from_file (trd->file_descriptors[fd]) == NULL)
    f->eax = file_write (trd->file_descriptors[fd], buffer, length);
  else
    f->eax = -1;
  // lock_release(&global_lock);
}

static void 
filesize_syscall (struct intr_frame *f, uint32_t* args, struct thread* trd)
{
  CHECK_ARGS (args, 1, VALUE);

  int fd = (int) args[1];

  /* Fail when want size of a wrong fd or standard input or standard output */
  if (!check_fd(trd, fd) || fd == 0 || fd == 1)
      EXIT_WITH_ERROR;

  // lock_acquire(&global_lock);
  f->eax = file_length (trd->file_descriptors[fd]);
  // lock_release(&global_lock);
}

static void 
tell_syscall (struct intr_frame *f, uint32_t* args, struct thread* trd)
{
  CHECK_ARGS (args, 1, VALUE);

  int fd = args[1];

  /* Fail when tell of a wrong fd or standard input or standard output */
  if (!check_fd (trd, fd) || fd == 0 || fd == 1)
      EXIT_WITH_ERROR;
  
  // lock_acquire(&global_lock);
  f->eax = file_tell (trd->file_descriptors[fd]);
  // lock_release(&global_lock);
}

static void 
seek_syscall (struct intr_frame *f, uint32_t* args, struct thread* trd)
{
  CHECK_ARGS (args, 2, VALUE, VALUE);

  int fd = args[1];
  int position = args[2];

  /* Fail when seek of a wrong fd or standard input or standard output */
  if (!check_fd (trd, fd) || fd == 0 || fd == 1)
      EXIT_WITH_ERROR;
  
  // lock_acquire(&global_lock);
  file_seek (trd->file_descriptors[fd], position);
  // lock_release(&global_lock);
  f->eax = 0;
}

static void
chdir_syscall (struct intr_frame *f, uint32_t* args, struct thread* trd)
{
  CHECK_ARGS (args, 1, STRING);

  struct dir *dir = dir_open_by_path((const char *) args[1]);
  if (dir != NULL)
  {
    trd->cwd = dir;
    f->eax = true;
  }
  else
    f->eax = false;
}

static void
mkdir_syscall (struct intr_frame *f, uint32_t* args)
{
  CHECK_ARGS (args, 1, STRING);

  const char *path = (const char *) args[1];

  f->eax = filesys_create (path, 0 /* unused */, true);
}

static void
inumber_syscall (struct intr_frame *f, uint32_t* args, struct thread* trd)
{
  CHECK_ARGS (args, 1, VALUE);

  struct inode *inode;
  if (!get_inode_from_fd (&inode, args[1], trd))
    EXIT_WITH_ERROR;

  f->eax = inode_get_inumber (inode);
}
