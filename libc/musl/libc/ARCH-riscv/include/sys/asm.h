#pragma once

/*
 * Handle the difference between standalone .S files, where __ASSEMBLER__ is
 * set and mnemonics are used bare, and inline asm strings inside C, where the
 * same mnemonics must come out as string literals instead.
 */
#ifdef __ASSEMBLER__
# define __RISCV_INSN(insn) insn
#else
# define __RISCV_INSN(insn) #insn
#endif

/*
 * General purpose register load/store macros that hide the ABI differences.
 * PTRLOG is the shift for scaling an index into an array of register-sized
 * (i.e. pointer-sized) slots.
 */
#if __riscv_xlen == 64
# define PTRLOG 3
# define SZREG  __RISCV_INSN(8)
# define SZMOD  __RISCV_INSN(d)
# define REG_S  __RISCV_INSN(sd)
# define REG_L  __RISCV_INSN(ld)
#elif __riscv_xlen == 32
# define PTRLOG 2
# define SZREG  __RISCV_INSN(4)
# define SZMOD  __RISCV_INSN(w)
# define REG_S  __RISCV_INSN(sw)
# define REG_L  __RISCV_INSN(lw)
#else
# error "__riscv_xlen must be 32 or 64"
#endif

/*
 * Floating point register load/store macros. Left undefined on soft-float
 * builds, where __riscv_flen is not set at all.
 */
#ifdef __riscv_flen
# if __riscv_flen == 32
#  define FREG_L __RISCV_INSN(flw)
#  define FREG_S __RISCV_INSN(fsw)
#  define SZFREG __RISCV_INSN(4)
# elif __riscv_flen == 64
#  define FREG_L __RISCV_INSN(fld)
#  define FREG_S __RISCV_INSN(fsd)
#  define SZFREG __RISCV_INSN(8)
# else
#  error "unsupported __riscv_flen"
# endif
#endif

/* Function entry/exit bookkeeping for the assembler and debugger. */
#define ENTRY(symbol)        \
  .globl  symbol;            \
  .align  2;                 \
  .type   symbol, @function; \
symbol:

#define END(symbol) \
  .size symbol, . - symbol

/* Required stack alignment (16 bytes), as a mask. */
#define ALMASK ~15
