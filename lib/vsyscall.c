#include <inc/vsyscall.h>
#include <inc/lib.h>

static inline int32_t
vsyscall(int num)
{
	// LAB 12: Your code here.
	if (0 < num < NVSYSCALLS)
		return vsys[num];
	cprintf("0 < num<NVSYSCALLS!");
	return 0;
}

int vsys_gettime(void)
{
	return vsyscall(VSYS_gettime);
}
