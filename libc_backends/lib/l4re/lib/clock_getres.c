/*
 * Copyright (C) 2024-2025 Kernkonzept GmbH.
 * Author(s): Jan Klötzke <jan.kloetzke@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include <errno.h>
#include <time.h>

#include "clocks.h"

typedef int Get_clock(struct timespec *);
extern Get_clock *__libc_l4_gettime[];

int clock_getres(clockid_t clock_id, struct timespec * res)
{
  if (clock_id < 0 || clock_id >= NCLOCKS || __libc_l4_gettime[clock_id] == NULL)
    {
      errno = EINVAL;
      return -1;
    }

  if (res)
    {
      res->tv_sec = 0;
      res->tv_nsec = 1000;
    }

  return 0;
}
