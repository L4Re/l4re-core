/*****************************************************************************/
/**
 * \file
 * \brief  Some useful generic macros, L4f  version
 *
 * \date   11/12/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de> */
/*
 * (c) 2000-2009 Author(s)
 *     economic rights: Technische Universität Dresden (Germany)
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

/*****************************************************************************/

#pragma once

#include <l4/util/arch/l4_macros.h>

/*****************************************************************************
 *** generic macros
 *****************************************************************************/

/* generate L4 thread id printf string */
#ifndef l4util_idstr
#  define l4util_idfmt         "%lx"
#  define l4util_idfmt_adjust  "%04lx"
#  define l4util_idstr(tid)    (tid >> L4_CAP_SHIFT)
#endif

