#include <inc/types.h>
#include <kern/alloc.h>
#include <inc/assert.h>
#include <kern/spinlock.h>


#define SPACE_SIZE 5*0x1000

static uint8_t space[SPACE_SIZE];
static Header base = { .s = { .next = (Header *) space, .prev = (Header *) space, .size = 0 } }; /* empty list to get started */

static Header *freep = NULL; /* start of free list */


static void check_list(void)
{
	Header *p, *prevp;
	__asm __volatile("cli;");
	prevp = freep;
	for( p = prevp->s.next; p != freep; p = p->s.next ) {
		if ( prevp != p->s.prev ) {
			panic("Corrupted list.\n");
			for(;;){}
		}
		prevp = p;
	}
	__asm __volatile("sti;");
}

/* malloc: general-purpose storage allocator */
void *
test_alloc(uint8_t nbytes)
{
	Header *p;
	unsigned nunits;

	nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;

	// my code
	spin_lock(&kernel_lock);
	// end of my code

	if (freep == NULL) { /* no free list yet */
		((Header *) &space)->s.next = (Header *) &base;
		((Header *) &space)->s.prev = (Header *) &base;
		((Header *) &space)->s.size = (SPACE_SIZE - sizeof(Header)) / sizeof(Header);
		freep = &base;
	}

	check_list();

	for(p = freep->s.next; ; p = p->s.next) {
		if (p->s.size >= nunits) { /* big enough */
			// my code
			//spin_lock(&kernel_lock);
			// end of my code
			freep = p->s.prev;
			if (p->s.size == nunits) { /* exactly */
				(p->s.prev)->s.next = p->s.next;
				(p->s.next)->s.prev = p->s.prev;
			} else { /* allocate tail end */
				p->s.size -= nunits;
				p += p->s.size;
				p->s.size = nunits;
			}
			// my code
			spin_unlock(&kernel_lock);
			// end of my code			
			return (void *)(p + 1);
		}
		if (p == freep) { /* wrapped around free list */
			// my code
			spin_unlock(&kernel_lock);
			// end of my code
			return NULL;
		}
	}
	// my code
	spin_unlock(&kernel_lock);
	// end of my code
}

/* free: put block ap in free list */
void
test_free(void *ap)
{
	Header *bp, *p;

	// my code
	spin_lock(&kernel_lock);
	// end of my code

	bp = (Header *) ap - 1; /* point to block header */

	for (p = freep; !(bp > p && bp < p->s.next); p = p->s.next)
		if (p >= p->s.next && (bp > p || bp < p->s.next))
			break; /* freed block at start or end of arena */

	// my code
	//spin_lock(&kernel_lock);
	// end of my code

	if (bp + bp->s.size == p->s.next) { /* join to upper nbr */
		// my code
		//spin_lock(&kernel_lock);
		// end of my code

		bp->s.size += p->s.next->s.size;

		// my code
		//spin_unlock(&kernel_lock);
		// end of my code

		bp->s.next = p->s.next->s.next;
		bp->s.prev = p->s.next->s.prev;
	} else if (p + p->s.size == bp) { /* join to lower nbr */
		
		// my code
		//spin_lock(&kernel_lock);
		// end of my code
		
		p->s.size += bp->s.size;
		
		// my code
		//spin_unlock(&kernel_lock);
		// end of my code

	} else {
		bp->s.next = p->s.next;
		bp->s.prev = p;
		p->s.next->s.prev = bp;
		p->s.next = bp;
	}
	freep = p;

	// my code
	spin_unlock(&kernel_lock);
	// end of my code


	check_list();
}

