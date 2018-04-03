// tools.c, 159

#include "spede.h"
#include "kernel_types.h"
#include "kernel_data.h"


int MyStrcmp(char *s1, char *s2) {
	 while (*s1 == *s2) {
	    if (*s1 == '\0') break;
	    s1++;
	    s2++;
	}
	return ((*s1 == *s2) && *s1 == '\0');
}

void MyMemcpy(char *dst, char *src, int bytes) {
	int i;
	for (i = 0; i < bytes; i++){
		dst[i] = src[i];
	}
}

void MyStrAppend(char* str, char ch) {
    while(*str){
        str++;
    }
    *str = ch;
    str++;
    *str='\0';
}

void MyStrMove(char *str) {
    char* temp = str;
    while (*str) {
	    temp++;
	    *str = *temp;
	    str++;
    }
}

// clear DRAM by setting each byte to zero
void MyBzero(char *p, int size) {
   int i;
   for(i=1; i <= size ; i++){
//      char p =  (char *)0;  //check this if something goes wrong
        *p++ = 0;
   }
}

// dequeue, return 1st element in array, and move all forward
// if queue empty, return -1
int DeQ(pid_q_t *p) {
   int i,  element = -1;
   if (p->size == 0){
	return -1;
   }

   //if the size of the queue p points to is zero, return element (-1)
   //(otherwise, continue)
   element = p->q[0];
   p->size--;
   for(i=0; i < p->size ; i++){
	p->q[i] = p->q[i+1];
   }	  
 //  copy the 1st in the array that p points to to element
  // decrement the size of the queue p points to by 1
 //  move all elements in the array to the front by one position

   return element;
}

// enqueue element to next available position in array, 'size' is array index
void EnQ(int element, pid_q_t *p) {
   if(p->size == Q_SIZE){
	cons_printf("Kernel Panic: queue is full, cannot EnQ!\n");
	return;		//alternative: breakpoint() into GDB
   }
   p->q[p->size] = element;	//copy element into the array indexed by 'size'
   p->size++;	 //increment 'size' of the queue p points to by 1
}

void MyStrcpy(char *dst, char *src){ //used in WriteServices()
	while(*src) {  
		*dst++ = *src++;
	}
	*dst='\0'; 
	
}
