/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>,
 *               Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
#include "mem_man.h"
#include "globals.h"

#include <l4/cxx/iostream>
#include <l4/sys/assert.h>
#include <l4/sys/cxx/consts>

Mem_man Mem_man::_ram;

/**
 * Find an existing region which contains `r` ignoring region owner and rights.
 *
 * \param[in] r  The region to search for.
 *
 * \returns A region if a suitable region was found or nullptr if no such region
 *          was found.
 */
Region const *
Mem_man::find(Region const &r, bool force) const
{
  if (!r.valid())
    return nullptr;

  Tree::Const_iterator n = _tree.find(r);
  if (n == _tree.end())
    return nullptr;

  if (n->contains(r) || force)
    return &(*n);

  return nullptr;
}

/**
 * Add a new region to the list of regions.
 *
 * If possible, merge the new region with an existing region if the new region
 * ends immediately before or starts immediately after an existing region with
 * the same owner and the same rights.
 *
 * \return true   The region was added.
 * \return false  The region was not added due to lack of memory when allocating
 *                the new tree node.
 */
bool
Mem_man::add(Region const &r)
{
  /* try to merge with previous region */
  Region rs = r;
  if (rs.start() > 0)
    {
      /* Required because r.end = r.start + size - 1. Actually this decreases
       * r.start by 0x1000, not 1, because Region::start() rounds down to the
       * next 4K boundary, but that doesn't matter. */
      rs.start(rs.start() - 1);

      Tree::Node n = _tree.find_node(rs);
      if (n && n->owner() == r.owner() && n->rights() == r.rights())
        {
          r.start(n->start());
          int err = _tree.remove(*n);
          if (err < 0)
            {
              L4::cout << "err=" << err << " dump:\n";
              dump();
              l4_assert(!"BUG");
            }
        }
    }

  /* try to merge with next region */
  rs = r;
  if (rs.end() + 1 != 0)
    {
      /* required because r.end = r.start + size - 1 */
      rs.end(rs.end() + 1);

      Tree::Node n = _tree.find_node(rs);
      if (n && n->owner() == r.owner() && n->rights() == r.rights())
        {
          r.end(n->end());
          int err = _tree.remove(*n);
          if (err < 0)
            {
              L4::cout << "err=" << err << " dump:\n";
              dump();
              l4_assert(!"BUG");
            }
        }
    }

  /* do throw away regions owned by myself */
  if (r.owner() == sigma0_taskno)
    return true;

  if (_tree.insert(r).second == -_tree.E_nomem)
    {
      if (debug_errors)
        L4::cout << PROG_NAME": Out of memory\n";
      return false;
    }

  return true;
}

/**
 * Add region `r` to list of free regions.
 *
 * \param[in]  r Region to add.
 */
bool
Mem_man::add_free(Region const &r)
{
  if (!r.valid())
    return true;

  // calculate the combined set of all overlapping regions within the tree
  while (1)
    {
      Tree::Node n = _tree.find_node(r);

      if (!n)
        break;

      if (n->start() < r.start())
        r.start(n->start());
      if (n->end() > r.end())
        r.end(n->end());

      int err = _tree.remove(*n);
      if (err < 0)
        {
          L4::cout << "err=" << err << " dump:\n";
          dump();
          l4_assert(!"BUG");
        }
    }

  return add(r);
}

/**
 * Allocate region `r` from region `r2`.
 *
 * \param[in,out] r2  Region to allocate `r` from.
 * \param[in]     _r  Region.
 *
 * \return true   The region was allocated.
 * \return false  The region was not allocated.
 */
bool
Mem_man::alloc_from(Region const *r2, Region const &_r)
{
  if (!_r.valid())
    return false;

  Region r(_r);
  if (r2->owner() && r2->owner() != r.owner())
    return false;

  if (r2->owner() == r.owner() && r2->rights() == r.rights())
    return true;

  if (r == *r2)
    {
      if (0)
        {
          L4::cout << "dump " << r << " " << *r2 << "\n";
          dump();
        }
      int err = _tree.remove(*r2);
      if (err < 0)
        {
          L4::cout << "err=" << err << " dump:\n";
          dump();
          l4_assert(!"BUG");
        }
      return add(r);
    }

  Region r2_orig = *r2;

  if (r.start() == r2->start())
    {
      /* existing region now starts right after new region */
      r2->start(r.end() + 1);
      if (0)
        L4::cout << "move start to " << *r2 << '\n';
    }
  else if (r.end() == r2->end())
    {
      /* existing region now ends right before new region */
      r2->end(r.start() - 1);
      if (0)
        L4::cout << "shrink end to " << *r2 << '\n';
    }
  else
    {
      Region const nr(r.end() + 1, r2->end(), r2->owner(), r2->rights());
      r2->end(r.start() - 1);
      if (0)
        L4::cout << "split to " << *r2 << "; " << nr << '\n';

      if (nr.valid() && !add(nr))
        {
          r2->restore_range_from(r2_orig);
          return false;
        }
    }

  if (!add(r))
    {
      r2->restore_range_from(r2_orig);
      return false;
    }

  return true;
}

/**
 * Lookup the region containing `r` and, if found, remove the memory range
 * described by `r` from the found region.
 *
 * \param[in] r   Region to allocate.
 *
 * \return true   The region was allocated.
 * \return false  The region was not allocated.
 */
bool
Mem_man::alloc(Region const &r)
{
  if (!r.valid())
    return false;
  Region const *r2 = find(r);
  if (!r2)
    return false;

  if (0)
    L4::cout << "alloc_from(" << *r2 << ", " << r << ")\n";
  if (!alloc_from(r2, r))
    return false;

  return true;
}

/**
 * Allocate region from its containing region and inherit its rights.
 *
 * \param[in] r        Region to allocate. Its rights are the necessary minimal
 *                     rights of the containing region.
 * \param[out] rights  Address of the variable that will receive the full rights
 *                     of the containing region.
 *
 * \retval true   The region was allocated.
 * \retval false  The region was not allocated.
 */
bool
Mem_man::alloc_get_rights(Region const &r, L4_fpage_rights *rights)
{
  Region q = r;
  auto const *p = find(q);
  if (!p)
    return false;
  if ((p->rights() & q.rights()) != q.rights())
    return false;

  *rights = p->rights();
  q.rights(p->rights());
  return alloc_from(p, q);
}

/**
 * Add a reserved memory region into the map.
 *
 * \retval true   The region could be reserved.
 * \retval false  Error: The region could not be reserved.
 *
 * \note Any error is considered fatal and might create an inconsistent /
 *       incorrect memory map.
 */
bool
Mem_man::reserve(Region const &r)
{
  if (!r.valid())
    return false;

  for (;;)
    {
      auto r2 = _tree.find(r);

      //Region const *r2 = find(r, true);
      if (r2 == _tree.end())
        {
          if (0)
            L4::cout << this << ": ADD: " << r << "\n";
          return add(r);
        }

      if (0)
        L4::cout << this << ":  RESERVE: " << r << " from " << (*r2) << "\n";

      if (r2->owner() && r2->owner() != r.owner())
        return false;

      // allow exact matches to update owner and rights of the reserved region
      if (*r2 == r && (r2->owner() != r.owner() || r2->rights() != r.rights()))
        {
          int err = _tree.remove(*r2);
          if (err < 0)
            {
              L4::cout << "err=" << err << " dump:\n";
              dump();
              l4_assert(!"BUG");
            }
          return add(r);
        }

      // contained region will not get any updated rights
      if (r2->contains(r) && r2->owner() == r.owner())
        return true;

      if (r2->contains(r))
        {
          Region r2_orig = *r2;

          if (r2->start() == r.start())
            r2->start(r.end() + 1);
          else
            {
              Region const nr(r.end() + 1, r2->end(), r2->owner(), r2->rights());
              r2->end(r.start() - 1);
              if (0)
                L4::cout << this << ": ADDnr: " << nr << "\n";
              // FIXME: we could avoid the merge code for this add
              //        because this region is per definition not mergeable
              if (nr.valid() && !add(nr))
                {
                  r2->restore_range_from(r2_orig);
                  return false;
                }
            }

          if (0)
            L4::cout << this << ": ADD: " << r << "\n";

          if (!add(r))
            {
              r2->restore_range_from(r2_orig);
              return false;
            }
          return true;
        }

      if (r.contains(*r2))
        {
          if (0)
            L4::cout << this << ": REMOVE: " << *r2 << "\n";
          _tree.remove(*r2);
          continue;
        }

      if (r2->start() < r.start())
        {
          if (r2->owner())
            r.start(r2->end() + 1);
          else
            r2->end(r.start() - 1);
        }
      else
        {
          if (r2->owner())
            r.end(r2->start() - 1);
          else
            r2->start(r.end() + 1);
        }
    }
}

/**
 * Find a suitable region of at least the size of `2^order` without an assigned
 * owner.
 *
 * \param order  log2() of the requested region size.
 * \param owner  owner to allocate the new region for.
 *
 * \returns the start address of the allocated region or `~0UL` in case no such
 *          region could be found.
 */
unsigned long
Mem_man::alloc_first(unsigned order, unsigned owner)
{
  Tree::Item_type *n = 0;

  for (Tree::Iterator i = _tree.begin(); i != _tree.end(); ++i)
    {
      if (i->owner())
        continue;

      // wrap-around?
      if ((i->start() + (1UL << order) - 1) < i->start())
        continue;

      l4_addr_t st = L4::round_order(i->start(), order);
      if (0)
        L4::cout << "test: " << (void*)st << " - " << i->end() << '\n';

      if (st < i->end() && i->end() - st >= (1UL << order) - 1)
        {
          n = &(*i);
          break;
        }
    }

  if (!n)
    return ~0UL;

  Region a = Region::start_order(L4::round_order(n->start(), order), order,
                                 owner);

  if (!alloc_from(n, a))
    return ~0UL;

  return a.start();
}

/**
 * Debug dump.
 */
void
Mem_man::dump()
{
  for (Tree::Iterator i = _tree.begin(); i != _tree.end(); ++i)
    L4::cout << *i << '\n';
}
