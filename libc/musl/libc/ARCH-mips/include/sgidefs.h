#pragma once

/*
 * Symbolic names for the ISA revision levels. These are used by the _MIPS_ISA
 * GCC predefine for the corresponding -march/-mips selection.
 */
#define _MIPS_ISA_MIPS1   1
#define _MIPS_ISA_MIPS2   2
#define _MIPS_ISA_MIPS3   3
#define _MIPS_ISA_MIPS4   4
#define _MIPS_ISA_MIPS5   5
#define _MIPS_ISA_MIPS32  6
#define _MIPS_ISA_MIPS64  7

/*
 * Symbolic names for the calling convention (o32 / n32 / n64). GCC already
 * predefines _ABIO32, _ABIN32 and _ABI64 with these very values and sets
 * _MIPS_SIM to whichever of them matches the selected ABI; the #ifndef
 * guards below only cover toolchains that, for whatever reason, don't.
 */
#ifndef _ABIO32
# define _ABIO32  1
#endif
#define _MIPS_SIM_ABI32  _ABIO32

#ifndef _ABIN32
# define _ABIN32  2
#endif
#define _MIPS_SIM_NABI32  _ABIN32

#ifndef _ABI64
# define _ABI64  3
#endif
#define _MIPS_SIM_ABI64  _ABI64
