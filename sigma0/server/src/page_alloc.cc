/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
#include "page_alloc.h"

Page_alloc_base::Alloc Page_alloc_base::_alloc;
unsigned long Page_alloc_base::_total;

// One page is sufficient for about 80 regions on 64-bit targets or 160 regions
// on 32-bit targets. Maintaining RAM usually requires only a few regions as the
// root task allocates the entire RAM during startup. A static region list is
// used for the MMIO space (no user tracking), so the number of required regions
// depends mostly on the fragmentation of RAM and MMIO space, and, on x86, on
// the variable number of I/O port regions.
alignas(L4_PAGESIZE) static
  char page_alloc_scratch_mem[CONFIG_SIGMA0_HEAP_NUM_PAGES * L4_PAGESIZE];

void Page_alloc_base::init()
{
  // Page_alloc::free() always uses L4_PAGESIZE
  for (unsigned i = 0; i < sizeof(page_alloc_scratch_mem) / L4_PAGESIZE; ++i)
    free(page_alloc_scratch_mem + i * L4_PAGESIZE);
}
