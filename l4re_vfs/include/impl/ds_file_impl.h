/*
 * (c) 2010 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *          Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include "ds_file.h"

#include <limits.h>
#include <sys/ioctl.h>

#include <l4/re/env>

namespace L4Re { namespace Core {

Ds_file::~Ds_file() noexcept
{
  if (_addr)
    L4Re::Env::env()->rm()->detach(l4_addr_t(_addr), 0);

  L4Re::virt_cap_alloc->release(_ds);
}

int
Ds_file::fstat(struct stat64 *buf) const noexcept
{
  static int fake = 0;

  memset(buf, 0, sizeof(*buf));
  buf->st_size = _size;
  buf->st_mode = S_IFREG | 0644;
  buf->st_dev = _ds.cap();
  buf->st_ino = ++fake;
  buf->st_blksize = L4_PAGESIZE;
  buf->st_blocks = l4_round_page(_size);
  return 0;
}

ssize_t
Ds_file::read_single(const struct iovec *vec, off64_t pos) noexcept
{
  // POSIX declares read operations with a length > SSIZE_MAX to not be
  // portable, so we do not have to support them. This check also ensures that
  // casting the size_t variable vec->iov_len to an ssize_t does not overflow.
  if (vec->iov_len > SSIZE_MAX)
    return -EINVAL;

  off64_t l = vec->iov_len;
  if (_size - pos < l)
    l = _size - pos;

  if (l > 0)
    {
      Vfs_config::memcpy(vec->iov_base, _addr + pos, l);
      return l;
    }

  return 0;
}

ssize_t
Ds_file::preadv(const struct iovec *vec, int cnt, off64_t offset) noexcept
{
  if (cnt < 0 || offset < 0)
    return -EINVAL;

  if (!_size)
    return 0;

  if (!_addr)
    {
      // Region manager flags and dataspace cap need to be adjusted according
      // to file permissions
      Rm::Flags rm_flags = Rm::F::Search_addr | Rm::F::R;
      L4::Ipc::Cap<L4Re::Dataspace> ds_cap = _ds;
      if (_writable)
        {
          rm_flags |= Rm::F::W;
          ds_cap = L4::Ipc::make_cap_rw(_ds);
        }

      void *file = reinterpret_cast<void*>(L4_PAGESIZE);
      long err = L4Re::Env::env()->rm()->attach(&file, _size, rm_flags,
                                                ds_cap, 0);

      if (err < 0)
        return err;

      _addr = static_cast<char *>(file);
    }

  ssize_t l = 0;

  while (cnt > 0)
    {
      ssize_t r = read_single(vec, offset);

      // This check also ensures that casting r to a size_t does not cause
      // overflows if r is negative.
      if (r < 0)
        return (l == 0) ? r : l;

      offset += r;
      l      += r;

      if (static_cast<size_t>(r) < vec->iov_len)
        return l;

      ++vec;
      --cnt;
    }
  return l;
}

ssize_t
Ds_file::write_single(const struct iovec *vec, off64_t pos) noexcept
{
  // POSIX declares write operations with a length > SSIZE_MAX to not be
  // portable, so we do not have to support them. This check also ensures that
  // casting the size_t variable vec->iov_len to an ssize_t does not overflow.
  if (vec->iov_len > SSIZE_MAX)
    return -EINVAL;

  // The memcpy should never result in invalid memory accesses! Therefore,
  // check for overflow, potentially performing a short write.
  off64_t l = vec->iov_len;
  if (_size - pos < l)
    l = _size - pos;

  if (l > 0)
    {
      Vfs_config::memcpy(_addr + pos, vec->iov_base, l);
      return l;
    }

  // The write operation would happen entirely beyond EOF. For now, we cannot
  // extend the file, so return an appropriate error code.
  return -ENOSPC;
}

ssize_t
Ds_file::pwritev(const struct iovec *vec, int cnt, off64_t offset) noexcept
{
  if (cnt < 0 || offset < 0)
    return -EINVAL;

  if (! _writable)
    return -EBADF;

  // Create a mapping if none is established yet
  if (! _addr)
    {
      void *file = reinterpret_cast<void*>(L4_PAGESIZE);
      long err = L4Re::Env::env()->rm()->attach(&file, _size,
                                                Rm::F::Search_addr | Rm::F::RW,
                                                L4::Ipc::make_cap_rw(_ds), 0);

      if (err < 0)
        return err;

      _addr = static_cast<char *>(file);
    }

  // Copy loop, iterating over all I/O vectors
  ssize_t l = 0;                        // No. of bytes copied so far
  while (cnt > 0)
    {
      ssize_t r = write_single(vec, offset);

      // This check also ensures that casting r to a size_t does not cause
      // overflows if r is negative.
      if (r < 0)
        return (l == 0) ? r : l;

      offset += r;
      l      += r;

      // Check for short writes
      if (static_cast<size_t>(r) < vec->iov_len)
        return l;

      ++vec;
      --cnt;
    }

  return l;
}

int
Ds_file::ioctl(unsigned long v, va_list args) noexcept
{
  switch (v)
    {
    case FIONREAD: // return amount of data still available
      int *available = va_arg(args, int *);
      *available = _size - pos();
      return 0;
    };
  return -ENOTTY;
}

}}
