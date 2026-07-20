#pragma once

#include <features.h>
#include <sgidefs.h>

/*
 * The MIPS libpthread sources (ported from glibc/uClibc) expect a glibc-style
 * __extern_always_inline to be available as the default expansion of their
 * PT_EI marker; musl doesn't define one, so supply a compatible fallback here.
 */
#ifndef __extern_always_inline
# define __extern_always_inline \
    extern __inline __attribute__ ((__always_inline__, __gnu_inline__))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Atomic test-and-set: store v into *p and return the value *p held before the
 * store. Unlike glibc/uClibc, musl has no _test_and_set libc symbol for the
 * ported MIPS pt-machine.h (testandset()) to call, so it is provided here,
 * always-inlined, to avoid introducing a new external symbol into the library.
 */
extern __inline __attribute__ ((__always_inline__, __gnu_inline__)) int
_test_and_set (int *p, int v)
{
  int old, tmp;

  __asm__ __volatile__
    ("/* _test_and_set */\n"
     "    .set  push\n"
#if _MIPS_SIM == _ABIO32 && __mips < 2
     "    .set  mips2\n"
#endif
     "    sync\n"
     "1:\n"
     "    ll    %0, %3\n"     /* old = *p (linked)           */
     "    move  %1, %4\n"     /* tmp = v                     */
     "    beq   %0, %4, 2f\n" /* already equal -> done       */
     "    sc    %1, %2\n"     /* *p = tmp (conditional)      */
     "    beqz  %1, 1b\n"     /* lost the reservation, retry */
     "    sync\n"
     "    .set  pop\n"
     "2:"
     : "=&r" (old), "=&r" (tmp), "=m" (*p)
     : "m" (*p), "r" (v)
     : "memory");

  return old;
}

#ifdef __cplusplus
}
#endif
