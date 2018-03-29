// hello, world
#include <inc/lib.h>

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
			cprintf("%d --> %d", PGNUM(i), uvpt[PGNUM(i)]& 0xFFFFF000);
		}
	}
}
