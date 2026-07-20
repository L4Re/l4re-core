/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (C) 2021, 2025 Kernkonzept GmbH.
 * Author(s): Georg Kotheimer <georg.kotheimer@kernkonzept.com>
 */

#include <sys/asm.h>

.text
.global __pthread_new_thread_entry
__pthread_new_thread_entry:
  REG_L t0, 0(sp) /* entry point */
  REG_L ra, (1*SZREG)(sp) /* return address */
  REG_L a0, (2*SZREG)(sp) /* argument */

  /* align stack */
  andi sp, sp, ALMASK

.option push
.option norelax
  /* Load the main executable's global pointer, captured on the main thread in
     __pthread_initialize_minimal(). We must not use our own __global_pointer$
     here: as this code is part of libc.so, that would resolve to libc's gp,
     whereas non-PIC code in the main executable needs the executable's gp.
     lla is PC-relative and therefore works without a valid gp. */
  lla  t1, __pthread_l4_gp
  REG_L gp, 0(t1)
.option pop

  jr t0
