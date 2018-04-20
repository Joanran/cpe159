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
		case SYS_WAITCHILD:
			WaitchildService((int *)(p->ebx), &(p->ecx));
			break;
		case SYS_EXIT:
			ExitService((int) p->ebx);
			break;
		case SYS_SIGNAL:
			SignalService((int)(p->ebx), (void *)p->ecx); 
			break;
		case SYS_GETPPID:
			GetPpidService(&(p->ebx));
			break;
		case SYS_FORK:
			ForkService(&(p->ebx));
			break;
		case SYS_GETPID:	//20
			GetpidService(&(p->ebx)); 
			break;
		case SYS_SLEEP:		//162
			SleepService((int)p->ebx); 
			break;
		case SYS_READ:
			ReadService((int)p->ebx,(char *)(p->ecx), (int)p->edx); 
			break;
		case SYS_WRITE:		//4
			WriteService((int)p->ebx,(char *)(p->ecx), (int)p->edx); 
			break;	
		case SYS_SEMWAIT:	//300
			SemWaitService((int)p->ebx);
			break;
		case SYS_SEMPOST:	//301
			SempostService((int)p->ebx);
			break;		
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
	int i, which;
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
	
	if (fileno==TERM1) {
		which=0;	
	} else if (fileno==TERM2) {
		which=1;
	}

	if((fileno==TERM1)|| (fileno=TERM2)) {	
		MyStrcpy(term[which].dsp, str);		//1. the 'str' is first copied to the terminal 'dsp' buffer
		EnQ(run_pid, &term[which].dsp_wait_q); //2. and the running process is 'blocked' in the wait queue
		pcb[run_pid].state=WAIT;
		run_pid=-1;
		DspService(which);	//At the end of the WriteService(), change it to calling DspService()
	}
	
}

void SemWaitService(int sem_num) {
	if(sem_num==STDOUT) {
		if (video_sem.val > 0){	//if the value of the video semaphore is greater than zero	
			video_sem.val--;	//downcount the semaphore value by one
		} else {
			//block the running process
			EnQ(run_pid, &video_sem.wait_q);// 1. enqueue it to the wait queue in the semaphore
			pcb[run_pid].state=WAIT;// 2. change its state
			run_pid=-1;		// 3. no running process anymore (lack one)
		}
	} else {
		cons_printf("Kernl Panic: non-such semaphore number!");	
	}
	
}

void SempostService(int sem_num) {
	int pid;
	if(sem_num==STDOUT) {
		if(video_sem.wait_q.size==0) {	// if the wait queue of the video semaphore is empty
			video_sem.val++;	//upcount the semaphore value by one	
		} else {
		// liberate a waiting process
		pid=DeQ(&video_sem.wait_q);	// 1. dequeue it from the wait queue in the semaphore
		pcb[run_pid].state=READY;	// 2. change its state
		EnQ(run_pid, &ready_pid_q);	// 3. enqueue the linerated PID to the ready PID queue
		}
	} else {
		cons_printf("Kernl Panic: non-such semaphore number!");		
	}

}

void DspService(int which) { //does the same work of the TermService of the previous phase
      int i, pid;

      if((term[which].dsp[0]=='\0') && (term[which].dsp_wait_q.size!= 0)) { //if 1st char of dsp buffer is null and the wait queue has PID
          //str ends & there's a waiter
         // release the 1st waiter in the wait queue:
            pid=DeQ(&term[which].dsp_wait_q);	//1. dequeue it from the wait queue
            pcb[pid].state=READY;			//2. update its state
            EnQ(pid, &ready_pid_q);			//3. enqueue it to ready PID queue
      }
	

	
      if(term[which].dsp[0]=='\0') return;	//if 1st character of dsp buffer is null, return; // nothing to dsp

      outportb(term[which].port, term[which].dsp[0]); // disp 1st char      

     for(i=0; i<BUFF_SIZE-1; i++) {	// conduct a loop, one by one {
         term[which].dsp[i]=term[which].dsp[i+1];	//move each character in dsp buffer forward by 1 character
         if(term[which].dsp[i]=='\0') {	//if encounter moving a NULL character, break loop
		break;
	 }
      }
	

   }

void ReadService(int fileno, char *str, int len) {
	int which;
	if(fileno==TERM1) { //determine which term_t to use (from the given argument)
		//"block" the running process to the terminal keyboard wait queue
		which=0;
		EnQ(run_pid, &term[which].kb_wait_q);
		pcb[run_pid].state=WAIT;
		run_pid=-1;
	} else if (fileno==TERM2) { //determine which term_t to use (from the given argument)
		//"block" the running process to the terminal keyboard wait queue
		which=1;
		EnQ(run_pid, &term[which].kb_wait_q);
		pcb[run_pid].state=WAIT;
		run_pid=-1;
	}
}

void TermService(int which){
	//phase five below
	
	char ch= inportb(term[which].status); //1. read the 'status' of the port
			 
      	if (ch==DSP_READY) { 	//2. if it's DSP_READY, 
		DspService(which);		 //call DspService()
	}
	
	if(ch==KB_READY) {	//3. if it's KB_READY,
      		KbService(which);	//call KbService()
	}
}
	   
void KbService(int which) {
      int pid; 
      char ch=inportb(term[which].port);	
		
      if(ch == (char) CTRL_C) {
      	   if (term[which].kb_wait_q.size > 0) {
	  	pid = DeQ(&term[which].kb_wait_q);
		pcb[pid].state = READY;
		EnQ(pid, &ready_pid_q);
		MyBzero((char *) pcb[pid].trapframe_p->ecx, 1);	   
		MyBzero(term[which].kb, BUFF_SIZE);
	        if (signal_table[pid][SIGINT] != NULL) {
		    WrapperService(pid, signal_table[pid][SIGINT]);
	        } else {
		    outportb(term[which].port, '^');   
	        }

	   }
	   return;
      }

      if(ch != '\r') {
        outportb(term[which].port, ch);	
	MyStrAppend(term[which].kb, ch);
        return; //and just return
	}

	
        outportb(term[which].port, '\n');
      //4. (not returning, continue) if there appears a waiting process in the kb wait queue of the terminal,	
      if(term[which].kb_wait_q.size > 0) {	 
	pid=DeQ(&term[which].kb_wait_q);	
        pcb[pid].state=READY;		
	EnQ(pid, &ready_pid_q); 
      	MyStrcpy((char *)pcb[pid].trapframe_p->ecx, term[which].kb);  //kb str it needs (use MyStrcpy)
      }
      term[which].kb[0] = '\0'; //5. reset the terminal kb string (put a single NUL at its start)	
}

void ForkService(int *ebx_p) {

	int *p;
	int delta;
	if (avail_pid_q.size == 0) {
		*ebx_p = -1;
		cons_printf("Cannot Create Child! No More Process!\r\n");
		return;
	}
	
	*ebx_p = DeQ(&avail_pid_q); // get a new child PID (set as what ebx_p points to)
	EnQ(*ebx_p, &ready_pid_q);  // enqueue the PID to be ready to run
	
	MyBzero((char*)&pcb[*ebx_p], sizeof(pcb_t));
	pcb[*ebx_p].state = RUN;
	pcb[*ebx_p].ppid = run_pid;
	
	MyMemcpy(proc_stack[*ebx_p], proc_stack[run_pid], PROC_STACK_SIZE);

	MyMemcpy((char *)signal_table[*ebx_p], (char*)signal_table[run_pid], SIG_NUM);	
	delta = proc_stack[*ebx_p] - proc_stack[run_pid];
	
	pcb[*ebx_p].trapframe_p = (trapframe_t *)((int)pcb[run_pid].trapframe_p+delta); 
	pcb[*ebx_p].trapframe_p->ebx = 0;
	
	pcb[*ebx_p].trapframe_p->esp += delta;
	pcb[*ebx_p].trapframe_p->ebp += delta;
	pcb[*ebx_p].trapframe_p->esi += delta;
	pcb[*ebx_p].trapframe_p->edi += delta;
	
	p = (int *)pcb[*ebx_p].trapframe_p->ebp;
	while (*p) {
		*p += delta;
		p = (int *) *p;
	}
}
	   
void SignalService(int pid, func_p_t p) {
	signal_table[pid][SIGINT] = p;
}

void GetPpidService(int *p){
	*p = pcb[run_pid].ppid;
}

void WrapperService(int pid, func_p_t p){

   trapframe_t *temp_tp = (trapframe_t *)proc_stack[pid];
   MyMemcpy((char*)temp_tp, (char*)pcb[pid].trapframe_p, sizeof(trapframe_t));

   pcb[pid].trapframe_p = (trapframe_t *) ((int)pcb[pid].trapframe_p - (sizeof(int)*2));	
   MyMemcpy((char*) pcb[pid].trapframe_p, (char*)temp_tp, sizeof(trapframe_t));

   MyMemcpy((char *) (int)pcb[pid].trapframe_p->ebp-8, (char *) &p, sizeof(int));
   MyMemcpy((char *) (int)pcb[pid].trapframe_p->ebp-12, (char *) &pcb[pid].trapframe_p->eip, sizeof(int));

   pcb[pid].trapframe_p->eip = (int) Wrapper;

   MyBzero((char *)temp_tp, sizeof(trapframe_t));
}

void ExitService(int exit_code) { // as child calls sys_exit()
	int ppid, *p;
	ppid = pcb[run_pid].ppid;
	if (pcb[ppid].state != WAITCHILD) {
	    pcb[run_pid].state = ZOMBIE;
	    run_pid = ppid;
	    if (signal_table[ppid][SIGCHILD] != NULL)
		    WrapperService(ppid, signal_table[ppid][SIGCHILD]);
	    return;	
	}
	pcb[ppid].trapframe->ebx = exit_code;
	pcb[ppid].trapframe->ecx = run_pid;
	pcb[ppid].state = READY;
	EnQ(ppid, &ready_pid_q);
	
	EnQ(run_pid, &avail_pid_q);
	MyBzero(&pcb[run_pid], sizeof(pcb_t));
	MyBzero(proc_stack[run_pid], sizeof(proc_stack));
	MyBzero(signaltable[run_pid], SIGNUM);
	
	run_pid = -1;
}

void WaitchildService(int *exit_code_p, int *child_pid_p) { // parent requests
      int child_pid, exit_code, i; // really only need these vars (besides args given)


      for(i=1; i<PROC_NUM; i++) { //search by looping thru each PCB in the PCB array:
         //if state ZOMBIE and ppid matches parent (run_pid) --> break loop (found)
      	if(pcb[i].state==ZOMBIE && pcb[i].ppid == run_pid) {
		child_pid = i;
		break;
	}
      }	

      if(i == PROC_NUM) {
	  pcb[pcb[child_pid].ppid].state = WAITCHILD;
	  run_pid = -1;
	  return;
      }
	
      *child_pid_p = child_pid; // found by searching the PCB for a zombie PID
      *exit_code_p = pcb[child_pid].trapframe->ebx; // the child's exit code is found in ebx in the trapframe.

      EnQ(run_pid, &avail_pid_q);
      MyBzero(&pcb[run_pid], sizeof(pcb_t));
      MyBzero(proc_stack[run_pid], sizeof(proc_stack));
      MyBzero(signaltable[run_pid], SIGNUM);
   }
