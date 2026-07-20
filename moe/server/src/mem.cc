/*
 * Copyright (C) 2008-2009, 2025 Kernkonzept GmbH.
 * Author(s): Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *            Alexander Warg <warg@os.inf.tu-dresden.de>
 *            Martin Decky <martin.decky@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include <cstddef>
#include <l4/sys/consts.h>
#include <l4/umalloc/umalloc.h>
#include "page_alloc.h"

// Populate the page allocator with a few pages of memory reserved in BSS to
// allow for dynamic allocation of memory arena during the static
// initialization of stdc++'s emergency_pool in GCC versions 5 and newer.
alignas(L4_PAGESIZE) static char umalloc_scratch_mem[4 * L4_PAGESIZE];
static unsigned long umalloc_scratch_mem_used = 0;

size_t umalloc_area_granularity = L4_PAGESIZE;

void *umalloc_area_create(size_t area_size) noexcept
{
  if (area_size <= sizeof(umalloc_scratch_mem) - umalloc_scratch_mem_used)
    {
      void *mem = umalloc_scratch_mem + umalloc_scratch_mem_used;
      umalloc_scratch_mem_used += area_size;
      return mem;
    }

  return Single_page_alloc_base::_alloc(Single_page_alloc_base::nothrow,
                                        l4_round_page(area_size), L4_PAGESIZE);
}
