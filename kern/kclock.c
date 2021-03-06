/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/time.h>
#include <kern/kclock.h>
#include <inc/vsyscall.h>
#include <kern/vsyscall.h>

bool equal(struct tm a1, struct tm a2)
{
	if(a1.tm_sec == a2.tm_sec && a1.tm_min == a2.tm_min &&
	a1.tm_hour == a2.tm_hour && a1.tm_mday == a2.tm_mday &&
	a1.tm_mon == a2.tm_mon && a1.tm_year == a2.tm_year)
		return true;
	else return false;
}

int gettime(void)
{
	nmi_disable();
	// LAB 12: your code here
	struct tm tm;
	struct tm new;

	do {
		new = tm;

		outb(IO_RTC_CMND, RTC_SEC);
		uint8_t secValue = inb(IO_RTC_DATA);
		outb(IO_RTC_CMND, RTC_MIN);
		uint8_t minValue = inb(IO_RTC_DATA);
		outb(IO_RTC_CMND, RTC_HOUR);
		uint8_t hourValue = inb(IO_RTC_DATA);
		outb(IO_RTC_CMND, RTC_DAY);
		uint8_t dayValue = inb(IO_RTC_DATA);
		outb(IO_RTC_CMND, RTC_MON);
		uint8_t monthValue = inb(IO_RTC_DATA);
		outb(IO_RTC_CMND, RTC_YEAR);
		uint8_t yearValue = inb(IO_RTC_DATA);

		tm.tm_sec = BCD2BIN(secValue);
		tm.tm_min = BCD2BIN(minValue);
		tm.tm_hour = BCD2BIN(hourValue);
		tm.tm_mday = BCD2BIN(dayValue);
		tm.tm_mon = BCD2BIN(monthValue)-1;
		tm.tm_year = BCD2BIN(yearValue);
	} while (!equal(tm, new));
	nmi_enable();
	return timestamp(&tm);
}

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

