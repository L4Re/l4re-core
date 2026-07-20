/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include <errno.h>
#include <inttypes.h>
#include <time.h>
#include <l4/re/env.h>
#include <l4/sys/kip.h>
#include <l4/libc_backends/clk.h>
#include <l4/sys/compiler.h>

#include "clocks.h"

typedef int Get_clock(struct timespec *);
uint64_t __attribute__((weak)) __libc_l4_rt_clock_offset;

int __attribute__((weak))
libc_backend_rt_clock_gettime(struct timespec *tp)
{
  uint64_t clock;

  clock = l4_kip_clock(l4re_kip());
  clock += __libc_l4_rt_clock_offset;

  tp->tv_sec  = clock / 1000000;
  tp->tv_nsec = (clock % 1000000) * 1000;

  return 0;
}

static int mono_clock_gettime(struct timespec *tp)
{
  uint64_t clock;
  clock = l4_kip_clock(l4re_kip());
  tp->tv_sec = clock / 1000000;
  tp->tv_nsec = (clock % 1000000) * 1000;

  return 0;
}

Get_clock *__libc_l4_gettime[NCLOCKS] =
{
  [CLOCK_REALTIME]         = libc_backend_rt_clock_gettime,
  [CLOCK_REALTIME_COARSE]  = libc_backend_rt_clock_gettime,
  [CLOCK_MONOTONIC]        = mono_clock_gettime,
  [CLOCK_MONOTONIC_RAW]    = mono_clock_gettime,
  [CLOCK_MONOTONIC_COARSE] = mono_clock_gettime,
};

// musl's internal code references __clock_gettime directly, while the public
// clock_gettime symbol may be redirected to a time64 variant
// (__clock_gettime64) by <time.h> on 32-bit architectures (_REDIR_TIME64).
// Define the implementation under the non-redirected __clock_gettime name and
// alias the (possibly redirected) public clock_gettime name to it, so both the
// internal and public entry points resolve to our implementation.
int __clock_gettime(clockid_t clk_id, struct timespec *tp);

int __clock_gettime(clockid_t clk_id, struct timespec *tp)
{
  if (clk_id < 0 || clk_id >= NCLOCKS || __libc_l4_gettime[clk_id] == NULL)
    {
      errno = ENODEV;
      return -1;
    }

  return __libc_l4_gettime[clk_id](tp);
}

L4_BEGIN_DECLS
L4_STRONG_ALIAS(__clock_gettime, clock_gettime);
L4_END_DECLS
