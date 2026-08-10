/*
 * Copyright (C) 2008 TU Dresden,
 *               2014 Kernkonzept GmbH.
 * Author(s): Adam Lackorzynski <adam@l4re.org>
 *            Alexander Warg <alexander.warg@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include <errno.h>
#include <stdio.h>
#include <sched.h>
#include <unistd.h>

#include <l4/re/env>
#include <l4/sys/consts.h>
#include <l4/sys/scheduler>

namespace {

/* Number of CPUs our scheduler offers us. */
static long num_online_cpus(void)
{
  size_t const bits_per_word = sizeof(l4_umword_t) * 8;
  l4_umword_t cpu_max = 0;
  long count = 0;
  unsigned offset = 0;

  do
    {
      l4_sched_cpu_set_t cs = l4_sched_cpu_set(offset, 0, 0);

      if (l4_error(l4_scheduler_info(l4re_env()->scheduler, &cpu_max, &cs)) < 0)
        break;

      count += __builtin_popcountg(cs.map);

      offset += bits_per_word;
    }
  while (offset < cpu_max);

  /* We are running somewhere, so never claim there is no CPU at all. */
  return count > 0 ? count : 1;
}

static long
phys_pages(bool available)
{
  auto ma = L4Re::Env::env()->mem_alloc();
  if (!ma.is_valid())
    {
      errno = ENODEV;
      return -1;
    }

  L4Re::Mem_alloc::Stats stats;
  long err = ma->info(stats);
  if (err < 0)
    {
      errno = -err;
      return -1;
    }

  l4_size_t const bytes = available ? stats.mem_free : stats.mem_limit;
  return bytes / L4_PAGESIZE;
}

} // namespace

/*
 * Backend of CPU_COUNT() and CPU_COUNT_S(): the number of CPUs set in the
 * given mask. This says nothing about how many CPUs the system has.
 */
int __sched_cpucount(size_t __setsize, const cpu_set_t *__setp)
{
  size_t const words = __setsize / sizeof(__setp->__bits[0]);

  if (__setp == nullptr)
    return 0;

  int count = 0;
  for (size_t w = 0; w < words; ++w)
    count += __builtin_popcountg(__setp->__bits[w]);

  return count;
}

long sysconf(int name)
{
  switch (name)
  {
  case _SC_NPROCESSORS_CONF:
  case _SC_NPROCESSORS_ONLN:
    return num_online_cpus();
  case _SC_PAGE_SIZE:
    return L4_PAGESIZE;
  case _SC_PHYS_PAGES:
    return phys_pages(false);
  case _SC_AVPHYS_PAGES:
    return phys_pages(true);
  case _SC_CLK_TCK:
    return 1000;
  case _SC_MONOTONIC_CLOCK:
    return 200112L;
  case _SC_OPEN_MAX:
    return 512;
  case _SC_CHILD_MAX:
    return 2000;
  case _SC_LINE_MAX:
    return 2048;
  case _SC_NGROUPS_MAX:
    return 32;
  default:
    break;
  }
  fprintf(stderr, "%s: unknown command, name=%d\n", __func__, name);
  return 0;
}
