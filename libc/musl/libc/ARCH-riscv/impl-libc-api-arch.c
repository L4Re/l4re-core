#include "pthread_impl.h"

#include <l4/sys/utcb.h>
#include <stddef.h>

int
ptlc_set_tp(void *tls_tp)
{
  // On RISC-V the thread pointer is held in the tp register (x4), which is
  // freely writable from user mode. Per the RISC-V TLS ABI the thread pointer
  // points TP_OFFSET (0 for the static TLS model) above the TLS block.
  //
  // The L4 UTCB pointer is not kept in a dedicated register on RISC-V; instead
  // l4_utcb_direct() reads it from the TCB area just below the thread pointer,
  // at *(tp - 3*sizeof(void*)). As we are about to repoint tp to our own TLS
  // block, we must copy the UTCB pointer (still reachable via the current tp)
  // to that location first, otherwise l4_utcb() would read uninitialized memory
  // once the new tp is in effect.
  _Static_assert(offsetof(pthread_libc_data_t, __l4_utcb)
                   == sizeof(pthread_libc_data_t) - 3 * sizeof(void*),
                 "l4_utcb_direct directly reads from *(tp - 3*sizeof(void*))");

  void *utcb = l4_utcb();
  ((pthread_libc_data_t *)tls_tp)[-1].__l4_utcb = utcb;

  void *tp = (char *)tls_tp + TP_OFFSET;
  __asm__ __volatile__("mv tp, %0" : : "r"(tp));
  return 0;
}
