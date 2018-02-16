/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <kern/kclock.h>

void
rtc_init(void)
{
	nmi_disable();
	// LAB 4: your code here

	outb(0x70, RTC_BREG); // set to reg b
	uint8_t reg_b = inb(0x71); // read reg b
	outb(0x70, RTC_BREG); // reset to reg b
	outb(0x71, reg_b | RTC_PIE); // write reg b

	// task 5
	outb(0x70, RTC_AREG); // set to reg a
	uint8_t reg_a = inb(0x71); // read reg a
	outb(0x70, RTC_AREG); // set to reg a
	outb(0x71, reg_a | 15); // write reg a

	//irq_setmask_8259A(IRQ_CLOCK);
	nmi_enable();
}

uint8_t
rtc_check_status(void)
{
	uint8_t status = 0;
	// LAB 4: your code here

	outb(0x70, RTC_CREG); // set to reg c
	status = inb(0x71); // read reg c

	return status;
}

unsigned
mc146818_read(unsigned reg)
{
	outb(IO_RTC_CMND, reg);
	return inb(IO_RTC_DATA);
}

void
mc146818_write(unsigned reg, unsigned datum)
{
	outb(IO_RTC_CMND, reg);
	outb(IO_RTC_DATA, datum);
}

