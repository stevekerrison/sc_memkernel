#include <platform.h>
#include <print.h>
#include <stdlib.h>
#include "kernel.h"

extern unsigned lock_resource;

//Assumes we're on a 2-tile device and that the node IDs are 0x8000 and 0x0000
unsigned naive_translator(void *addr)
{
  if ((((unsigned)addr & 0xffff0000) - 0x00020000) == 0)
    return 0x1f02;
  return 0x80001f02;
}

//This translator does nothing, obviously. Using it will ruin your day.
unsigned translator_stub(void *addr)
{
  printstrln("TRANSLATE!");
  return 0;
}
unsigned (*translator)(void *) = translator_stub;

//https://github.com/rlsosborne/tool_axe
void decode_3r(unsigned short instr, unsigned ops[3])
{
  unsigned highbits = (instr >> 6) & 0x1f;
  ops[0] = ( highbits        % 3) | ((instr >> 4) & 0x3);
  ops[1] = ((highbits / 3)   % 3) | ((instr >> 2) & 0x3);
  ops[2] = ( highbits / 9)        | ( instr       & 0x3);
  return;
}

void ldwi_remote(unsigned *addr, unsigned short instr)
{
  unsigned op1d, op2b, op3i, ops[3], dst = translator(addr), data, lockdata, c;
  decode_3r(instr,ops);
  op1d = ops[0];
  op2b = ops[1];
  op3i = ops[2];
  __asm__ __volatile__("in %0,res[%1]\n":"=r"(lockdata):"r"(lock_resource));
  __asm__ __volatile__("getr %0,2\n":"=r"(c));
  __asm__ __volatile__("setd res[%0],%1"::"r"(c),"r"(dst));
  __asm__ __volatile__("out res[%0],%0"::"r"(c));
  __asm__ __volatile__("outct res[%0],%1"::"r"(c),"r"(0x83));
  __asm__ __volatile__("outt res[%0],%1"::"r"(c),"r"((unsigned)addr >> 8));
  __asm__ __volatile__("outt res[%0],%1"::"r"(c),"r"(addr));
  __asm__ __volatile__("chkct res[%0],0x3"::"r"(c));
  __asm__ __volatile__("in %0,res[%1]":"=r"(data):"r"(c));
  __asm__ __volatile__(
    "outct res[%0],1\n"
    "chkct res[%0],1\n"
    "freer res[%0]\n"
    ::"r"(c));
  __asm__ __volatile__("out res[%0],%0"::"r"(lock_resource));
  {
    unsigned tid = get_logical_core_id();
    unsigned stack_addr;
    __asm__ __volatile__("ldaw %0,dp[kernel_stack]":"=r"(stack_addr));
    stack_addr += (128 * tid);// - (12 * 4);
    stack_addr -= (12 * 4);
    __asm__ __volatile__("stw %0,%1[%2]"::"r"(data),"r"(stack_addr),"r"(op1d));
  }
  return;
}

void stwi_remote(unsigned *addr, unsigned short instr)
{
  unsigned op1s, op2b, op3i, ops[3], dst = translator(addr), data, lockdata, c;
  decode_3r(instr,ops);
  op1s = ops[0];
  op2b = ops[1];
  op3i = ops[2];
  {
    unsigned tid = get_logical_core_id();
    unsigned stack_addr;
    __asm__ __volatile__("ldaw %0,dp[kernel_stack]":"=r"(stack_addr));
    stack_addr += (128 * tid);// - (12 * 4);
    stack_addr -= (12 * 4);
    __asm__ __volatile__("ldw %0,%1[%2]":"=r"(data):"r"(stack_addr),"r"(op1s));
  }
  __asm__ __volatile__("in %0,res[%1]\n":"=r"(lockdata):"r"(lock_resource));
  __asm__ __volatile__("getr %0,2\n":"=r"(c));
  __asm__ __volatile__("setd res[%0],%1"::"r"(c),"r"(dst));
  __asm__ __volatile__("out res[%0],%0"::"r"(c));
  __asm__ __volatile__("outct res[%0],%1"::"r"(c),"r"(0x88));
  __asm__ __volatile__("outt res[%0],%1"::"r"(c),"r"((unsigned)addr >> 8));
  __asm__ __volatile__("outt res[%0],%1"::"r"(c),"r"(addr));
  __asm__ __volatile__("out res[%0],%1"::"r"(c),"r"(data));
  __asm__ __volatile__("chkct res[%0],0x3"::"r"(c));
  __asm__ __volatile__(
    "outct res[%0],1\n"
    "chkct res[%0],1\n"
    "freer res[%0]\n"
    ::"r"(c));
  __asm__ __volatile__("out res[%0],%0"::"r"(lock_resource));
  return;
}

void mem_bad_req(unsigned c)
{
  // NACK and END that son of a gun
  __asm__ __volatile__(
    "outct res[%0],4\n"
    "chkct res[%0],1\n"
    "outct res[%0],1\n"
    ::"r"(c));
  return;
}

void mem_svr(void)
{
  unsigned c = 0, nc = 0, ca[0x20];
  while((c & 0xffff) != SVR_CE)
  {
    __asm__ __volatile__("getr %0,2":"=r"(c):);
    ca[nc++] = c;
  }
  for (int i = 0; i < nc-1; i += 1)
  {
    __asm__ __volatile__("freer res[%0]"::"r"(ca[i]));
  }
  while(1)
  {
    unsigned dst, op, addr = 0, tmp;
    void *addr_pt = (void *)0x10000;
    __asm__ __volatile__(
      "in %0,res[%4]\n"
      "setd res[%4],%0\n"
      "inct %1,res[%4]\n"
      "int %2,res[%4]\n"
      "shl %2,%2,8\n"
      "int %3,res[%4]\n"
      "or %2,%2,%3\n"
      :"=r"(dst),"=r"(op),"=r"(addr),"=r"(tmp):"r"(c));
    switch (op)
    {
      case 0x83: //READ4
        if (addr & 0x3) {
          mem_bad_req(c);
          continue;
        }
        addr_pt = (void *)(addr + 0x10000);
        unsigned reply = *(unsigned *)addr_pt;
        __asm__ __volatile__(
          "outct res[%0],3\n"
          "out res[%0],%1\n"
          ::"r"(c),"r"(reply));
        break;
      case 0x88: //WRITE4
        if (addr & 0x3) {
          mem_bad_req(c);
          continue;
        }
        addr_pt = (void *)(addr + 0x10000);
        __asm__ __volatile__(
          "in r4,res[%0]\n"
          "stw r4,%1[0]\n"
          "outct res[%0],3\n"
          ::"r"(c),"r"(addr_pt):"r4");
        break;
      default:
        //BAD REQUEST
        mem_bad_req(c);
        continue;
        break; //LOL
    }
    // END of transaction!
    __asm__ __volatile(
      "chkct res[%0],1\n"
      "outct res[%0],1\n"
      ::"r"(c));
  }
}

void kernel_setup_address_translator(unsigned (*translator_fn)(void *))
{
  translator = translator_fn;
  return;
}

//Mess around with pointers, watch the world burn
void example(void)
{
  unsigned *x = (unsigned *)0x10000;
  unsigned i;
  unsigned t, tv1, tv2, v;
  //Waste some time so that the chanends are allocated before we use them.
  __asm__ __volatile__(
    "ldc %0,5000\n"
    "sub %0,%0,1\n"
    "bt %0,-0x2\n"
    :"=r"(i));
    
  //Store to an out of range memory address, to show stw works too
  __asm__ __volatile__("stw %0,%1[0]"::"r"(0xbabecafe),"r"(0x20000));
    
  //Using timers... who needs XC or builtins?
  __asm__ __volatile__("getr %0,1\n":"=r"(t));
  __asm__ __volatile__("in %0,res[%1]\n":"=r"(tv1):"r"(t));
  v = *x;
  __asm__ __volatile__("in %0,res[%1]\n":"=r"(tv2):"r"(t));
  printhexln(v);
  printstr("Normal read ticks: ");
  printintln(tv2 - tv1);
  
  //Memory address is out of range!
  x = (unsigned *)0x20000;

  __asm__ __volatile__("in %0,res[%1]\n":"=r"(tv1):"r"(t));
  v = *x;
  __asm__ __volatile__("in %0,res[%1]\n":"=r"(tv2):"r"(t));
  //printstr("'Remote' read of self: ");
  printhexln(v);
  printstr("'Remote' read of self ticks: ");
  printintln(tv2 - tv1);
  
  //Memory address is out of range!
  x = (unsigned *)0x30000;

  __asm__ __volatile__("in %0,res[%1]\n":"=r"(tv1):"r"(t));
  v = *x;
  __asm__ __volatile__("in %0,res[%1]\n":"=r"(tv2):"r"(t));
  printhexln(v);
  printstr("Remote read of tile[1] ticks: ");
  printintln(tv2 - tv1);
  
  __asm__ __volatile__("freer res[%0]\n"::"r"(t));
  exit(0);
}

//Example of how we set stuff up
void c_setup(void)
{
  kernel_setup_threads();
  kernel_setup_address_translator(naive_translator);
}


