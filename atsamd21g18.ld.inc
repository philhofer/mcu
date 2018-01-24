MEMORY {
       RAM(rwx): ORIGIN = 0x20000000, LENGTH = 32K
       FLASH(rx): ORIGIN = 0x00000000, LENGTH = 256K
}

ENTRY(reset_entry)

SECTIONS {
	 vector_table :
	 {
		. = ALIGN(4);
		*(vector_table)
		. = ALIGN(4);
	 } >FLASH

	 .text :
	 {
		. = ALIGN(4);
		_stext = .;
		*(.text)
		*(.text*)
		*(.rodata)
		*(.rodata*)
		. = ALIGN(4);
		_etext = .;
	 } >FLASH

	 _sidata = .;

	 .data :
	 {
		. = @STACK_SIZE@;
		. = ALIGN(4);
		_sdata = .;
		*(.data)
		*(.data*)
		. = ALIGN(4);
		_edata = .;
	 } >RAM

	 .bss :
	 {
		. = ALIGN(4);
		_sbss = .;
		*(.bss)
		*(.bss*)
		. = ALIGN(4);
		_ebss = .;
	 } >RAM
}