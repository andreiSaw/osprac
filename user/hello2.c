// hello, world
#include <inc/lib.h>
#define PTE_COW		0x800

void
umain(int argc, char **argv)
{
	int r;
	r = sys_page_alloc(sys_getenvid(), (void*)0xdeadb000, (PTE_U|PTE_P|PTE_W));
	cprintf("pages allocate\n\n");
	cprintf("%s\n", (char*)0xdeadb000);
	int i;
	for (i = 0; i < USTACKTOP; i += PGSIZE) {
		if ((uvpd[PDX(i)] & PTE_P) && // check if present
			(uvpt[PGNUM(i)] & PTE_P) && // check if present
			(uvpt[PGNUM(i)] & PTE_U)) {
			if (uvpt[PGNUM(i)] & PTE_W)
				cprintf("PTE_W ");
				else if (uvpt[PGNUM(i)] & PTE_COW)
					cprintf("PTE_COW ");
			cprintf("%d --> %d\n ", PGNUM(i), uvpt[PGNUM(i)]& 0xFFFFF000);
		}
	}
}
