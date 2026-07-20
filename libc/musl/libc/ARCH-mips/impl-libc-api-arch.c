#include "pthread_impl.h"

#include <l4/sys/thread_mips.h>
#include <l4/sys/utcb.h>
#include <stddef.h>

int
ptlc_set_tp(void *tls_tp)
{
  // On MIPS the thread pointer is read from the UserLocal hardware register
  // (rdhwr $29) and, per the MIPS TLS ABI, points TP_OFFSET (0x7000) above the
  // TLS block.
  //
  // The L4 UTCB pointer is not kept in a dedicated register on MIPS; instead
  // l4_utcb_direct() reads it from the TCB area just below the thread pointer,
  // at *(ULR - 0x7000 - 3*sizeof(void*)). As we are about to repoint the ULR
  // to our own TLS block, we must copy the UTCB pointer (still reachable via
  // the current ULR) to that location first, otherwise l4_utcb() would read
  // uninitialized memory once the new ULR is in effect.
  _Static_assert(offsetof(pthread_libc_data_t, __l4_utcb)
                   == sizeof(pthread_libc_data_t) - 3 * sizeof(void*),
                 "l4_utcb_direct directly reads from *(tp - 0x7000 - 3*sizeof(void*))");

  void *utcb = l4_utcb();
  ((pthread_libc_data_t *)tls_tp)[-1].__l4_utcb = utcb;

  // The ULR is not writable from user mode, so ask the kernel to set it for
  // the current thread (L4_INVALID_CAP addresses the caller itself).
  l4_thread_mips_set_ulr(L4_INVALID_CAP, (l4_umword_t)tls_tp + TP_OFFSET);
  return 0;
}
