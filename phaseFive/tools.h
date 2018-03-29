// tools.h, 159

#ifndef _TOOLS_H_
#define _TOOLS_H_

#include "kernel_types.h" // need definition of 'pid_q_t' below

void EnQ(int, pid_q_t *);
int DeQ(pid_q_t *);
void MyBzero(char *, int);

void MyStrcpy(char *, char *);

int MyStrcmp(char *s1, char*s2);
void MyMemcpy(char *dst, char *src, int bytes);

char* MyStrMove(char*);
char* MyStrAppend(char*, char);

#endif

