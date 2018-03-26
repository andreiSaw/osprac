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
	// uvpt - va of vitrual page table

	pte_t pte = uvpt[PGNUM(addr)];
	if (!(err & FEC_WR) ||	// FEC_WR - pgfault caused by a write
		!(pte & PTE_COW)) { 	// check if access not write
		panic("pgfault: faulting access panic");
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 9: Your code here.
	addr = ROUNDDOWN(addr, PGSIZE);
	sys_page_alloc(0, PFTEMP, PTE_W | PTE_P | PTE_U);
	memcpy(PFTEMP, addr, PGSIZE); // copy from old addr to temp location

	int perm = (uvpt[PGNUM(addr)] & PTE_SYSCALL & ~PTE_COW) | PTE_W; // set permission without copy-on-write
	sys_page_map(0, PFTEMP, 0, addr, perm); // map temp to old
	sys_page_unmap(0, PFTEMP);
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
	void *addr = (void *)(pn * PGSIZE);
	pte_t pte = uvpt[pn];

	if (((pte & PTE_W) || (pte & PTE_COW)) && !(pte & PTE_SHARE)) {
		// if the page is writable or copy-on-write
		int perm = PTE_COW | PTE_U | PTE_P;
		if ((sys_page_map(0, addr, envid, addr, perm) < 0) || 	// mark new mapping as copy-on-write
			(sys_page_map(0, addr, 0, addr, perm) < 0)) { 		// mark old mapping as copy-on-write, otherwise new env would see the change in this env
			panic("duppage : w | cow mapping failed");
		}
	} else {
		int perm = PTE_U | PTE_P;
		if (sys_page_map(0, addr, envid, addr, perm) < 0) {
			panic("duppage : readonly mapping failed");
		}
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
	set_pgfault_handler(pgfault);

	envid_t env_id = sys_exofork();
	if (env_id < 0) {
		panic("fork panic");
	}
	if (env_id == 0) {
		thisenv = &envs[ENVX(sys_getenvid())]; // fix "thisenv"
		return 0;
	}


	// uvpd - va of current page directory
	// uvpt - va of vitrual page table
	// PDX - index of page directory
	// PGNUM - index of page in page table

	int i;
	for (i = 0; i < USTACKTOP; i += PGSIZE) {
		if ((uvpd[PDX(i)] & PTE_P) &&
			(uvpt[PGNUM(i)] & PTE_P) &&
			(uvpt[PGNUM(i)] & PTE_U)) {
			duppage(env_id, PGNUM(i));
		}
	}
	sys_page_alloc(env_id, (void *)(UXSTACKTOP - PGSIZE), PTE_U | PTE_W | PTE_P);

	sys_env_set_status(env_id, ENV_RUNNABLE);

	extern void _pgfault_upcall();
	sys_env_set_pgfault_upcall(env_id, _pgfault_upcall);
	return env_id;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
