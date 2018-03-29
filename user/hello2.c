// hello, world
#include <inc/lib.h>

void handler(struct UTrapframe *utf)
{
	int r;

	void *addr = (void*)utf->utf_fault_va;

	cprintf("fault %x\n", (uint32_t)addr);
	if ((r = sys_page_alloc(0, ROUNDDOWN(addr, PGSIZE), (PTE_U|PTE_P|PTE_W))) < 0)
		panic("allocating at %x in page fault handler: %i", (uint32_t)addr, r);
	snprintf((char*) addr, 100, "this string was faulted in at %x", (uint32_t)addr);
}

void
umain(int argc, char **argv)
{
	int r;
	r = sys_page_alloc(sys_getenvid(), (void*)0xDeadBeef, (PTE_U|PTE_P|PTE_W));
	cprintf("pages allocate\n\n");
	cprintf("%s\n", (char*)0xDeadBeef);
	int i;
	for (i = 0; i < USTACKTOP; i += PGSIZE) {
		if ((uvpd[PDX(i)] & PTE_P) && // check if present
			(uvpt[PGNUM(i)] & PTE_P) && // check if present
			(uvpt[PGNUM(i)] & PTE_U)) {
			cprintf("%d --> %d", PGNUM(i), uvpt[PGNUM(i)]& 0xFFFFF000);
		}
	}
}
