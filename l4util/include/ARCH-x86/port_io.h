/*****************************************************************************/
/**
 * \file
 * \brief  x86 port I/O
 *
 * \date   06/2003
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/*
 * (c) 2003-2009 Author(s)
 *     economic rights: Technische Universität Dresden (Germany)
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#ifndef _L4UTIL_PORT_IO_H
#define _L4UTIL_PORT_IO_H

/**
 * \defgroup l4util_port_io IA32 Port I/O API
 * \ingroup l4util_api
 */

/* L4 includes */
#include <l4/sys/l4int.h>
#include <l4/sys/compiler.h>
#include <l4/sys/types.h>

/*****************************************************************************
 *** Prototypes
 *****************************************************************************/

L4_BEGIN_DECLS
/**
 * \addtogroup l4util_port_io
 */
/**@{*/
/**
 * \brief Read byte from I/O port
 *
 * \param  port	   I/O port address
 * \return value
 */
L4_INLINE l4_uint8_t
l4util_in8(l4_uint16_t port);

/**
 * \brief Read 16-bit-value from I/O port
 *
 * \param  port	   I/O port address
 * \return value
 */
L4_INLINE l4_uint16_t
l4util_in16(l4_uint16_t port);

/**
 * \brief Read 32-bit-value from I/O port
 *
 * \param  port	   I/O port address
 * \return value
 */
L4_INLINE l4_uint32_t
l4util_in32(l4_uint16_t port);

/**
 * \brief Read a block of 8-bit-values from I/O ports
 *
 * \param  port	   I/O port address
 * \param  addr    address of buffer
 * \param  count   number of I/O operations
 */
L4_INLINE void
l4util_ins8(l4_uint16_t port, l4_umword_t addr, l4_umword_t count);

/**
 * \brief Read a block of 16-bit-values from I/O ports
 *
 * \param  port	   I/O port address
 * \param  addr    address of buffer
 * \param  count   number of I/O operations
 */
L4_INLINE void
l4util_ins16(l4_uint16_t port, l4_umword_t addr, l4_umword_t count);

/**
 * \brief Read a block of 32-bit-values from I/O ports
 *
 * \param  port	   I/O port address
 * \param  addr    address of buffer
 * \param  count   number of I/O operations
 */
L4_INLINE void
l4util_ins32(l4_uint16_t port, l4_umword_t addr, l4_umword_t count);

/**
 * \brief Write byte to I/O port
 *
 * \param  port	   I/O port address
 * \param  value   value to write
 */
L4_INLINE void
l4util_out8(l4_uint8_t value, l4_uint16_t port);

/**
 * \brief Write 16-bit-value to I/O port
 *
 * \param  port	   I/O port address
 * \param  value   value to write
 */
L4_INLINE void
l4util_out16(l4_uint16_t value, l4_uint16_t port);

/**
 * \brief Write 32-bit-value to I/O port
 *
 * \param  port	   I/O port address
 * \param  value   value to write
 */
L4_INLINE void
l4util_out32(l4_uint32_t value, l4_uint16_t port);

/**
 * \brief Write a block of bytes to I/O port
 *
 * \param  port	   I/O port address
 * \param  addr    address of buffer
 * \param  count   number of I/O operations
 */
L4_INLINE void
l4util_outs8(l4_uint16_t port, l4_umword_t addr, l4_umword_t count);

/**
 * \brief Write a block of 16-bit-values to I/O port
 *
 * \param  port	   I/O port address
 * \param  addr    address of buffer
 * \param  count   number of I/O operations
 */
L4_INLINE void
l4util_outs16(l4_uint16_t port, l4_umword_t addr, l4_umword_t count);

/**
 * \brief Write block of 32-bit-values to I/O port
 *
 * \param  port	   I/O port address
 * \param  addr    address of buffer
 * \param  count   number of I/O operations
 */
L4_INLINE void
l4util_outs32(l4_uint16_t port, l4_umword_t addr, l4_umword_t count);

/**
 * \brief delay I/O port access by writing to port 0x80
 */
L4_INLINE void
l4util_iodelay(void);

/**
 * \brief Map a range of I/O ports.
 * \ingroup l4util_port_io
 *
 * \param sigma0id   I/O port service (sigma0).
 * \param port_start (Start) Port to request.
 * \param log2size   Log2size of range to request.
 *
 * \return IPC result: 0 if the range could be successfully mapped
 *         on error: IPC failure, or -L4_ENOENT if nothing mapped
 */
L4_INLINE int
l4util_ioport_map(l4_cap_idx_t sigma0id,
                  unsigned port_start, unsigned log2size);

/**@}*/

L4_END_DECLS


/*****************************************************************************
 *** Implementation
 *****************************************************************************/

#include <l4/sys/utcb.h>
#include <l4/sys/ipc.h>

L4_INLINE l4_uint8_t
l4util_in8(l4_uint16_t port)
{
  l4_uint8_t value;
  asm volatile ("inb %w1, %b0" : "=a" (value) : "Nd" (port));
  return value;
}

L4_INLINE l4_uint16_t
l4util_in16(l4_uint16_t port)
{
  l4_uint16_t value;
  asm volatile ("inw %w1, %w0" : "=a" (value) : "Nd" (port));
  return value;
}

L4_INLINE l4_uint32_t
l4util_in32(l4_uint16_t port)
{
  l4_uint32_t value;
  asm volatile ("inl %w1, %0" : "=a" (value) : "Nd" (port));
  return value;
}

L4_INLINE void
l4util_ins8(l4_uint16_t port, l4_umword_t addr, l4_umword_t count)
{
  l4_umword_t dummy1, dummy2;
  asm volatile ("rep insb" : "=D"(dummy1), "=c"(dummy2)
			   : "d" (port), "D" (addr), "c"(count)
			   : "memory");
}

L4_INLINE void
l4util_ins16(l4_uint16_t port, l4_umword_t addr, l4_umword_t count)
{
  l4_umword_t dummy1, dummy2;
  asm volatile ("rep insw" : "=D"(dummy1), "=c"(dummy2)
			   : "d" (port), "D" (addr), "c"(count)
			   : "memory");
}

L4_INLINE void
l4util_ins32(l4_uint16_t port, l4_umword_t addr, l4_umword_t count)
{
  l4_umword_t dummy1, dummy2;
  asm volatile ("rep insl" : "=D"(dummy1), "=c"(dummy2)
			   : "d" (port), "D" (addr), "c"(count)
			   : "memory");
}

L4_INLINE void
l4util_out8(l4_uint8_t value, l4_uint16_t port)
{
  asm volatile ("outb %b0, %w1" : : "a" (value), "Nd" (port));
}

L4_INLINE void
l4util_out16(l4_uint16_t value, l4_uint16_t port)
{
  asm volatile ("outw %w0, %w1" : : "a" (value), "Nd" (port));
}

L4_INLINE void
l4util_out32(l4_uint32_t value, l4_uint16_t port)
{
  asm volatile ("outl %0, %w1" : : "a" (value), "Nd" (port));
}

L4_INLINE void
l4util_outs8(l4_uint16_t port, l4_umword_t addr, l4_umword_t count)
{
  l4_umword_t dummy1, dummy2;
  asm volatile ("rep outsb" : "=S"(dummy1), "=c"(dummy2)
			    : "d" (port), "S" (addr), "c"(count)
			    : "memory");
}

L4_INLINE void
l4util_outs16(l4_uint16_t port, l4_umword_t addr, l4_umword_t count)
{
  l4_umword_t dummy1, dummy2;
  asm volatile ("rep outsw" : "=S"(dummy1), "=c"(dummy2)
			    : "d" (port), "S" (addr), "c"(count)
			    : "memory");
}

L4_INLINE void
l4util_outs32(l4_uint16_t port, l4_umword_t addr, l4_umword_t count)
{
  l4_umword_t dummy1, dummy2;
  asm volatile ("rep outsl" : "=S"(dummy1), "=c"(dummy2)
			    : "d" (port), "S" (addr), "c"(count)
			    : "memory");
}

L4_INLINE void
l4util_iodelay(void)
{
  asm volatile ("outb %al,$0x80");
}

L4_INLINE int
l4util_ioport_map(l4_cap_idx_t sigma0id,
                  unsigned port_start, unsigned log2size)
{
  l4_fpage_t iofp;
  l4_msgtag_t tag;
  long err;

  iofp = l4_iofpage(port_start, log2size);
  l4_utcb_mr()->mr[0] = iofp.raw;
  l4_utcb_br()->bdr   = 0;
  l4_utcb_br()->br[0] = L4_ITEM_MAP;
  l4_utcb_br()->br[1] = iofp.raw;
  tag = l4_ipc_call(sigma0id, l4_utcb(),
	            l4_msgtag(L4_PROTO_IO_PAGE_FAULT, 1, 0, 0),
                    L4_IPC_NEVER);

  if ((err = l4_ipc_error(tag, l4_utcb())))
    return err;

  return l4_msgtag_items(tag) > 0 ? 0 : -L4_ENOENT;
}

#endif
