
.section .dp.data, "awd", @progbits
.align 4
.extern kernel_stack
.globl kernel_stack
kernel_stack:
	.word 0
.align 1024 //8 * 32-words
old_exception_handler:
  .word 0

.section .text

.extern ex_handler
.globl ex_handler
.globl ex_handler.nstackwords
.globl ex_handler.maxthreads
.globl ex_handler.maxtimers
.globl ex_handler.maxchanends
.linkset ex_handler.nstackwords, 2
.linkset ex_handler.maxthreads,  0
.linkset ex_handler.maxtimers,   0
.linkset ex_handler.maxchanends, 0
.cc_top ex_handler.func, ex_handler
.align 4
ex_handler:
  entsp 0x2
  // old_exception anything not ET_LOAD_STORE
  get r11,et
  sub r11,r11,5
  bt r11,old_exception
  // old_exception alignment errors
  get r11,ed
  ldc r0,0x3
  and r11,r11,r0
  bt r11,old_exception
  //Get the SPC
  stw spc,sp[1]
  ldw r11,sp[1]
  // old_exception if not plain ldw/stw
  ldc r1,0
  ld16s r4,r11[r1]
  get r11,ed
  ldc r2,0xf800
  and r0,r4,r2
  bf r0,ex_stwi   //Looks like a STWI
  ldc r2,0x0800
  eq r3,r0,r2
  bt r3,ex_ldwi   //Looks like a LDWI
  //None of the instructions we want were found - old_exception time!
  bu old_exception
ex_stwi:
  mov r0,r11
  mov r1,r4
  bl stwi_remote
  bu ex_skip  
ex_ldwi:
  mov r0,r11
  mov r1,r4
  bl ldwi_remote
  bu ex_skip
ex_skip:
  // Skip over the exception instruction, having completed it via other means
  stw spc,sp[1]
  ldw r11,sp[1]
  add r11,r11,2
  stw r11,sp[1]
  ldw spc,sp[1]
  retsp 0x2

old_exception:
  ldw r11,dp[old_exception_handler]
  set kep,r11
  retsp 0x2
.cc_bottom ex_handler.func



.extern kernel_setup
.globl kernel_setup
.globl kernel_setup.nstackwords
.globl kernel_setup.maxthreads
.globl kernel_setup.maxtimers
.globl kernel_setup.maxchanends
.linkset kernel_setup.nstackwords, 1
.linkset kernel_setup.maxthreads,  0
.linkset kernel_setup.maxtimers,   0
.linkset kernel_setup.maxchanends, 0
.cc_top kernel_setup.func, kernel_setup
.align 4
kernel_setup:
  ldaw r2,dp[kernel_stack]
  get r11,id
  ldc r3,128
  mul r3,r3,r11
  add r2,r2,r3
  ldaw r1,sp[0]
  set sp,r2
  stw r1,sp[0]
  krestsp 0
  get r11,kep
  //ldaw r2,dp[old_exception_handler]
  stw r11,dp[old_exception_handler]
  ldap r11,kernel
  set kep,r11
  retsp 0
.cc_bottom kernel_setup.func

.extern kernel
.globl kernel
.globl kernel.nstackwords
.globl kernel.maxthreads
.globl kernel.maxtimers
.globl kernel.maxchanends
.linkset kernel.nstackwords, 13
.linkset kernel.maxthreads,  0
.linkset kernel.maxtimers,   0
.linkset kernel.maxchanends, 0
.cc_top kernel.func, kernel
.align 128              // align the kernel section to 128 bytes
kernel:                 // entry point for exceptions
    kentsp 13           // increase current stack
    stw r0,sp[1]        // Save r0-r11, we might poke these later anyway.
    stw r1,sp[2]
    stw r2,sp[3]
    stw r3,sp[4]
    stw r4,sp[5]
    stw r5,sp[6]
    stw r6,sp[7]
    stw r7,sp[8]
    stw r8,sp[9]
    stw r9,sp[10]
    stw r10,sp[11]
    stw r11,sp[12]
    bl ex_handler       // jump to the exception handler written in C
    ldw r0,sp[1]        // Get back r0-r11, some of which may have been changed
    ldw r1,sp[2]
    ldw r2,sp[3]
    ldw r3,sp[4]
    ldw r4,sp[5]
    ldw r5,sp[6]
    ldw r6,sp[7]
    ldw r7,sp[8]
    ldw r8,sp[9]
    ldw r9,sp[10]
    ldw r10,sp[11]
    ldw r11,sp[12]
    krestsp 13
    kret
.cc_bottom kernel.func
