MEMORY
{
	ASSERT_EMPTY(rwx)  : ORIGIN = 0x00000000, LENGTH = 0

	IOVECTORS(rw) : ORIGIN = 0x06007ff0, LENGTH = 0x00000010
	COM_RAM(rwx) : ORIGIN = 0x06100000, LENGTH = 0x00200000
}


GROUP(libgcc.a libc.a libm.a libnosys.a)


ENTRY(start)

SECTIONS
{
	.assert_empty (NOLOAD) :
	{
		*(.data*)
		*(.glue*)
		*(.vfp11_veneer)
		*(.v4_bx)
		*(.igot.plt)
		*(.iplt)
		*(.rel.iplt)
	} >ASSERT_EMPTY


	.iovectors ORIGIN(IOVECTORS) :
	{
		PROVIDE ( tSerialVectors = . );
		. = . + 0x10;
	} >IOVECTORS


	.code ORIGIN(COM_RAM) : {
		load_address = . ;

		KEEP(*(.header))
		*(.init_code .text* .rodata*)

		. = ALIGN(0x10);
	} > COM_RAM = 0xff


	.parameter (NOLOAD) :
	{
		parameter_start_address = . ;
		KEEP(*(.parameter))
		parameter_end_address = . ;
	} >COM_RAM


	.bss : ALIGN(4)
	{
		__bss_start__ = .;
		*(.bss* COMMON)
		. = ALIGN(4);
		__bss_end__ = .;
	} > COM_RAM


	.stack :
	{
		. = ORIGIN(COM_RAM) + LENGTH(COM_RAM);
		stack_top = . ;
	} >COM_RAM
}
