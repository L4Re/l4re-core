#pragma once

#include <sgidefs.h>

/* Token-paste helper, split in two so operands that are themselves macros
   get expanded before pasting. */
#ifndef CAT
# define __CAT(a, b) a##b
# define CAT(a, b) __CAT(a, b)
#endif

/*
 * Pointer-sized assembler directive/size, for tables of code or data
 * addresses. o32 and n32 both use 32 bit pointers; n64 uses 64 bit ones.
 */
#if _MIPS_SIM == _MIPS_SIM_ABI64
# define PTR      .dword
# define PTRSIZE  8
# define PTRLOG   3
#else /* _MIPS_SIM_ABI32 or _MIPS_SIM_NABI32 */
# define PTR      .word
# define PTRSIZE  4
# define PTRLOG   2
#endif

/*
 * Establishing $gp (the global pointer, used to reach the GOT) differs
 * between o32, which computes it explicitly with .cpload/.cprestore, and
 * n32/n64, which pass it in via the callee-saved-gp convention and use
 * .cpsetup/.cpreturn around calls that clobber it.
 */
#if _MIPS_SIM == _MIPS_SIM_ABI32

# ifdef __PIC__
#  define CPLOAD(reg)     .cpload reg
#  define CPRESTORE(off)  .cprestore off
# else
#  define CPLOAD(reg)
#  define CPRESTORE(off)
# endif
# define CPADD(reg)  .cpadd reg

/* Compute $gp from $t9 (the function's own address) at entry. */
# define SETUP_GP     \
  .set    noreorder;  \
  .cpload $25;        \
  .set    reorder

/*
 * As above, but usable anywhere in a function, not just as the very first
 * instruction: find our own address via a "bal" to the next line first.
 */
# define SETUP_GPX(r)     \
    .set    noreorder;    \
    move    r, $31;       \
    bal     42f;          \
    nop;                  \
42: .cpload $31;          \
    move    $31, r;       \
    .set    reorder

# define SETUP_GPX_L(r, l)    \
    .set    noreorder;        \
    move    r, $31;           \
    bal     l;                \
    nop;                      \
l:  .cpload $31;              \
    move    $31, r;           \
    .set    reorder

# define SAVE_GP(off)  .cprestore off

# define SETUP_GP64(gpoff, proc)
# define SETUP_GPX64(cpreg, rasave)
# define SETUP_GPX64_L(cpreg, rasave, l)
# define RESTORE_GP64
# define USE_ALT_CP(reg)

# define L(label)  $L##label

#else /* n32 / n64: gp is callee-saved, no explicit load needed on entry */

# define SETUP_GP
# define SETUP_GPX(r)
# define SETUP_GPX_L(r, l)
# define SAVE_GP(off)

# define SETUP_GP64(gpoff, proc)  .cpsetup $25, gpoff, proc

# define SETUP_GPX64(cpreg, rasave) \
    move      rasave, $31;          \
    .set      noreorder;            \
    bal       42f;                  \
    nop;                            \
42: .set      reorder;              \
    .cpsetup  $31, cpreg, 42b;      \
    move      $31, rasave

# define SETUP_GPX64_L(cpreg, rasave, l)  \
    move      rasave, $31;                \
    .set      noreorder;                  \
    bal       l;                          \
    nop;                                  \
l:  .set      reorder;                    \
    .cpsetup  $31, cpreg, l;              \
    move      $31, rasave

# define RESTORE_GP64  .cpreturn

# define USE_ALT_CP(reg)  .cplocal reg

# define L(label)  .L##label

#endif /* _MIPS_SIM */

/*
 * Bytes the o32 ABI reserves on the caller's stack for the first four argument
 * registers, in case the callee needs to spill them; n32/n64 place no such
 * obligation on the caller.
 */
#if _MIPS_SIM == _MIPS_SIM_ABI32
# define NARGSAVE  4
#else
# define NARGSAVE  0
#endif

/* Function entry/exit bookkeeping for the assembler and debugger. */
#define LEAF(name)              \
      .globl  name;             \
      .align  2;                \
      .type   name, @function;  \
      .ent    name, 0;          \
name: .frame  sp, 0, ra

#define NESTED(name, framesize, rpc)  \
      .globl  name;                   \
      .align  2;                      \
      .type   name, @function;        \
      .ent    name, 0;                \
name: .frame  sp, framesize, rpc

#ifndef END
# define END(name)  .end name; .size name, .-name
#endif

#define EXPORT(name)  .globl name; name:

#define ABS(name, value)  .globl name; name = value

/*
 * "pref"/"prefx" only exist from MIPS IV onward; turn them into no-ops
 * everywhere else so callers don't need their own #ifdef. Must only be used
 * with .set noreorder in effect.
 */
#if _MIPS_ISA == _MIPS_ISA_MIPS4 || _MIPS_ISA == _MIPS_ISA_MIPS5 \
    || _MIPS_ISA == _MIPS_ISA_MIPS32 || _MIPS_ISA == _MIPS_ISA_MIPS64
# define PREF(hint, addr)   pref hint, addr
# define PREFX(hint, addr)  prefx hint, addr
#else
# define PREF(hint, addr)
# define PREFX(hint, addr)
#endif

/*
 * Conditional move. Real movn/movz instructions appeared with MIPS IV;
 * older ISAs emulate them with a branch around a plain move.
 */
#if _MIPS_ISA == _MIPS_ISA_MIPS1

# define MOVN(rd, rs, rt) \
    .set  push;           \
    .set  reorder;        \
    beqz  rt, 43f;        \
    move  rd, rs;         \
    .set  pop;            \
43:

# define MOVZ(rd, rs, rt) \
    .set  push;           \
    .set  reorder;        \
    bnez  rt, 43f;        \
    move  rd, rt;         \
    .set  pop;            \
43:

#elif _MIPS_ISA == _MIPS_ISA_MIPS2 || _MIPS_ISA == _MIPS_ISA_MIPS3

# define MOVN(rd, rs, rt) \
    .set  push;           \
    .set  noreorder;      \
    bnezl rt, 43f;        \
    move  rd, rs;         \
    .set  pop;            \
43:

# define MOVZ(rd, rs, rt) \
    .set  push;           \
    .set  noreorder;      \
    beqzl rt, 43f;        \
    movz  rd, rs;         \
    .set  pop;            \
43:

#else /* MIPS4, MIPS5, MIPS32, MIPS64 */
# define MOVN(rd, rs, rt)  movn rd, rs, rt
# define MOVZ(rd, rs, rt)  movz rd, rs, rt
#endif

/*
 * Required stack alignment, in bytes minus one / as a mask: 8-byte aligned for
 * o32, 16-byte aligned for n32/n64.
 */
#if _MIPS_SIM == _MIPS_SIM_ABI64 || _MIPS_SIM == _MIPS_SIM_NABI32
# define ALSZ    15
# define ALMASK  ~15
#else
# define ALSZ    7
# define ALMASK  ~7
#endif

/*
 * Width of a general purpose register in this ABI, and the matching plain
 * load/store mnemonics.
 */
#if _MIPS_SIM == _MIPS_SIM_ABI64 || _MIPS_SIM == _MIPS_SIM_NABI32
# define SZREG  8
# define REG_S  sd
# define REG_L  ld
#else
# define SZREG  4
# define REG_S  sw
# define REG_L  lw
#endif

/* Arithmetic/load/store mnemonics sized to the C "int" type. */
#if _MIPS_SZINT == 64
# define INT_ADD    dadd
# define INT_ADDI   daddi
# define INT_ADDU   daddu
# define INT_ADDIU  daddiu
# define INT_SUB    dsub
# define INT_SUBI   dsubi
# define INT_SUBU   dsubu
# define INT_SUBIU  dsubu
# define INT_L      ld
# define INT_S      sd
#else /* _MIPS_SZINT == 32 */
# define INT_ADD    add
# define INT_ADDI   addi
# define INT_ADDU   addu
# define INT_ADDIU  addiu
# define INT_SUB    sub
# define INT_SUBI   subi
# define INT_SUBU   subu
# define INT_SUBIU  subu
# define INT_L      lw
# define INT_S      sw
#endif

/* Same, sized to the C "long" type, plus shifts. */
#if _MIPS_SZLONG == 64
# define LONG_ADD    dadd
# define LONG_ADDI   daddi
# define LONG_ADDU   daddu
# define LONG_ADDIU  daddiu
# define LONG_SUB    dsub
# define LONG_SUBI   dsubi
# define LONG_SUBU   dsubu
# define LONG_SUBIU  dsubu
# define LONG_L      ld
# define LONG_S      sd
# define LONG_SLL    dsll
# define LONG_SLLV   dsllv
# define LONG_SRL    dsrl
# define LONG_SRLV   dsrlv
# define LONG_SRA    dsra
# define LONG_SRAV   dsrav
#else /* _MIPS_SZLONG == 32 */
# define LONG_ADD    add
# define LONG_ADDI   addi
# define LONG_ADDU   addu
# define LONG_ADDIU  addiu
# define LONG_SUB    sub
# define LONG_SUBI   subi
# define LONG_SUBU   subu
# define LONG_SUBIU  subu
# define LONG_L      lw
# define LONG_S      sw
# define LONG_SLL    sll
# define LONG_SLLV   sllv
# define LONG_SRL    srl
# define LONG_SRLV   srlv
# define LONG_SRA    sra
# define LONG_SRAV   srav
#endif

/*
 * Same, sized to a C pointer: o32 pointers are always 32 bit; n32 pointers
 * are 32 bit values but n64 (and a hypothetical 64 bit o32) are 64 bit. The
 * n32 ABI additionally special-cases pre-Release-6 cores, which lack a
 * couple of the "u" (unsigned/no-trap) opcode variants used below.
 */
#if _MIPS_SIM == _MIPS_SIM_NABI32
# define PTR_ADD        add
# define PTR_ADDI       addi
# define PTR_SUB        sub
# define PTR_SUBI       subi
# if !defined __mips_isa_rev || __mips_isa_rev < 6
#  define PTR_ADDU      add
#  define PTR_ADDIU     addi
#  define PTR_SUBU      sub
#  define PTR_SUBIU     sub
# else
#  define PTR_ADDU      addu
#  define PTR_ADDIU     addiu
#  define PTR_SUBU      subu
#  define PTR_SUBIU     subu
# endif
# define PTR_L          lw
# define PTR_LA         la
# define PTR_S          sw
# define PTR_SLL        sll
# define PTR_SLLV       sllv
# define PTR_SRL        srl
# define PTR_SRLV       srlv
# define PTR_SRA        sra
# define PTR_SRAV       srav
# define PTR_SCALESHIFT 2
#elif _MIPS_SIM == _MIPS_SIM_ABI64 \
      || (_MIPS_SIM == _MIPS_SIM_ABI32 && _MIPS_SZPTR == 64)
# define PTR_ADD        dadd
# define PTR_ADDI       daddi
# define PTR_ADDU       daddu
# define PTR_ADDIU      daddiu
# define PTR_SUB        dsub
# define PTR_SUBI       dsubi
# define PTR_SUBU       dsubu
# define PTR_SUBIU      dsubu
# define PTR_L          ld
# define PTR_LA         dla
# define PTR_S          sd
# define PTR_SLL        dsll
# define PTR_SLLV       dsllv
# define PTR_SRL        dsrl
# define PTR_SRLV       dsrlv
# define PTR_SRA        dsra
# define PTR_SRAV       dsrav
# define PTR_SCALESHIFT 3
#else /* o32, 32 bit pointers */
# define PTR_ADD        add
# define PTR_ADDI       addi
# define PTR_ADDU       addu
# define PTR_ADDIU      addiu
# define PTR_SUB        sub
# define PTR_SUBI       subi
# define PTR_SUBU       subu
# define PTR_SUBIU      subu
# define PTR_L          lw
# define PTR_LA         la
# define PTR_S          sw
# define PTR_SLL        sll
# define PTR_SLLV       sllv
# define PTR_SRL        srl
# define PTR_SRLV       srlv
# define PTR_SRA        sra
# define PTR_SRAV       srav
# define PTR_SCALESHIFT 2
#endif

/*
 * CP0 accessors: the upper half of some CP0 registers only became directly
 * addressable with the 64 bit ("d") move instructions from MIPS III on.
 */
#if    _MIPS_ISA == _MIPS_ISA_MIPS3 || _MIPS_ISA == _MIPS_ISA_MIPS4 \
    || _MIPS_ISA == _MIPS_ISA_MIPS5 || _MIPS_ISA == _MIPS_ISA_MIPS64
# define MFC0  dmfc0
# define MTC0  dmtc0
#else
# define MFC0  mfc0
# define MTC0  mtc0
#endif

/*
 * MIPS has a weak memory model in general; a few implementations promise
 * stronger ordering, but code that needs to be portable across cores must
 * still issue an explicit SYNC. Very early (true MIPS I) cores lack the
 * instruction entirely, hence the override hook below.
 */
#ifndef MIPS_SYNC
# define MIPS_SYNC  sync
#endif
