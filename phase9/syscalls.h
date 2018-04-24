//syscalls.h

#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

#include "kernel_types.h"
#include "kernel_constants.h"

void sys_exec(func_p_t p, int);

void sys_exit(int);
int sys_waitchild(int *);


int sys_fork(void);
void sys_signal(int, func_p_t);

int sys_getppid(void);
int sys_getpid(void); 
void sys_write(int, char *, int);
void sys_read(int, char *, int);
void sys_sleep(int);

void sys_semwait(int);
void sys_sempost(int); 

#endif
