#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static bool is_user_mapped_memory(const void *address)
{
  if (is_user_vaddr(address))
    return (pagedir_get_page(thread_current ()->pagedir, address) != NULL);
  return 0;
}

static void
syscall_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  const int max_args = (uint32_t*) PHYS_BASE - args;

  if (max_args < 1)
    {
      f->eax = -1;
      thread_exit ();
      return;
    }

  /*
   * The following print statement, if uncommented, will print out the syscall
   * number whenever a process enters a system call. You might find it useful
   * when debugging. It will cause tests to fail, however, so you should not
   * include it in your final submission.
   */

   /*printf("System call number: %d\n", args[0]);*/

  switch (args[0])
    {
    case SYS_EXIT:                   //  Terminate this process.
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
    default:
      break;
    }
}
