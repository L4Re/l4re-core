/*
 * Copyright (C) 2026 Kernkonzept GmbH.
 * Author(s): Georg Kotheimer <georg.kotheimer@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include "pi_mutex_mem.h"

#include <l4/cxx/bitmap>
#include <l4/re/env>
#include <l4/re/rm>
#include <l4/sys/compiler.h>
#include <l4/sys/task>
#include <l4/sys/utcb.h>

#include <cstddef>
#include <cstdio>

namespace
{

// OPTIMIZE: Detect cache line size.
constexpr unsigned Cache_line_size = 64;
constexpr unsigned Kumem_chunk_size = L4_PAGESIZE;
constexpr l4_addr_t Kumem_chunk_mask = L4_PAGEMASK;
constexpr unsigned Kumem_slot_size = sizeof(l4_umword_t);
// Estimate size of Kumem_chunk_meta, a slight overestimation, since for the
// bitmap size estimation we assume that the entire chunk is used for slots.
constexpr unsigned Kumem_chunk_meta_reserved =
  2 * sizeof(void *) + (Kumem_chunk_size / Kumem_slot_size / 8);
constexpr unsigned Kumem_chunk_data_size =
  Kumem_chunk_size - Kumem_chunk_meta_reserved;

/**
 * Per-chunk meta data for Kumem chunk allocator.
 *
 * The meta data is located at the start of each chunk.
 */
struct Kumem_chunk_meta
{
  static constexpr unsigned Num_slots = Kumem_chunk_data_size / Kumem_slot_size;
  // Number of complete cache lines (there can be another incomplete cache line).
  static constexpr unsigned Num_cache_lines = Kumem_chunk_data_size / Cache_line_size;
  static constexpr unsigned Slots_per_cache_line = Cache_line_size / Kumem_slot_size;
  static_assert(Cache_line_size % Kumem_slot_size == 0);

  Kumem_chunk_meta() { bitmap.clear_all(); }

  void *operator new(std::size_t, void *mem) { return mem; }

  l4_addr_t data() const
  {
    return reinterpret_cast<l4_addr_t>(this) + Kumem_chunk_meta_reserved;
  }

  l4_umword_t *alloc_slot()
  {
    for (;;)
      {
        long slot_idx = bitmap.scan_zero();
        // No free slot in this chunk.
        if (slot_idx < 0)
          return nullptr;

        if (bitmap.atomic_get_and_set(slot_idx))
          // Someone else was faster...
          continue;

        // Since `bitmap.atomic_get_and_set()` has only RELAXED semantics, we
        // need a fence to ensure that reads/writes to the allocated slot are
        // observable only after marking the slot as used in the bitmap.
        // Pairs with RELEASE fence in free_slot().
        __atomic_thread_fence(__ATOMIC_ACQUIRE);

        return reinterpret_cast<l4_umword_t *>(data() + slot_to_off(slot_idx));
      }
  }

  void free_slot(l4_umword_t *slot)
  {
    // Since `bitmap.atomic_clear_bit()` has only RELAXED semantics, we
    // need a fence to ensure that reads/writes to the freed slot are
    // observable before marking the slot as free in the bitmap.
    // Pairs with ACQUIRE fence in alloc_slot().
    __atomic_thread_fence(__ATOMIC_RELEASE);

    unsigned slot_off = reinterpret_cast<l4_addr_t>(slot) - data();
    bitmap.atomic_clear_bit(off_to_slot(slot_off));
  }

  /**
   * Distribute slots with a cache line-sized stride to reduce the contention.
   *
   * The slots are grouped in Num_cache_lines-sized groups, and then each group
   * member is put onto a different cache line.
   */
  static constexpr unsigned slot_to_off(unsigned slot)
  {
    // Identitiy mapping for the last (incomplete) cache line.
    if (L4_UNLIKELY(slot >= Num_cache_lines * Slots_per_cache_line))
      return slot * Kumem_slot_size;

    unsigned group_num = slot / Num_cache_lines;
    unsigned idx_in_group = slot % Num_cache_lines;
    unsigned idx = (idx_in_group * Slots_per_cache_line) + group_num;
    return idx * Kumem_slot_size;
  }

  static constexpr unsigned off_to_slot(unsigned off)
  {
    unsigned idx = off / Kumem_slot_size;
    // Identitiy mapping for the last (incomplete) cache line.
    if (L4_UNLIKELY(idx >= Num_cache_lines * Slots_per_cache_line))
      return idx;

    unsigned group_num = idx % Slots_per_cache_line;
    unsigned idx_in_group = idx / Slots_per_cache_line;
    return (group_num * Num_cache_lines) + idx_in_group;
  }

  // Pointer to next entry in linked list of allocated kumem chunks.
  Kumem_chunk_meta *next = nullptr;
  // Bitmap tracking the allocations in this chunk.
  cxx::Bitmap<Num_slots> bitmap;
};
static_assert(sizeof(Kumem_chunk_meta) <= Kumem_chunk_meta_reserved);
static_assert(Kumem_chunk_meta_reserved % Kumem_slot_size == 0,
              "Kumem chunk data not properly aligned.");


Kumem_chunk_meta *
allocate_pi_mutex_kumem_chunk()
{
  static_assert(Kumem_chunk_size == L4_PAGESIZE);

  l4_addr_t kumem = 0;
  L4Re::Env const *env = L4Re::Env::env();

#ifdef CONFIG_MMU
  // On MMU systems, user space chooses the spot in the virtual address space.
  if (env->rm()->reserve_area(&kumem, L4_PAGESIZE,
                              L4Re::Rm::F::Reserved | L4Re::Rm::F::Search_addr))
    return nullptr;

  l4_fpage_t fp = l4_fpage(kumem, L4_PAGESHIFT, L4_FPAGE_RW);
  if (l4_error(env->task()->add_ku_mem(&fp)))
    {
      env->rm()->free_area(kumem);
      return nullptr;
    }
#else
  // On systems without MMU the kernel determines the actual location.
  l4_fpage_t fp = l4_fpage(0, L4_PAGESHIFT, L4_FPAGE_RW);
  if (l4_error(env->task()->add_ku_mem(&fp)))
    return nullptr;
  kumem = l4_fpage_memaddr(fp);

  // The kernel allocated the address so it is known to be valid. The
  // reservation should never fail, unless something is really broken.
  long err =
    env->rm()->reserve_area(&kumem, L4_PAGESIZE, L4Re::Rm::F::Reserved);
  if (err < 0)
    fprintf(stderr,
            "ERROR: could not reserve ku_mem area, reserve_area returned %ld\n",
            err);
#endif

  return new (reinterpret_cast<char *>(kumem)) Kumem_chunk_meta();
}

// Head of lockless linked list of kumem chunks that is expanded on demand.
Kumem_chunk_meta *_head_kumem_chunk = nullptr;

} // namespace

unsigned long *
__pthread_alloc_pi_mutex_kumem_slot()
{
  Kumem_chunk_meta *kumem_chunk =
    __atomic_load_n(&_head_kumem_chunk, __ATOMIC_RELAXED);
  for (;;)
    {
      if (!kumem_chunk)
        {
          // Reached end of list, need to allocate new chunk.
          Kumem_chunk_meta *new_chunk = allocate_pi_mutex_kumem_chunk();
          if (!new_chunk)
            return nullptr;

          new_chunk->next =
            __atomic_load_n(&_head_kumem_chunk, __ATOMIC_RELAXED);
          // The data dependency of following the head_kumem_chunk pointer
          // ensures that readers observe a properly initialized
          // Kumem_chunk_meta.
          while (!__atomic_compare_exchange_n(&_head_kumem_chunk,
                                              &new_chunk->next, new_chunk, true,
                                              __ATOMIC_ACQ_REL,
                                              __ATOMIC_RELAXED))
            ; // Failed cas updated new_chunk->next to the new head_kumem_chunk.

          kumem_chunk = new_chunk;
        }

      if (l4_umword_t *slot = kumem_chunk->alloc_slot())
        return slot;

      kumem_chunk = kumem_chunk->next;
    }
}

void
__pthread_free_pi_mutex_kumem_slot(unsigned long *slot)
{
  // Truncate slot address to page start to get chunk metadata.
  auto kumem_chunk_addr = reinterpret_cast<l4_addr_t>(slot) & Kumem_chunk_mask;
  reinterpret_cast<Kumem_chunk_meta *>(kumem_chunk_addr)->free_slot(slot);
}
