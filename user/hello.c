// hello, world
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	envid_t envid1 = thisenv->env_id;
	cprintf("hello, world\n");
	cprintf("i am environment %08x\n", envid1);
	int r = sys_page_alloc(0, (void *)(UXSTACKTOP - PGSIZE), (PTE_U|PTE_P|PTE_W));
	cprintf("page allocated %i", r);
}
