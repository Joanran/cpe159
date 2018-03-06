// services.h, 159

#ifndef _SERVICES_H_
#define _SERVICES_H_

#include "kernel_types.h"   // need definition of 'func_p_t' below

void NewProcService(func_p_t);
void TimerService(void);

void SyscallService(trapframe_t *);
void SleepService(int);
void GetpidService(int *);
void WriteService(int,char *,int);

//new ones are below
void SemwaitService(int);
void SempostService(int);

#endif
