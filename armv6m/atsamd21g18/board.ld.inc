MEMORY {
       RAM(rwx): ORIGIN = 0x20000000, LENGTH = 32K
       FLASH(rx): ORIGIN = 0x00000000, LENGTH = 256K
}

ENTRY(reset_entry)

SECTIONS {
	 .text : AT(0)
	 {
		*(.vector_table)
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

	 .data : AT(_sidata)
	 {
		. = ALIGN(4);
		_sdata = .;
		*(.data)
		*(.data*)
		. = ALIGN(4);
		_edata = .;
	 } >RAM

	 . = ALIGN(4);
	 .bss :
	 {
		_sbss = .;
		*(.bss)
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	 } >RAM
}