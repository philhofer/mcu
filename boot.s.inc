	.cpu cortex-m0
	.arch armv6-m
	.thumb

	.global reset_entry
	.global nmi_entry
	.global hardfault_entry
	.global irq_entry
	.global systick_entry
	.global svcall_entry
	.global pendsv_entry

	.pushsection vector_table
	.word @STACK_TOP@     /* sp on reset */
	.word reset_entry     /* 1: reset routine */
	.word nmi_entry       /* 2: NMI handler */
	.word hardfault_entry /* 3: hard-fault handler */
	.word fatal0          /* 4-10: reserved */
	.word fatal0	      /* 5 */
	.word fatal0	      /* 6 */
	.word fatal0	      /* 7 */
	.word fatal0	      /* 8 */
	.word fatal0	      /* 9 */
	.word fatal0	      /* 10 */
	.word svcall_entry    /* 11: SVCall */
	.word fatal0	      /* 12 */
	.word fatal0	      /* 13 */
	.word pendsv_entry    /* 14: PendSV */
	.word systick_entry   /* 15: SysTick */
	.word irq_entry	      /* 16: irq */
	.word irq_entry	      /* 17 */
	.word irq_entry	      /* 18 */
	.word irq_entry	      /* 19 */
	.word irq_entry	      /* 20 */
	.word irq_entry	      /* 21 */
	.word irq_entry	      /* 22 */
	.word irq_entry	      /* 23 */
	.word irq_entry	      /* 24 */
	.word irq_entry	      /* 25 */
	.word irq_entry	      /* 26 */
	.word irq_entry	      /* 27 */
	.word irq_entry	      /* 28 */
	.word irq_entry	      /* 29 */
	.word irq_entry	      /* 30 */
	.word irq_entry	      /* 31 */
	.popsection

hardfault0:
nmi0:
fatal0:
	b .	/* loop forever */
