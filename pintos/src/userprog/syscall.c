#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

#define MAX_SYSCALL_ARGUMENTS     10


#define CHECK_ARGS(args, count, is_address...) \
if (!check_arguments(args, count, is_address)) EXIT_WITH_ERROR

#define EXIT_WITH_ERROR \
{ \
  printf ("%s: exit(%d)\n", &thread_current ()->name, -1); \
  thread_exit (); \
  return; \
}

static void syscall_handler (struct intr_frame *);

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
check_arguments(uint32_t* array, uint32_t arg_count, bool is_address, ... /* address argument
 * indices */)
{
  if (arg_count > MAX_SYSCALL_ARGUMENTS)
    return false;

  if (!is_user_mapped_memory(array) || !is_user_mapped_memory((void *)(array + arg_count + 1) - 1))
    return false;

  if (is_address && !is_user_mapped_memory(array[1]))
    return false;

  va_list args;
  va_start(args, arg_count);
  for (size_t i = 2; i <= arg_count; i ++) {
    if (va_arg(args, bool) && !is_user_mapped_memory(array[i]))
      return false;
  }
  va_end(args);

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

  switch (args[0])
    {
    case SYS_EXIT:                   //  Terminate this process.
      CHECK_ARGS(args, 1, false);
      f->eax = args[1];
      printf ("%s: exit(%d)\n", &thread_current ()->name, args[1]);
      thread_exit ();
      break;
    case SYS_HALT:                   //  Halt the operating system.
    case SYS_EXEC:                   //  Start another process.
    case SYS_WAIT:                   //  Wait for a child process to die.
      break;
    case SYS_CREATE:                 //  Create a file.
      printf("create args: %d %s %d\n", args[0], (const char *) args[1], args[2]);
      break;
    case SYS_REMOVE:                 //  Delete a file.
    case SYS_OPEN:                   //  Open a file. 
    case SYS_FILESIZE:               //  Obtain a file's size. 
    case SYS_READ:                   //  Read from a file. 
    case SYS_WRITE:                  //  Write to a file.
      printf(args[2]);
      break;
    case SYS_SEEK:                   //  Change position in a file. 
    case SYS_TELL:                   //  Report current position in a file. 
    case SYS_CLOSE:                  //  Close a file. 
    case SYS_PRACTICE:               //  Returns arg incremented by 1
      check_arguments(args, 1, false);
      f->eax = args[1] + 1;
      break;
    default:
      break;
    }
}
