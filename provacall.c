#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/kernel.h>
#include <string.h>
#include <errno.h>
#define __NR_sys_defclose 379


long defcall(pid_t pid)
{
return syscall(__NR_sys_defclose, pid );
}

int main()
{
pid_t pida;
int ret;
printf("Inserisci il pid\n");
scanf("%u",&pida);
ret = defcall(pida);
if (ret != 0) 
 {
  int error_code = errno;
  printf("in errno trovo %u", error_code);
 }
}
