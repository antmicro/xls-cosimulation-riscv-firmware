OUTPUT_FORMAT("elf32-littleriscv")
ENTRY(_start)

MEMORY {
	rom : ORIGIN = 0x00000000, LENGTH = 0x00008000
	sram : ORIGIN = 0x10000000, LENGTH = 0x00008000
	main_ram : ORIGIN = 0x40000000, LENGTH = 0x10000000
}

SECTIONS
{
	.text :
	{
		_ftext = .;
		*(.start)
		*(.text .stub .text.* .gnu.linkonce.t.*)
		_etext = .;
	} > main_ram 

	.rodata :
	{
		. = ALIGN(4);
		_frodata = .;
		*(.rodata .rodata.* .gnu.linkonce.r.*)
		*(.rodata1)
		_erodata = .;
	} > main_ram

	.data :
	{
		. = ALIGN(4);
		_fdata = .;
		__global_pointer$ = .;
		*(.data .data.* .gnu.linkonce.d.*)
		*(.data1)
		*(.sdata .sdata.* .gnu.linkonce.s.*)
		_edata = .;
	} > main_ram

	.bss :
	{
		. = ALIGN(4);
		_fbss = .;
		__bss_start = .;
		*(.dynsbss)
		*(.sbss .sbss.* .gnu.linkonce.sb.*)
		*(.scommon)
		*(.dynbss)
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
		_end = .;
	} > main_ram

	.heap :
	{
		. = ALIGN(8);
		__heap_start = .;
		. += 0x100000;
		. = ALIGN(8);
		__heap_end = .;
	} > main_ram

	/DISCARD/ :
	{
		*(.eh_frame)
		*(.comment)
	}
}

PROVIDE(_fstack = ORIGIN(main_ram) + LENGTH(main_ram) - 8);
