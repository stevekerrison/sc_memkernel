#include <platform.h>
#include "kernel.h"

unsigned lock_resource;

void kernel_setup_threads(void)
{
  __asm__ __volatile__("getr %0,5":"=r"(lock_resource):);
  par (int i = 0; i < 8; i += 1)
  {
    kernel_setup();
  }
  return;
}

int main(void)
{
  par
  {
    on tile[0]:
    {
      c_setup();
      par
      {
        mem_svr();
        example();
      }
    }
    on tile[1]:
    {
      c_setup();
      par
      {
        mem_svr();
      }
    }
  }
}
