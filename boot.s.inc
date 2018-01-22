	.cpu cortex-m0
	.arch armv6-m
	.thumb

	.pushsection vector_table
	.word @STACK_TOP@   /* sp on reset */
	.word reset0      /* 1: reset routine */
	.word nmi0        /* 2: NMI handler */
	.word hardfault0  /* 3: hard-fault handler */
	.word fatal0      /* 4: reserved */
	.word fatal0	  /* 5 */
	.word fatal0	  /* 6 */
	.word fatal0	  /* 7 */
	.word fatal0	  /* 8 */
	.word fatal0	  /* 9 */
	.word fatal0	  /* 10 */
	.word svcall0     /* 11: SVCall */
	.word fatal0	  /* 12 */
	.word fatal0	  /* 13 */
	.word pendsv0	  /* 14: PendSV */
	.word systick0	  /* 15: SysTick */
	.word irq0	  /* 16: irq */
	.word irq0	  /* 17 */
	.word irq0	  /* 18 */
	.word irq0	  /* 19 */
	.word irq0	  /* 20 */
	.word irq0	  /* 21 */
	.word irq0	  /* 22 */
	.word irq0	  /* 23 */
	.word irq0	  /* 24 */
	.word irq0	  /* 25 */
	.word irq0	  /* 26 */
	.word irq0	  /* 27 */
	.word irq0	  /* 28 */
	.word irq0	  /* 29 */
	.word irq0	  /* 30 */
	.word irq0	  /* 31 */
	.popsection

	.global reset_entry
	.global irq_entry
	.global systick_entry
	.global svcall_entry
	.global pendsv_entry
reset0:
	b reset_entry
hardfault0:
nmi0:
fatal0:
	b .	/* loop forever */
irq0:
	b irq_entry
	b eret
systick0:
	b systick_entry
	b eret
svcall0:
	b svcall_entry
	b eret
pendsv0:
	b pendsv_entry
	b eret
eret:
	mov  r0, #0
	sub  r0, r0, #7
	bx   r0         /* bx 0xfffffff9 = return to thread-mode */
	b    fatal0
