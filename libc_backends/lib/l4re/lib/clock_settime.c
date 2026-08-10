/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include <errno.h>
#include <inttypes.h>
#include <time.h>
#include <l4/libc_backends/clk.h>
#include <l4/re/env.h>
#include <l4/sys/kip.h>

#include "clocks.h"

typedef int Get_clock(const struct timespec *);

static int rt_clock_settime(const struct timespec *tp)
{
  uint64_t clock;

  clock = tp->tv_sec * 1000000 + tp->tv_nsec / 1000;
  __libc_l4_rt_clock_offset = clock - l4_kip_clock(l4re_kip());
  return 0;
}

/* Only the realtime clock can be set; the monotonic ones are what they are. */
Get_clock *__libc_l4_settime[NCLOCKS] =
{
  [CLOCK_REALTIME]  = rt_clock_settime,
};

int clock_settime(clockid_t clk_id, const struct timespec *tp)
{
  if (clk_id < 0 || clk_id >= NCLOCKS || !__libc_l4_settime[clk_id])
    {
      errno = ENODEV;
      return -1;
    }

  return __libc_l4_settime[clk_id](tp);
}

