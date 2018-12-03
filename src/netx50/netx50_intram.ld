/*---------------------------------------------------------------------------
  Author : Christoph Thelen

           Hilscher GmbH, Copyright (c) 2009, All Rights Reserved

           Redistribution or unauthorized use without expressed written 
           agreement from the Hilscher GmbH is forbidden
---------------------------------------------------------------------------*/

ENTRY(start)

MEMORY
{
	ASSERT_EMPTY(rwx)	: ORIGIN = 0x00000000, LENGTH = 0

	DTCM(rw)		: ORIGIN = 0x04000000, LENGTH = 0x00001000
	INTRAM(rwx)		: ORIGIN = 0x08004000, LENGTH = 0x0000c000
	BUFFER(rwx)		: ORIGIN = 0x08010000, LENGTH = 0x00008000
}


/* Section Definitions */
SECTIONS
{
	.assert_empty (NOLOAD) :
	{
		/* NOTE: include all sections here whioch should be empty.
		 */
		*(.data)
		*(.glue*)
		*(.vfp11_veneer)
		*(.v4_bx)
	} >ASSERT_EMPTY


	.serialvectors 0x04000ff0 :
	{
		PROVIDE ( tSerialVectors = . );
		. = . + 0x10;
	} >DTCM


	.code ORIGIN(INTRAM) :
	{
		load_address = . ;

		KEEP(*(.header))
		*(.init_code .text* .rodata*)

		FILL(0xffffffff)
		. = ALIGN(0x10);
	} >INTRAM


	.parameter :
	{
		parameter_start_address = . ;
		KEEP(*(.parameter))
		parameter_end_address = . ;
	} >INTRAM


	.bss ALIGN(0x10) :
	{
		PROVIDE ( __bss_start__ = . );
		*(.bss COMMON)
		PROVIDE ( __bss_end__ = . );
	} >INTRAM


	/* Set the top of the stack to the end of INTRAM. */
	.stack :
	{
		/* Reserve at least 4096 bytes. */
		. = . + 0x1000;
		. = ORIGIN(INTRAM) + LENGTH(INTRAM) ;
		stack_top = . ;
	} >INTRAM


	/* IMPORTANT: Keep the buffer in one single INTRAM block.
	 *
	 * The buffer is used for the normal SPI FIFO mode and the XC SPI
	 * code. The latter can only copy data into one single INTRAM block
	 * per request. If a request crosses an INTRAM border, it does not
	 * continue in the next INTRAM block. Instead it wraps back to the
	 * start of the INTRAM block where the request started.
	 */
	.buffer ORIGIN(BUFFER) :
	{
		pcBufferStart = . ;
		pucBufferStart = . ;
		pulBufferStart = . ;
		. = ORIGIN(BUFFER) + LENGTH(BUFFER) ;
		pcBufferEnd = . ;
		pucBufferEnd = . ;
		pulBufferEnd = . ;
	} >BUFFER


	/* Stabs debugging sections.  */
	.stab          0 : { *(.stab) }
	.stabstr       0 : { *(.stabstr) }
	.stab.excl     0 : { *(.stab.excl) }
	.stab.exclstr  0 : { *(.stab.exclstr) }
	.stab.index    0 : { *(.stab.index) }
	.stab.indexstr 0 : { *(.stab.indexstr) }
	.comment       0 : { *(.comment) }
	/* DWARF debug sections.
		Symbols in the DWARF debugging sections are relative to the beginning
		of the section so we begin them at 0.  */
	/* DWARF 1 */
	.debug          0 : { *(.debug) }
	.line           0 : { *(.line) }
	/* GNU DWARF 1 extensions */
	.debug_srcinfo  0 : { *(.debug_srcinfo) }
	.debug_sfnames  0 : { *(.debug_sfnames) }
	/* DWARF 1.1 and DWARF 2 */
	.debug_aranges  0 : { *(.debug_aranges) }
	.debug_pubnames 0 : { *(.debug_pubnames) }
	/* DWARF 2 */
	.debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
	.debug_abbrev   0 : { *(.debug_abbrev) }
	.debug_line     0 : { *(.debug_line) }
	.debug_frame    0 : { *(.debug_frame) }
	.debug_str      0 : { *(.debug_str) }
	.debug_loc      0 : { *(.debug_loc) }
	.debug_macinfo  0 : { *(.debug_macinfo) }
	/* SGI/MIPS DWARF 2 extensions */
	.debug_weaknames 0 : { *(.debug_weaknames) }
	.debug_funcnames 0 : { *(.debug_funcnames) }
	.debug_typenames 0 : { *(.debug_typenames) }
	.debug_varnames  0 : { *(.debug_varnames) }
}

