// hello, world
#include <inc/lib.h>
#define PTE_COW		0x800

void
umain(int argc, char **argv)
{
	sys_page_alloc(sys_getenvid(), (void*)0xdeadb000, (PTE_U|PTE_P|PTE_W));
	sys_page_alloc(sys_getenvid(), (void*)0xa0000000, (PTE_U|PTE_P|PTE_W));
	sys_page_alloc(sys_getenvid(), (void*)0xa1000000, (PTE_U|PTE_P|PTE_W));
	sys_page_alloc(sys_getenvid(), (void*)0xa2000000, (PTE_U|PTE_P|PTE_W));
	*(int *)0xa0000000 = 1;

	cprintf("pages allocate\n\n");
	int i;
	for (i = 0; i < USTACKTOP; i += PGSIZE) {
		if ((uvpd[PDX(i)] & PTE_P) && // check if present
			(uvpt[PGNUM(i)] & PTE_P) && // check if present
			(uvpt[PGNUM(i)] & PTE_U)) {
			if (uvpt[PGNUM(i)] & PTE_W)
				cprintf("PTE_W ");
				else if (uvpt[PGNUM(i)] & PTE_COW)
					cprintf("PTE_COW ");
			cprintf("%x --> %x\n ", i, uvpt[PGNUM(i)]);
		}
	}
}
