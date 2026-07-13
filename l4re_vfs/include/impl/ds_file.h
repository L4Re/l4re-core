/*
 * (c) 2010 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *          Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
#pragma once

#include <l4/l4re_vfs/backend>

namespace L4Re { namespace Core {

/**
 * Implementation for files backed by an L4Re::Dataspace abstraction.
 */
class Ds_file : public L4Re::Vfs::Be_file_pos
{
private:
  L4::Cap<L4Re::Dataspace> _ds;
  off64_t _size;
  char *_addr;

  bool _writable;                    // Is this file writable?

public:
  explicit Ds_file(L4::Cap<L4Re::Dataspace> ds) noexcept
  : Be_file_pos(), _ds(ds), _addr(0)
  {
    L4Re::Dataspace::Stats ds_stats{};      // Properties of backing dataspace

    // Use a single IPC to query backing dataspace for size and permissions
    _ds->info(&ds_stats);
    _size = ds_stats.size;

    // w() returns true if flags has the write permission bit set
    _writable = ds_stats.flags.w();
  }

  L4::Cap<L4Re::Dataspace> data_space() noexcept override { return _ds; }

  int fstat(struct stat64 *buf) const noexcept override;

  int ioctl(unsigned long, va_list) noexcept override;

  off64_t size() const noexcept override { return _size; }

  int get_status_flags() const noexcept override
  { return O_RDONLY; }

  int set_status_flags(long) noexcept override
  { return 0; }

  /**
   * Check whether the file is ready for an I/O operation/condition.
   *
   * By definition, a dataspace-backed file is always ready for reading. If
   * the file has write permissions, it is also always ready for writing.
   *
   * \param rt  Type of the I/O operation/condition to be ready.
   *
   * \return Always true for reading. True for writing if writable.
   *         Always false otherwise.
   */
  bool check_ready(Ready_type rt) noexcept override
  {
    if (rt == Read)
      return true;

    if (rt == Write)
      return _writable;

    return false;
  }

  ~Ds_file() noexcept;

private:
  ssize_t read_single(const struct iovec*, off64_t) noexcept;
  ssize_t preadv(const struct iovec *, int, off64_t) noexcept override;
  ssize_t write_single(const struct iovec*, off64_t) noexcept;
  ssize_t pwritev(const struct iovec *, int , off64_t) noexcept override;
};


}}
