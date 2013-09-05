#ifndef _MEMKERNEL_H
#define _MEMKERNEL_H

#define SVR_CE    0x1f02
#define MEM_OFF   0x10000

//Setup all threads
void kernel_setup_threads(void);
//Setup current thread
void kernel_setup(void);
//Thread that handles memory read/write requests over channels
void mem_svr(void);
void c_setup(void);
void example(void);

#endif //_MEMKERNEL_H
