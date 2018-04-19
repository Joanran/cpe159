// services.h, 159

#ifndef _SERVICES_H_
#define _SERVICES_H_

#include "kernel_types.h"   // need definition of 'func_p_t' below

void ExitService(int);
void WaitchildService(int *, int *);

void NewProcService(func_p_t);
void TimerService(void);

void SyscallService(trapframe_t *);
void SleepService(int);
void GetpidService(int *);
void WriteService(int,char *,int);

//phase three below
void SemWaitService(int);
void SempostService(int);

//phase four below
void TermService(int);

//phase five below
void ReadService(int,char *,int);
void DspService(int);
void KbService(int);

//phase six below
void ForkService(int *);

// phase seven below
void SignalService(int, func_p_t);
void WrapperService(int, func_p_t);
void GetPpidService(int *);
#endif
