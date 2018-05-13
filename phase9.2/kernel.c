// kernel.c, 159
// Team Name: CENA (Members: Jimmy Tran, Erik Gonzalez, & Ann Theriot-Thirakoune) 
#include "spede.h"         
#include "kernel_types.h"
#include "kernel_data.h"
#include "entry.h"         
#include "tools.h"         
#include "proc.h"          
#include "services.h"     

struct i386_gate *IDT_p;
int current_time;
int run_pid;                       
pid_q_t ready_pid_q, avail_pid_q;  
pcb_t pcb[PROC_NUM];             
char proc_stack[PROC_NUM][PROC_STACK_SIZE]; 
semaphore_t video_sem;			// Phase 3
term_t term[2];				//Phase 4
func_p_t signal_table[PROC_NUM][SIG_NUM];  //phase 7

pid_q_t page_q;	//phase 9

void InitKernelData(void) {        // init kernel data
  	int i;
	run_pid=-1;	
        MyBzero((char *)proc_stack, (PROC_STACK_SIZE*PROC_NUM));  
        MyBzero((char *)signal_table, sizeof(signal_table));	// phase 7
        MyBzero((char *)&avail_pid_q, sizeof(avail_pid_q));
        MyBzero((char *)&ready_pid_q, sizeof(ready_pid_q));

	
	MyBzero((char *)&term[0], sizeof(term_t)); //first zero-ed it out
	MyBzero((char *)&term[1], sizeof(term_t)); //first zero-ed it out
	
	term[0].port=0x2f8;
	term[1].port=0x3e8;
	
	term[0].status = 0x2f8 + IIR;  
	term[1].status = 0x3e8 + IIR;
	

	for(i=0; i<PROC_NUM; i++) {
		EnQ(i, &avail_pid_q );	//enqueue all PID numbers into the available PID queue
	}
	for(i=0; i<PAGE_NUM; i++) {
		EnQ(i, &page_q);	//enqueue available pages
	}
	
}

void InitKernelControl(void) {     // init kernel control 
	current_time=0;
	video_sem.val = 1;
	video_sem.wait_q.size=0;

	
	IDT_p = get_idt_base(); //get IDT location
	//show: "IDT located at DRAM addr %x (%d).\n" (both address of IDT)
	cons_printf("IDT located at DRAM addr %x (%d). \n", IDT_p, IDT_p );
	fill_gate(&IDT_p[TIMER], (int)TimerEntry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&IDT_p[SYSCALL], (int)SyscallEntry, get_cs(), ACC_INTR_GATE, 0);
	//phase four stuff below
	
	fill_gate(&IDT_p[TERM1], (int)Term1Entry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&IDT_p[TERM2], (int)Term2Entry, get_cs(), ACC_INTR_GATE, 0);

	outportb(0x21, ~(16 | 8 | 1));	//0x21 is PIC mask, ~1 is mask, turn on IRQ 0 3,4
}

 void InitTerm(void) {
      int i, j;

      for(j=0; j<2; j++) { // alter two terminals
         // set baud, Control Format Control Register 7-E-1 (data-parity-stop bits)
         // raise DTR, RTS of the serial port to start read/write
         outportb(term[j].port + CFCR, CFCR_DLAB);             // CFCR_DLAB is 0x80
         outportb(term[j].port + BAUDLO, LOBYTE(115200/9600)); // period of each of 9600 bauds
         outportb(term[j].port + BAUDHI, HIBYTE(115200/9600));
         outportb(term[j].port + CFCR, CFCR_PEVEN | CFCR_PENAB | CFCR_7BITS);

         outportb(term[j].port + IER, 0);
         outportb(term[j].port + MCR, MCR_DTR|MCR_RTS|MCR_IENABLE);
         outportb(term[j].port + IER, IER_ERXRDY|IER_ETXRDY);       // enable TX & RX intr

         for(i=0;i<LOOP;i++) asm("inb $0x80");              // let term reset

         inportb(term[j].port); // clean up buffer (extra key at PROCOMM screen)
      }
   }

void ProcScheduler(void) {            
   if(run_pid >0){
	return;
   }
   if(ready_pid_q.size == 0){	// if the ready_pid_q is empty: 
	run_pid=0;		//let run_pid be zero
   }else{			 // else: get the 1st one in ready_pid_q to be run_pid
   	run_pid=DeQ(&ready_pid_q);
	pcb[run_pid].state= RUN;
   	pcb[run_pid].totaltime = pcb[run_pid].totaltime + pcb[run_pid].runtime; 
  	pcb[run_pid].runtime= 0;	 
   }
}

int main(void) {  
  	InitKernelData();	// initialize kernel data
  	InitKernelControl();	//initialize kernel control
	InitTerm();		//new subroutine for phase four below

  	NewProcService(IdleProc); 	//call NewProcService() with address of IdleProc to create it
  	ProcScheduler();	//call ProcScheduler() to select a run_pid
  	ProcLoader(pcb[run_pid].trapframe_p);		//call ProcLoader() with address of the trapframe of the selected run_pid

   	return 0; // compiler needs for syntax altho this statement is never exec
}

void Kernel(trapframe_t *trapframe_p) {   // kernel code runs (100 times/second)
	char key;
   	pcb[run_pid].trapframe_p = trapframe_p; //save the trapframe_p to the PCB of run_pid
	
	switch(trapframe_p->intr_num) {
		case TIMER:	//32
			TimerService();
			break;
		case SYSCALL:	//128
			SyscallService(trapframe_p); 
			break;
		case TERM1: 	//35
			TermService(0);
			outportb(0x20,0x63);
			break;
		case TERM2:	//36
			TermService(1);
			outportb(0x20, 0x64);
			break;
	}

   	if (cons_kbhit()) {     // poll keybord, retursn 1 if pressed
		key  = cons_getchar();
      	if (key == 'n') {
          	NewProcService(UserProc);
      	} else if (key == 'b') {
          	breakpoint();
      	}
	}	
	ProcScheduler();
	
	if(pcb[run_pid].TT != '\0' ) {	
		set_cr3(pcb[run_pid].TT); 
	} 
	
	ProcLoader(VM_TF);
}


