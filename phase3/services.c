//services.c, 159

#include "spede.h"
#include "kernel_types.h"
#include "kernel_data.h" 
#include "services.h"
#include "tools.h"       
#include "proc.h"

int tick_count = 0;


// to create process, alloc PID, PCB, and process stack
// build trapframe, initialize PCB, record PID to ready_pid_q (unless 0)
void NewProcService(func_p_t proc_p) {  // arg: where process code starts
   	int pid;
   	if (avail_pid_q.size == 0)  { // may occur as too many proc got created
		cons_printf("Kernel Panic: no more process!\n");
      		return;                   // alternative: breakpoint()
   	}
   	pid = DeQ(&avail_pid_q);
   	MyBzero((char *)&pcb[pid], sizeof(pcb_t));
   	MyBzero((char *)&proc_stack[pid][0], PROC_STACK_SIZE );
   	pcb[pid].state = READY;
   	if (pid != 0)
      	EnQ(pid, &ready_pid_q); //queue pid to be ready_pid_q unless it's 0 (IdleProc)
   	pcb[pid].trapframe_p = (trapframe_t *)&proc_stack[pid][PROC_STACK_SIZE - sizeof(trapframe_t)];
   	pcb[pid].trapframe_p->efl = EF_DEFAULT_VALUE | EF_INTR;//";//fill out efl with "EF_DEFAULT_VALUE | EF_INTR" // to enable intr!
   	pcb[pid].trapframe_p->eip = (int) proc_p; //fill out eip to proc_p
   	pcb[pid].trapframe_p->cs = get_cs(); //fill out cs with the return of get_cs() call
}

// count runtime of process and preempt it when reaching time limit
void TimerService(void) { 
   	
	//new code is here
	int i;
	current_time++;	//upcount OS current time (current_time)
	
	 for(i=0; i<PROC_NUM; i++) {		//loop thru the PCB array looking for (may be multiple):
		if ((pcb[i].wake_time <= current_time) && (pcb[i].state == SLEEP)) { //SLEEP state processes that has its wake time arrives -> time arrives -> then:
		pcb[i].state=READY;
		EnQ(i, &ready_pid_q);	//enqueue its PID to the ready PID queue and update its state
		}
	}


	//original code starts below here for TimerService
  	 outportb(0x20, 0x60);
  	 tick_count++;

        if (tick_count == 75) { // every 0.75 seconds

                tick_count = 0;
        }

   	if (run_pid ==  0)  return; // IdleProc();

   	pcb[run_pid].runtime++;
	if (pcb[run_pid].runtime == TIME_LIMIT){
      	pcb[run_pid].state = READY;
      	EnQ(run_pid, &ready_pid_q );
      	run_pid = -1;
   	}
}

//the new stuff for phaseTwo is down below

void SyscallService(trapframe_t *p) {
	switch(p->eax) {	//switch on p->eax to call one of the 3 services below
		case SYS_GETPID:	//20
			GetpidService(&(p->ebx)); break;
		case SYS_SLEEP:		//162
			SleepService((int)p->ebx); break;
		case SYS_WRITE:		//4
			WriteService((int)p->ebx,(char *)(p->ecx), (int)p->edx); break;		
	}	
}

void GetpidService(int *p) {
	*p=run_pid;		//fill out what p point to with the currently-running PID
}

void SleepService(int centi_sec) {
	pcb[run_pid].wake_time =current_time + centi_sec;//set wake time of running process by current OS time + centi+sec
	pcb[run_pid].state=SLEEP;			//alter process state
	run_pid=-1;					//reset the running PID
}

void WriteService(int fileno, char *str, int len) {
	int i;
	static unsigned short *vga_p=(unsigned short *)0xb8000;	//top-left of screen
	if(fileno==STDOUT) {
		for(i=0; i<len; i++) { 			//for each char in str {
			*vga_p=str[i] + 0x0f00;	// *vga_p=char +color mask

			vga_p++;	//incr vga_p
			if(vga_p >= (unsigned short *)0xb8000 + 25*80) { //bottom-right
				vga_p = (unsigned short *) 0xb8000;
				MyBzero((char *)vga_p, 25*80*2);
			}

		}
	}
}

void SemWaitService(int sem_num) {
	if(sem_num==STDOUT) {
		if (video_sem.val > 0){	//if the value of the video semaphore is greater than zero	
			video_sem--;	//downcount the semaphore value by one
		} else {
			//block the running process
			// 1. enqueue it to the wait queue in the semaphore
			// 2. change its state
			// 3. no running process anymore (lack one)
		}
	} else {
		cons_printf("Kernel Panic: non-such semaphore number!");	
	}
	
}

void SempostService(int sem_num) {
	if(sem_num==STDOUT) {
		if(video_sem.wait_q==0) {	// if the wait queue of the video semaphore is empty
			video_sem.value++;	//upcount the semaphore value by one	
		} else {
		// liberate a waiting process
		// 1. dequeue it from the wait queue in the semaphore
		// 2. change its state
		// 3. enqueue the linerated PID to the ready PID queue
		}
	} else {
		cons_printf("Kernel Panic: non-such semaphore number!");		
	}

}
