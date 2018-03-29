// hello, world
#include <inc/lib.h>
#define VA	((char *) 0xA0000000)

void
umain2(int argc, char **argv)
{
	int r;
	envid_t envid1 = thisenv->env_id;
	cprintf("hello, world\n");
	cprintf("i am environment %08x\n", envid1);
	if ((r = sys_page_alloc(envid1, VA, (PTE_U|PTE_P|PTE_W))) < 0)
		panic("sys_page_alloc: %i\n", r);
	cprintf("page allocated\n");
}
