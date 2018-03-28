// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;addr=addr;
	uint32_t err = utf->utf_err;err=err;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 9: Your code here.
	if (!((err & FEC_WR) &&     // check that the faulting access was a write
		(uvpd[PDX(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_P) &&
		(uvpt[PGNUM(addr)] & PTE_COW))) {   // check that write was to a COW page
		panic("pgfault faulting access panic");
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 9: Your code here.
	addr = ROUNDDOWN(addr, PGSIZE);
	// itask
	//sys_page_alloc(0, PFTEMP, PTE_W | PTE_P | PTE_U);
	sys_page_alloc(sys_getenvid(), (void *)PFTEMP, PTE_U | PTE_P);
	sys_page_map(sys_getenvid(), (void *)PFTEMP,sys_getenvid(), (void*)PFTEMP, PTE_U | PTE_W | PTE_P)
	memcpy(PFTEMP, addr, PGSIZE);
	sys_page_map(0, PFTEMP,	0, addr, (uvpt[PGNUM(addr)] & PTE_SYSCALL & ~PTE_COW) | PTE_W);
	sys_page_unmap(0, PFTEMP);
	//panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	// LAB 9: Your code here.
	//panic("duppage not implemented");
	void *addr = (void *)(pn * PGSIZE);
	if (((uvpt[pn] & PTE_W) || (uvpt[pn] & PTE_COW)) && !(uvpt[pn] & PTE_SHARE)) {
		int perm = ((uvpt[pn] & PTE_SYSCALL) & (~PTE_W)) | PTE_COW;
		if ((sys_page_map(0, addr, envid, addr, perm) < 0) ||
	  		(sys_page_map(0, addr, 0, addr, perm) < 0)) {
			panic("duppage: (w | cow) mapping failed!");
		}
	} else {
		if (sys_page_map(0, addr, envid, addr, (uvpt[pn] & PTE_SYSCALL)) < 0)
			panic("duppage: (r-only) mapping failed");
	}
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 9: Your code here.
	// set up page fault handler
	set_pgfault_handler(pgfault);

	// create a child
	envid_t id = sys_exofork();
	if (id < 0) {
		panic("fork panic");
	}
	if (id == 0) {
		thisenv = &envs[ENVX(sys_getenvid())]; // fixing "thisenv"
		return 0;
	}

	// copy address space and page fault handler setup to the child
	int i;
	for (i = 0; i < USTACKTOP; i += PGSIZE) {
		if ((uvpd[PDX(i)] & PTE_P) && // check if present
			(uvpt[PGNUM(i)] & PTE_P) && // check if present
			(uvpt[PGNUM(i)] & PTE_U)) {
			duppage(id, PGNUM(i));
		}
	}
	sys_page_alloc(id, (void *)(UXSTACKTOP - PGSIZE), PTE_U | PTE_W | PTE_P);

	// mark the child as runnable
	sys_env_set_status(id, ENV_RUNNABLE);

	extern void _pgfault_upcall();
	sys_env_set_pgfault_upcall(id, _pgfault_upcall);
	return id;
	//panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
