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
	{ "hello_world", "Display 'hello, world!'", mon_hello_world },
	{ "backtrace", "Display stacktrace", mon_backtrace },
	{ "timer_start", "Timer start", mon_timer_start },
	{ "timer_stop", "Timer stop", mon_timer_stop },
	{ "page_list", "Display page list", mon_page_list },
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
mon_hello_world(int argc, char **argv, struct Trapframe *tf)
{
	cprintf("hello world!\n");
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	cprintf("Stack backtrace:\n");

	int ebp = read_ebp();
	while (ebp != 0) {
		int eip = *((int*)ebp + 1);
		int arg1 = *((int*)ebp + 2);
		int arg2 = *((int*)ebp + 3);
		int arg3 = *((int*)ebp + 4);
		int arg4 = *((int*)ebp + 5);
		int arg5 = *((int*)ebp + 6);
		cprintf("  ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n", ebp, eip, arg1, arg2, arg3, arg4, arg5);
		
		struct Eipdebuginfo* info = NULL;
		debuginfo_eip(eip, info);
		cprintf("    %s:%d: ", info->eip_file, info->eip_line);
		cprintf("%.*s", info->eip_fn_namelen, info->eip_fn_name);
		cprintf("+%d\n", (eip - info->eip_fn_addr));

		ebp = *((int*)ebp);
	}
	
	return 0;
}

int
mon_timer_start(int argc, char **argv, struct Trapframe *tf)
{
	timer_start();	
	return 0;
}

int
mon_timer_stop(int argc, char **argv, struct Trapframe *tf)
{
	timer_stop();	
	return 0;
}

int
mon_page_list(int argc, char **argv, struct Trapframe *tf)
{
	bool is_free = pages[0].pp_ref == 1; 
	int start_index = 0; 
	if (npages > 1) { 
		int i; 
		for (i = 1; i < npages; i++) { 
			bool current_page_is_free = pages[i].pp_ref == 0; 
			if (current_page_is_free == is_free) { 
				continue; 
			} else { 
				cprintf("%d", start_index + 1); 
				if (i - start_index > 1) { 
					cprintf("..%d", i); 
				} 
				cprintf(" %s\n", is_free ? "FREE" : "ALLOCATED"); 
				start_index = i; 
				is_free = current_page_is_free; 
			} 
		} 
		cprintf("%d", start_index + 1); 
		if (i - start_index > 1) { 
			cprintf("..%d", i); 
		} 
		cprintf(" %s\n", is_free ? "FREE" : "ALLOCATED"); 
	} 
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
