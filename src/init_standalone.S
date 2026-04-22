	.section .init_code, "ax"

	.extern main_standalone
	.extern stack_top

	.global start

#include "asic_types.h"

@--------------------------------------

@ The Cortex-M4 in the netX90 is a THUMB-only CPU.
@ All others start in 32 bit mode.
#if ASIC_TYP==ASIC_TYP_NETX90_MPW || ASIC_TYP==ASIC_TYP_NETX90
	.syntax unified
	.thumb
#else
	.arm
#endif


    .func start, start
    .type start, STT_FUNC
start:
@----------------------------------------------------------------------------
@
@ Save the registers and switch to the new stack.
@
	@ Save the return address.
	push    {lr}

	@ Save the old stack position.
	mov     r3, sp

	@ Set own stack position.
	ldr     r2, =stack_top
	mov     sp, r2

	@ Store the old stack position on the new stack.
	push    {r3}


@----------------------------------------------------------------------------
@
@ Clear the .bss section (zero init).
@
	mov     r1, #0
	ldr     r2, =__bss_start__
	ldr     r3, =__bss_end__
clear_bss:
	str     r1, [r2], #4
	cmp     r2, r3
	bmi     clear_bss


@----------------------------------------------------------------------------
@
@ Call the main routine.
@
	@ Call the main routine. It requires the pointer to the parameter block in r0.
	bl      main_standalone

	@ now the return value is in r0.


@----------------------------------------------------------------------------
@
@ Restore the registers and switch to the old stack.
@
	@ Restore the old stack position.
	pop     {r1}
	mov     sp, r1

	pop     {lr}
    bx      lr

    .endfunc

@--------------------------------------

  .end

