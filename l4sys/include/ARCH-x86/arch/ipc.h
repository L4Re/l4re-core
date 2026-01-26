/**
 * \file
 * \brief   L4 IPC System Calls, x86
 * \ingroup l4_api
 */
/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>,
 *               Lars Reuther <reuther@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
#ifndef __L4_IPC_H__
#define __L4_IPC_H__

#include <l4/sys/types.h>
#include <l4/sys/consts.h>

#ifdef __PIC__
# define L4S_PIC_SAVE "push %%ebx; "
# define L4S_PIC_RESTORE "pop %%ebx; "
# define L4S_PIC_CLOBBER
# define L4S_PIC_SYSCALL , [func] "m" (__l4sys_invoke_indirect)
extern void (*__l4sys_invoke_indirect)(void);
# define IPC_SYSENTER      "# indirect sys invoke \n\t" \
                           "call *%[func]    \n\t"
# define IPC_SYSENTER_ASM   call __l4sys_invoke_direct@plt
#else
/**
 * \internal
 * \brief Kernel entry code for inline assembly.
 */
#define IPC_SYSENTER      "call __l4sys_invoke_direct    \n\t"
/**
 * \internal
 * \brief Kernel entry code for assembler code.
 */
#define IPC_SYSENTER_ASM   call __l4sys_invoke_direct
/**
 * \internal
 * \brief Save PIC register, if needed.
 */
#  define L4S_PIC_SAVE
/**
 * \internal
 * \brief Restore PIC register, if needed.
 */
#  define L4S_PIC_RESTORE
/**
 * \internal
 * \brief PIC clobber list.
 */
#  define L4S_PIC_CLOBBER ,"ebx"
#  define L4S_PIC_SYSCALL

#endif

L4_INLINE l4_msgtag_t
l4_ipc(l4_cap_idx_t dest, l4_utcb_t *u,
       l4_umword_t flags,
       l4_umword_t slabel,
       l4_msgtag_t tag,
       l4_umword_t *rlabel,
       l4_timeout_t timeout) L4_NOTHROW
{
  l4_umword_t dummy, dummy1, dummy2;

  (void)u;

  __asm__ __volatile__
    (L4S_PIC_SAVE "push %%ebp; " IPC_SYSENTER " pop %%ebp; " L4S_PIC_RESTORE
     :
     "=d" (dummy2),
     "=S" (slabel),
     "=c" (dummy1),
     "=D" (dummy),
     "=a" (tag.raw)
     :
     "S" (slabel),
     "c" (timeout),
     "a" (tag.raw),
     "d" (dest | flags)
     L4S_PIC_SYSCALL
     :
     "memory", "cc" L4S_PIC_CLOBBER
     );

  if (rlabel)
    *rlabel = slabel;

  return tag;
}

#endif /* !__L4_IPC_H__ */
