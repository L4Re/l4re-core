#include "pthread_impl.h"

#include <l4/sys/thread.h>

int
ptlc_set_tp(void *tls_tp)
{
  // On ARM the thread pointer lives in the user read-only TPIDRURO register,
  // which cannot be written from user mode. Ask the kernel to set it for the
  // current thread (L4_INVALID_CAP addresses the caller itself).
  l4_thread_arm_set_tpidruro(L4_INVALID_CAP, (l4_addr_t)tls_tp);
  return 0;
}
