#include "libc-api.h"
#include "pthread_impl.h"

#include <stdint.h>

#include <l4/sys/segment.h>
#include <l4/sys/utcb.h>

/* x86 GDT/LDT segment descriptor. See libpthread sysdeps/i386/tls.h. */
struct x86_descr {
    union {
        struct {
            unsigned int a;
            unsigned int b;
        };
        struct {
            uint16_t limit0;
            uint16_t base0;
            unsigned base1: 8, type: 4, s: 1, dpl: 2, p: 1;
            unsigned limit: 4, avl: 1, l: 1, d: 1, g: 1, base2: 8;
        };
    };
} __attribute__((packed));

int
ptlc_set_tp(void *tls_tp)
{
  __pthread_descr_libc_data(ptlc_tls_tp_to_thread_descr(tls_tp))->self_tp = tls_tp;

  // On x86 the thread pointer is accessed through the %gs segment register.
  // Set up a GDT segment whose base points at the TLS block (so that %gs:0
  // reads self_tp) and load the returned selector into %gs. The kernel picks
  // the entry and returns the segment selector to use.
  struct x86_descr segdesc;
  segdesc.limit0 = 0xffff;
  segdesc.base0  = ((unsigned long)tls_tp) & 0x0ffff;
  segdesc.base1  = (((unsigned long)tls_tp) & 0x0ff0000) >> 16;
  segdesc.type   = 2;
  segdesc.s      = 1;
  segdesc.dpl    = 3;
  segdesc.p      = 1;
  segdesc.limit  = 0xf;
  segdesc.avl    = 1;
  segdesc.d      = 1; // 32bit
  segdesc.g      = 1; // pages
  segdesc.base2  = (((unsigned long)tls_tp) & 0xff000000) >> 24;
  segdesc.l      = 0;

  long seg = fiasco_gdt_set(L4_INVALID_CAP, &segdesc, 8, 0, l4_utcb());
  if (seg < 0)
    return -1;

  __asm__ __volatile__("movw %w0, %%gs" :: "q" ((int)seg));

  return 0;
}
