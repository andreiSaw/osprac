// hello, world
#include <inc/lib.h>
#define VA	((char *) 0xA0000000)

void
umain(int argc, char **argv)
{
	cprintf("hello, world\n");
	cprintf("i am environment %08x\n", thisenv->env_id);
}
