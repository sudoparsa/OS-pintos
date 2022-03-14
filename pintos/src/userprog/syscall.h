#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#define CHECK_ARGS(args, count, ...)                        \
if (!check_arguments((args), (count), __VA_ARGS__)) EXIT_WITH_ERROR

#define EXIT_WITH_ERROR                                     \
{                                                           \
  printf ("%s: exit(%d)\n", thread_current ()->name, -1);   \
  thread_exit ();                                           \
  return;                                                   \
}

void syscall_init (void);

#endif /* userprog/syscall.h */
