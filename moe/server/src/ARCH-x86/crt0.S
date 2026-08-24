/**
 * \file
 * \brief	Startup code
 *
 * \date	06/03/2003
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/*
 * (c) 2003-2009 Author(s)
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#define __ASSEMBLY__
#include <l4/sys/compiler.h>

/*--- .text (program code) -------------------------*/
	.text

/*
 * Provide the following stack layout to the libc's startup code:
 *
 *	0x00	0	(argc)
 *	0x04	NULL	(argv final NULL)
 *	0x08	NULL	(env final NULL)
 *			auxv[]
 *	0x0c	3		AT_PHDR
 *	0x10	?		<ptr to PHDRs>
 *	0x14	5		AT_PHNUM
 *	0x18	?		<number of PHDRs>
 *	0x1c	4		AT_PHENT
 *	0x20	32		<size of a PHDR entry>
 *	0x24	NULL		(auxv termination)
 *
 * This is required for C++ exceptions to work in relocated (PIE) executables
 * and with libunwind (LLVM)!
 *
 * ATTENTION: the following code assumes that the executable starts directly
 * with the ELF header! For regularly loaded executables the aux vector is
 * provided by libloader, which can work with any elf file.
 */
	.globl	_real_start
_real_start:
	/* Clear the base pointer so that stack backtraces will work.  */
	xorl	%ebp,%ebp
	/* Mind the argc/argv/env/auxv that needs to be on the stack! */
	mov 	$_stack_top-40, %esp

	mov $__executable_start,%eax

	movl	$3, 0x0c(%esp)		// AT_PHDR
	movl	0x1c(%eax), %edx	// e_phoff
	addl	%eax, %edx
	movl	%edx, 0x10(%esp)

	movl	$5, 0x14(%esp)		// AT_PHNUM
	movzwl	0x2c(%eax), %edx	// e_phnum
	movl	%edx, 0x18(%esp)

	movl	$4, 0x1c(%esp)		// AT_PHENT
	movl	$32, 0x20(%esp)		// sizeof(Elf32_Phdr)

	jmp _start
/*--- my stack -------------------------------------*/
	.bss
	.balign	16
_stack_bottom:
	.space 0x4000
_stack_top:

