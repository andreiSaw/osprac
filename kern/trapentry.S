/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>

#ifdef CONFIG_KSPACE
.comm intr_ret_eip, 4
.comm intr_ebp_reg, 4
.comm intr_esp_reg, 4
.comm intr_cs, 4
.comm intr_eflags, 4
#endif


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps


.text

.globl _alltraps
.type _alltraps, @function;
.align 2
_alltraps:
// lab 8
  pushl %ds
  pushl %es
  pushal
  movw $GD_KD, %ax
  movw %ax, %ds
  movw %ax, %es
  pushl %esp
  call trap


.globl clock_thdlr
.type clock_thdlr, @function;
.align 2;
#ifdef CONFIG_KSPACE
clock_thdlr:
	popl intr_ret_eip
	popl intr_cs
	popl intr_eflags
	movl %ebp, intr_ebp_reg
	movl %esp, intr_esp_reg
	movl $0x0,%ebp
	movl $(bootstacktop),%esp
	pushl $GD_KD
	pushl intr_esp_reg
	pushl intr_eflags
	pushl intr_cs
	pushl intr_ret_eip
	pushl $0
	pushl $(IRQ_OFFSET + IRQ_CLOCK)
	pushl %ds
	pushl %es

	pushl %eax
	pushl %ecx
	pushl %edx
	pushl %ebx
	pushl intr_esp_reg
	pushl intr_ebp_reg
	pushl %esi
	pushl %edi

	pushl %esp  /* trap(%esp) */
	call trap
	jmp .
#else
TRAPHANDLER_NOEC(clock_thdlr, IRQ_OFFSET + IRQ_CLOCK);
// LAB 8: Your code here.
TRAPHANDLER_NOEC(thdlr0, 0);
TRAPHANDLER_NOEC(thdlr1, 1);
TRAPHANDLER_NOEC(thdlr2, 2);
TRAPHANDLER_NOEC(thdlr3, 3);
TRAPHANDLER_NOEC(thdlr4, 4);
TRAPHANDLER_NOEC(thdlr5, 5);
TRAPHANDLER_NOEC(thdlr6, 6);
TRAPHANDLER_NOEC(thdlr7, 7);
TRAPHANDLER(thdlr8, 8);
TRAPHANDLER(thdlr10, 10);
TRAPHANDLER(thdlr11, 11);
TRAPHANDLER(thdlr12, 12);
TRAPHANDLER(thdlr13, 13);
TRAPHANDLER(thdlr14, 14);
TRAPHANDLER_NOEC(thdlr16, 16);
TRAPHANDLER(thdlr17, 17);
TRAPHANDLER_NOEC(thdlr18, 18);
TRAPHANDLER_NOEC(thdlr19, 19);
TRAPHANDLER_NOEC(thdlr48, 48);
TRAPHANDLER_NOEC(irq_thdlr0, IRQ_OFFSET + 0);
TRAPHANDLER_NOEC(irq_thdlr1, IRQ_OFFSET + 1);
TRAPHANDLER_NOEC(irq_thdlr4, IRQ_OFFSET + 4);
TRAPHANDLER_NOEC(irq_thdlr7, IRQ_OFFSET + 7);
TRAPHANDLER_NOEC(irq_thdlr14, IRQ_OFFSET + 14);
TRAPHANDLER_NOEC(irq_thdlr19, IRQ_OFFSET + 19);
#endif
