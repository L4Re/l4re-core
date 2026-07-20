#pragma once

#include <sgidefs.h>

/*
 * Assembler-level names for the MIPS general purpose registers, as used by the
 * o32 and n32/n64 calling conventions. $0 (zero) and $1 (AT, reserved for the
 * assembler) as well as the register layout for $16..$31 are shared between
 * both conventions; the argument/temporary registers $4..$15 differ.
 */

#define zero  $0   /* hardwired zero */
#define AT    $1   /* reserved for the assembler ("AT" == assembler temp) */

#define v0    $2   /* function results */
#define v1    $3

#define a0    $4   /* first four (o32) or eight (n32/n64) call arguments */
#define a1    $5
#define a2    $6
#define a3    $7

#if _MIPS_SIM == _MIPS_SIM_ABI32

#define t0    $8   /* caller-saved temporaries */
#define t1    $9
#define t2    $10
#define t3    $11
#define t4    $12
#define t5    $13
#define t6    $14
#define t7    $15

#define ta0   t4   /* o32 has no a4..a7; varargs spill into t4..t7 instead */
#define ta1   t5
#define ta2   t6
#define ta3   t7

#else /* n32 or n64 */

#define a4    $8   /* remaining four call arguments */
#define a5    $9
#define a6    $10
#define a7    $11

#define t0    $12  /* caller-saved temporaries */
#define t1    $13
#define t2    $14
#define t3    $15

#define ta0   a4
#define ta1   a5
#define ta2   a6
#define ta3   a7

#endif /* _MIPS_SIM == _MIPS_SIM_ABI32 */

#define s0    $16  /* callee-saved */
#define s1    $17
#define s2    $18
#define s3    $19
#define s4    $20
#define s5    $21
#define s6    $22
#define s7    $23

#define t8    $24  /* caller-saved temporaries */
#define t9    $25
#define jp    $25  /* indirect jump target register for PIC calls */

#define k0    $26  /* reserved for kernel/exception use */
#define k1    $27

#define gp    $28  /* global pointer */
#define sp    $29  /* stack pointer */
#define fp    $30  /* frame pointer */
#define s8    $30  /* $30 is fp when a frame pointer is used, else a 9th
                      callee-saved register */
#define ra    $31  /* return address */
