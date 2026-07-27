/*
 * (c) 2008-2009 Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
#pragma once

#include <l4/util/l4mod.h>

#ifdef CONFIG_MOE_BOOT_FB
void init_boot_fb(l4util_l4mod_info *mbi);
#else
static inline void init_boot_fb(l4util_l4mod_info *)
{}
#endif
