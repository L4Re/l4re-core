/*
 * Support for dynamic linking code in static libc.
 * Copyright (C) 1996-2002, 2003, 2004, 2005 Free Software Foundation, Inc.
 *
 * Partially based on GNU C Library (file: libc/elf/dl-support.c)
 *
 * Copyright (C) 2008 STMicroelectronics Ltd.
 * Author: Carmelo Amoroso <carmelo.amoroso@st.com>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 *
 */

#include <link.h>
#include <ldso.h>
#include <elf.h>
#if defined(__UCLIBC_HAS_THREADS__)
#include <bits/libc-lock.h>
#endif
#if defined(USE_TLS) && USE_TLS
#include <assert.h>
#include <tls.h>
#include <stdbool.h>
#include <ldsodefs.h>
#include <string.h>
#endif
#include <bits/uClibc_page.h>

#if defined(USE_TLS) && USE_TLS

void (*_dl_init_static_tls) (struct link_map *) = &_dl_nothread_init_static_tls;

#endif

ElfW(Phdr) *_dl_phdr;
size_t _dl_phnum;
size_t _dl_pagesize;

#ifdef CONFIG_MMU
ElfW(auxv_t) _dl_auxvt[AUX_MAX_AT_ID];
#endif
ElfW(auxv_t) *_dl_auxv_start;

#ifndef __NOT_FOR_L4__
extern void *l4re_global_env __attribute__((weak));
extern void *l4_global_kip __attribute__((weak));
#endif

void internal_function _dl_aux_init (ElfW(auxv_t) *av);
void internal_function _dl_aux_init (ElfW(auxv_t) *av)
{
   _dl_auxv_start = av;

#ifdef CONFIG_MMU
   memset(_dl_auxvt, 0x00, sizeof(_dl_auxvt));
#endif
   for (; av->a_type != AT_NULL; av++)
     {
#ifdef CONFIG_MMU
       if (av->a_type < AUX_MAX_AT_ID)
         _dl_auxvt[av->a_type] = *av;
#endif

       switch (av->a_type)
        {
        case AT_PHDR:
          /* Get the program headers base address from the aux vect */
          _dl_phdr = (ElfW(Phdr) *) av->a_un.a_val;
          break;
        case AT_PHNUM:
          /* Get the number of program headers from the aux vect */
          _dl_phnum = (size_t) av->a_un.a_val;
          break;
        case AT_PAGESZ:
          /* Get the pagesize from the aux vect */
          _dl_pagesize = (av->a_un.a_val) ? (size_t) av->a_un.a_val : PAGE_SIZE;
          break;
#ifndef __NOT_FOR_L4__
        case AT_L4_ENV:
          if (&l4re_global_env)
            l4re_global_env = (void *)av->a_un.a_val;
          break;
        case AT_L4_KIP:
          if (&l4_global_kip)
            l4_global_kip = (void *)av->a_un.a_val;
          break;
#endif
        }
     }
}

#if defined(USE_TLS) && USE_TLS
/* Initialize static TLS area and DTV for current (only) thread.
   libpthread implementations should provide their own hook
   to handle all threads.  */
void
attribute_hidden
_dl_nothread_init_static_tls (struct link_map *map)
{
# if defined(TLS_TCB_AT_TP)
  void *dest = (char *) THREAD_SELF - map->l_tls_offset;
# elif defined(TLS_DTV_AT_TP)
  void *dest = (char *) THREAD_SELF + map->l_tls_offset + TLS_PRE_TCB_SIZE;
# else
#  error "Either TLS_TCB_AT_TP or TLS_DTV_AT_TP must be defined"
# endif

  /* Fill in the DTV slot so that a later LD/GD access will find it.  */
  dtv_t *dtv = THREAD_DTV ();
  assert (map->l_tls_modid <= dtv[-1].counter);
  dtv[map->l_tls_modid].pointer.val = dest;
  dtv[map->l_tls_modid].pointer.is_static = true;

  /* Initialize the memory.  */
  memset (mempcpy (dest, map->l_tls_initimage, map->l_tls_initimage_size),
	  '\0', map->l_tls_blocksize - map->l_tls_initimage_size);
}

#endif

#if defined(__UCLIBC_HAS_THREADS__)
__rtld_lock_define_initialized_recursive (, _dl_load_lock)
__rtld_lock_define_initialized_recursive (, _dl_load_write_lock)
#endif
