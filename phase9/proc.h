// proc.h, 159

#ifndef _PROC_H_
#define _PROC_H_

void IdleProc(void);      // PID 0, never preempted
void ChildStuff(int);
void UserProc(void);      // PID 1, 2, 3, ...

void Wrapper(func_p_t);
void Ouch(void);
void ChildHandler(void);

#endif
