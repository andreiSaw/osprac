// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/tsc.h>
#include <kern/pmap.h>
#include <kern/trap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
    { "message", "Display information about the kernel", mon_message },
    { "backtrace", "Display stack backtrace", mon_backtrace },
    { "timer_start", "start timer", mon_start },
    { "timer_stop", "stop timer", mon_stop },
	{ "pages", "Display free and allocated pages", mon_pages }
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", (uint32_t)_start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n",
            (uint32_t)entry, (uint32_t)entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n",
            (uint32_t)etext, (uint32_t)etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n",
            (uint32_t)edata, (uint32_t)edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n",
            (uint32_t)end, (uint32_t)end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int 
mon_pages(int argc, char **argv, struct Trapframe *tf) {
	size_t i, j;
	for (i = 1, j = 0; i < npages; i++) {
		if (!pages[i - 1].pp_link && pages[i].pp_link) {
			if (i - j == 1) {
				cprintf("%d ALLOCATED\n", j + 1);
			} else {
				cprintf("%d..%d ALLOCATED\n", j + 1, i);
			}
			j = i;
		} else if (pages[i - 1].pp_link && !pages[i].pp_link) {
			if (i - j == 1) {
				cprintf("%d FREE\n", j + 1);
			} else {
				cprintf("%d..%d FREE\n", j + 1, i);
			}
			j = i;
		}			
	}
	if (j == npages - 1) {
		if (!pages[j].pp_link) {
			cprintf("%d ALLOCATED\n", j + 1);
		} else {
			cprintf("%d FREE\n", j + 1);
		}
	} else {
		if (!pages[j].pp_link) {
			cprintf("%d..%d ALLOCATED\n", j + 1, npages);
		} else {
			cprintf("%d..%d FREE\n", j + 1, npages);
		}
	}		
	return 0;
}

int
mon_message(int argc, char **argv, struct Trapframe *tf)
{
	cprintf("Special Message %o\n", 9);
	return 0;
}

int
mon_start(int argc, char **argv, struct Trapframe *tf)
{
	timer_start();
	return 0;
}

int
mon_stop(int argc, char **argv, struct Trapframe *tf)
{
	timer_stop();
	return 0;
}


int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
    cprintf("Stack backtrace:\n");
    uint32_t *ebp = (uint32_t *) read_ebp();
    //int i = 0;
    struct Eipdebuginfo *info = NULL;

	// while (ebp < (uint32_t *)0x0010f000) {
    while (ebp) {
        uint32_t base = (uint32_t)(ebp);
        uint32_t eip = *(ebp + 1);
        uint32_t arg1 = *(ebp + 2);
        uint32_t arg2 = *(ebp + 3);
        uint32_t arg3 = *(ebp + 4);
        uint32_t arg4 = *(ebp + 5);
        uint32_t arg5 = *(ebp + 6);

        cprintf("  ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n", base, eip, arg1, arg2, arg3, arg4, arg5);
        
        if(debuginfo_eip(eip, info) >= 0) {
            cprintf("    %s:%d: %.*s+%d\n", info->eip_file, info->eip_line, info->eip_fn_namelen, info->eip_fn_name, eip - (uint32_t) info->eip_fn_addr);
        }

        ebp = (uint32_t *) *ebp;
    }
    //int i;
    //for (i = 0; i < KSTKSIZE; i++) {
    //    cprintf("ebp %08x  eip   args ", read_ebp());
    //}
	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
