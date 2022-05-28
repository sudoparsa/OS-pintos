#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/directory.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"

#define MAX_ARGUMENTS        32
#define MAX_ARGUMENT_LENGTH  1024
#define ARGUMENT_DELIMITER   " "

static char* argv[MAX_ARGUMENTS];
static int argc;
static bool tokenize(char* cmd_line);

static struct semaphore temporary;
static thread_func start_process NO_RETURN;
static bool load (char *cmdline, void (**eip) (void), void **esp);

static void free_finished_threads (struct list *threads);
static void free_file_descriptors (struct thread *);

/* Tokenizes the cmd_line provided as the input, and sets argc and
 * argv variables accordingly, returns true on sucess, and false
 * otherwise. */
bool
tokenize (char* cmd_line)
{
  int i;
  char *c;

  for (i = 0; i < MAX_ARGUMENTS; ++ i)
    argv[i] = NULL;  /* empty argv */

  argc = 0;
  char* strtok_saveptr;
  c = strtok_r (cmd_line, ARGUMENT_DELIMITER, &strtok_saveptr);  /* Start tokenizer on the input */
  for (i = 0; c && (i < MAX_ARGUMENTS); ++ i)
    {
      argv[i] = c;
      argc ++;
      c = strtok_r (NULL, ARGUMENT_DELIMITER, &strtok_saveptr);  /* scan for next token */
    }

  /* Return false if we ran out of space on argv. */
  if (strtok_r (NULL, ARGUMENT_DELIMITER, &strtok_saveptr) != NULL)
    return false;

  return true;
}


/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */
tid_t
process_execute (const char *file_name)
{
  char *fn_copy;
  tid_t tid;
  struct fn_cps *fn_cps_ = malloc (sizeof (struct fn_cps));

  struct child_parent_status *cps = malloc (sizeof (struct child_parent_status));
  cps->ref_count = 2;
  sema_init (&cps->sema, 0);
  lock_init (&cps->lock);

  fn_cps_->cps = cps;
  struct thread *cur = thread_current ();
  list_push_back (&(cur->children), &(cps->elem)); 
  fn_cps_->cwd = cur->cwd;
  sema_init (&temporary, 0);
  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE);
  
  fn_cps_->fn = fn_copy;

  /* Create a new thread to execute FILE_NAME. */
  tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_cps_);

  if (tid == TID_ERROR)
    palloc_free_page (fn_copy);
  else
    sema_down (&cps->sema);
  if (!fn_cps_->success)
    {
      free (fn_cps_);
      return TID_ERROR;
    }
  free (fn_cps_);
  return tid;
}

/* A thread function that loads a user process and starts it
   running. */
static void
start_process (void *fn)
{
  struct fn_cps *fn_cps_ = (struct fn_cps *) fn;
  struct child_parent_status *cps = fn_cps_->cps;
  // Set associated thread with the cps
  struct thread *cur = thread_current ();
  cur->cps = cps;
  cps->pid = cur->tid;

  char *file_name = fn_cps_->fn;
  struct intr_frame if_;
  bool success;

  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  success = load (file_name, &if_.eip, &if_.esp);
  
  if (fn_cps_->cwd)
    cur->cwd = dir_reopen (fn_cps_->cwd);
  else
    cur->cwd = dir_open_root ();

  fn_cps_->success = success;

  palloc_free_page (file_name);

  /* If load failed, quit. */
  if (!success)
    {
      cps->exit_code = -1;
      sema_up (&cps->sema);
      thread_exit ();
    }

  

  sema_up (&cps->sema);

  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int
process_wait (tid_t child_tid UNUSED)
{
  struct thread *cur = thread_current ();
  struct child_parent_status *child = NULL;
  struct list_elem *e;

  for (e = list_begin (&(cur->children)); e != list_end (&(cur->children)); e = list_next (e))
    {
      child = list_entry (e, struct child_parent_status, elem);

      if (child->pid == (pid_t) child_tid)
          break;
      
      child = NULL;
    }

  if (child == NULL)
    return -1;
  
  sema_down (&(child->sema));
  list_remove (&child->elem);
  return child->exit_code;
}

static void
free_file_descriptors (struct thread *cur)
{
  int i;
  for (i = 0; i < MAX_FILE_DESCRIPTORS; i++)
    {
      if (cur->file_descriptors[i] != NULL)
      {
        if (dir_from_file (cur->file_descriptors[i]) == NULL)
          file_close (cur->file_descriptors[i]);
        else
          dir_close (dir_from_file (cur->file_descriptors[i]));
      }
    }
}

/* Free the current process's resources. */
void
process_exit (void)
{
  struct thread *cur = thread_current ();

  lock_acquire (&(cur->cps->lock));
  (cur->cps->ref_count)--;
  lock_release (&(cur->cps->lock));
  sema_up (&(cur->cps->sema));

  if (cur->cps->ref_count == 0)
    free (cur->cps);
  
  free_finished_threads (&cur->children);

  free_file_descriptors (cur);

  if (cur->cwd)
    dir_close(cur->cwd);
  

  uint32_t *pd;
  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  pd = cur->pagedir;
  if (pd != NULL)
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }
  file_close (cur->process_file);
}

static void
free_finished_threads (struct list *threads)
{
  struct child_parent_status *finished_threads[list_size (threads)];
  int size = 0;
  struct list_elem *e;
  struct child_parent_status *child;

  for (e = list_begin (threads); e != list_end (threads); e = list_next (e))
    {
      child = list_entry (e, struct child_parent_status, elem);
      lock_acquire (&(child->lock));
      (child->ref_count)--;
      lock_release (&(child->lock));
      if (child->ref_count == 0)
          finished_threads[size++] = child;
    }
  int i;
  for (i = 0; i < size; i++)
    free (finished_threads[i]);
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

static bool setup_stack (void **esp);
static bool validate_segment (const struct Elf32_Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool
load (char *file_name, void (**eip) (void), void **esp)
{
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofs;
  bool success = false;
  int i;

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL)
    goto done;
  process_activate ();

  /* Fill argc and argv. */
  if (!tokenize (file_name))
    {
      printf ("load: %s: cannot tokenize\n", file_name);
      goto done;
    }
  if (argc < 1)
    {
      printf ("load: cannot run program with no args.\n");
      goto done;
    }

  memcpy (thread_current ()->name, argv[0], 15);
  thread_current ()->name[15] = '\0';

  /* Open executable file. */
  file = filesys_open (argv[0]);
  t->process_file = file;
  if (file == NULL)
    {
      printf ("load: %s: open failed\n", file_name);
      goto done;
    }
  file_deny_write (file);

  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024)
    {
      printf ("load: %s: error loading executable\n", file_name);
      goto done;
    }

  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++)
    {
      struct Elf32_Phdr phdr;

      if (file_ofs < 0 || file_ofs > file_length (file))
        goto done;
      file_seek (file, file_ofs);

      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
        goto done;
      file_ofs += sizeof phdr;
      switch (phdr.p_type)
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, file))
            {
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                                - read_bytes);
                }
              else
                {
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
              if (!load_segment (file, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable))
                goto done;
            }
          else
            goto done;
          break;
        }
    }

  /* Set up stack. */
  if (!setup_stack (esp))
    goto done;

  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;

  success = true;

 done:
  /* We arrive here whether the load is successful or not. */
  // file_close (file);
  return success;
}

/* load() helpers. */

static bool install_page (void *upage, void *kpage, bool writable);

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, struct file *file)
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK))
    return false;

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length (file))
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz)
    return false;

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;

  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  file_seek (file, ofs);
  while (read_bytes > 0 || zero_bytes > 0)
    {
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      /* Get a page of memory. */
      uint8_t *kpage = palloc_get_page (PAL_USER);
      if (kpage == NULL)
        return false;

      /* Load this page. */
      if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
        {
          palloc_free_page (kpage);
          return false;
        }
      memset (kpage + page_read_bytes, 0, page_zero_bytes);

      /* Add the page to the process's address space. */
      if (!install_page (upage, kpage, writable))
        {
          palloc_free_page (kpage);
          return false;
        }

      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
    }
  return true;
}

/* Adds arguments and corresponding argc and argv to stack and
 * decreases esp. */
static bool
fill_args_in_stack (void **esp)
{
  /* Put arguments on stack and update argv accordingly. */
  for (int i = 0; i < argc; i ++)
    {

      const size_t len = strnlen(argv[i], MAX_ARGUMENT_LENGTH) + 1;
      *esp -= len;
      if (len > MAX_ARGUMENT_LENGTH)
        return false;

      memcpy (*esp, argv[i], len);
      argv[i] = *esp;
    }

  /* Align stack. */
  const int align_size = ((size_t)(*esp) - ((argc + 3) * 4 )) % 16;
  *esp -= align_size;
  memset (*esp, 0x11, align_size);

  /* Put argv[argc]. */
  *esp -= 4;
  memset (*esp, 0, 4);

  /* Put argv. */
  *esp -= argc * 4;
  memcpy (*esp, argv, argc * 4);
  *esp -= 4;
  *((size_t*)*esp) = (size_t)(*esp + 4);

  /* Put argc. */
  *esp -= 4;
  *((size_t*)*esp) = argc;

  /* Put return address. */
  *esp -= 4;
  return true;
}

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */
static bool
setup_stack (void **esp)
{
  uint8_t *kpage;
  bool success = false;

  kpage = palloc_get_page (PAL_USER | PAL_ZERO);
  if (kpage != NULL)
    {
      success = install_page (((uint8_t *) PHYS_BASE) - PGSIZE, kpage, true);
      if (success)
        {
          *esp = PHYS_BASE;
          success &= fill_args_in_stack (esp);
        }
      else
        palloc_free_page (kpage);
    }
  return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}
