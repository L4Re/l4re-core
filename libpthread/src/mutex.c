/* Linuxthreads - a simple clone()-based implementation of Posix        */
/* threads for Linux.                                                   */
/* Copyright (C) 1996 Xavier Leroy (Xavier.Leroy@inria.fr)              */
/*                                                                      */
/* This program is free software; you can redistribute it and/or        */
/* modify it under the terms of the GNU Library General Public License  */
/* as published by the Free Software Foundation; either version 2       */
/* of the License, or (at your option) any later version.               */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU Library General Public License for more details.                 */

/* Mutexes */

#include <bits/libc-lock.h>
#include <errno.h>
#include <stddef.h>
#include <limits.h>
#include "pthread.h"
#include "internals.h"
#include "pi_mutex_mem.h"
#include "spinlock.h"
#include "queue.h"
#include "restart.h"

#include <l4/re/c/util/cap_alloc.h>
#include <l4/re/env.h>
#include <l4/sys/compiler.h>
#include <l4/sys/factory.h>

int
L4_HIDDEN
__pthread_mutex_init(pthread_mutex_t * mutex,
                     const pthread_mutexattr_t * mutex_attr)
{
  mutex->__m_kind =
    mutex_attr == NULL ? PTHREAD_MUTEX_TIMED_NP : mutex_attr->__mutexkind;
  mutex->__m_protocol  =
    mutex_attr == NULL ? PTHREAD_PRIO_NONE : mutex_attr->__mutexprotocol;
  mutex->__m_count = 0;
  mutex->__m_owner = NULL;

  if(mutex->__m_protocol == PTHREAD_PRIO_NONE)
    {
      __pthread_init_lock(&mutex->__m_lock);
      return 0;
    }

  if (mutex->__m_protocol == PTHREAD_PRIO_INHERIT)
    {
      mutex->__m_pi_lock.pi_mutex = L4_INVALID_CAP;
      mutex->__m_pi_lock.status = __pthread_alloc_pi_mutex_kumem_slot();
      if (!mutex->__m_pi_lock.status)
        return ENOMEM;
      *mutex->__m_pi_lock.status = 0;

      mutex->__m_pi_lock.pi_mutex = l4re_util_cap_alloc();
      if (l4_is_invalid_cap(mutex->__m_pi_lock.pi_mutex))
        {
          __pthread_free_pi_mutex_kumem_slot(mutex->__m_pi_lock.status);
          mutex->__m_pi_lock.status = NULL;
          return ENOMEM;
        }

      int err = l4_error(l4_factory_create_pi_mutex(
        l4re_env()->factory, mutex->__m_pi_lock.pi_mutex,
        (l4_addr_t)mutex->__m_pi_lock.status, L4RE_THIS_TASK_CAP));
      if (err < 0)
        {
          l4re_util_cap_free(mutex->__m_pi_lock.pi_mutex);
          mutex->__m_pi_lock.pi_mutex = L4_INVALID_CAP;
          __pthread_free_pi_mutex_kumem_slot(mutex->__m_pi_lock.status);
          mutex->__m_pi_lock.status = NULL;

          switch (err)
            {
            case -L4_ENOMEM: return ENOMEM;
            case -L4_EPERM: return EPERM;
            default: return -err;
            }
        }
      return 0;
    }

  return EINVAL;
}
L4_STRONG_ALIAS(__pthread_mutex_init, pthread_mutex_init)

int
L4_HIDDEN
__pthread_mutex_destroy(pthread_mutex_t * mutex)
{
  if (mutex->__m_protocol == PTHREAD_PRIO_INHERIT)
    {
      if (mutex->__m_pi_lock.status == NULL)
        // Mutex not initialized.
        return 0;

      if (*mutex->__m_pi_lock.status != 0)
        // Mutex still in use.
        return EBUSY;

      // Free kernel mutex object
      l4re_util_cap_free_um(mutex->__m_pi_lock.pi_mutex);
      mutex->__m_pi_lock.pi_mutex = L4_INVALID_CAP;

      // Free kumem slot
      __pthread_free_pi_mutex_kumem_slot(mutex->__m_pi_lock.status);
      mutex->__m_pi_lock.status = NULL;

      return 0;
    }

  switch (mutex->__m_kind) {
  case PTHREAD_MUTEX_ADAPTIVE_NP:
  case PTHREAD_MUTEX_RECURSIVE_NP:
    if ((mutex->__m_lock.__status & 1) != 0)
      return EBUSY;
    return 0;
  case PTHREAD_MUTEX_ERRORCHECK_NP:
  case PTHREAD_MUTEX_TIMED_NP:
    if (mutex->__m_lock.__status != 0)
      return EBUSY;
    return 0;
  default:
    return EINVAL;
  }
}
L4_STRONG_ALIAS(__pthread_mutex_destroy, pthread_mutex_destroy)

int
L4_HIDDEN
__pthread_mutex_trylock(pthread_mutex_t * mutex)
{
  pthread_descr self;
  int retcode;

  switch(mutex->__m_kind) {
  case PTHREAD_MUTEX_ADAPTIVE_NP:
    if (mutex->__m_protocol == PTHREAD_PRIO_INHERIT)
      retcode = __pthread_trylock_pi(&mutex->__m_pi_lock, thread_self());
    else
      retcode = __pthread_trylock(&mutex->__m_lock);
    return retcode;

  case PTHREAD_MUTEX_RECURSIVE_NP:
    self = thread_self();
    if (mutex->__m_owner == self) {
      mutex->__m_count++;
      return 0;
    }
    if (mutex->__m_protocol == PTHREAD_PRIO_INHERIT)
      retcode = __pthread_trylock_pi(&mutex->__m_pi_lock, self);
    else
      retcode = __pthread_trylock(&mutex->__m_lock);
    if (retcode == 0) {
      mutex->__m_owner = self;
      mutex->__m_count = 0;
    }
    return retcode;

  case PTHREAD_MUTEX_ERRORCHECK_NP:
    if (mutex->__m_protocol == PTHREAD_PRIO_INHERIT)
      retcode = __pthread_trylock_pi(&mutex->__m_pi_lock, thread_self());
    else
      retcode = __pthread_alt_trylock(&mutex->__m_lock);
    if (retcode == 0) {
      mutex->__m_owner = thread_self();
    }
    return retcode;

  case PTHREAD_MUTEX_TIMED_NP:
    if (mutex->__m_protocol == PTHREAD_PRIO_INHERIT)
      retcode = __pthread_trylock_pi(&mutex->__m_pi_lock, thread_self());
    else
      retcode = __pthread_alt_trylock(&mutex->__m_lock);
    return retcode;

  default:
    return EINVAL;
  }
}
L4_STRONG_ALIAS(__pthread_mutex_trylock, pthread_mutex_trylock)

int
L4_HIDDEN
__pthread_mutex_lock(pthread_mutex_t * mutex)
{
  pthread_descr self;

  int res = 0;
  switch(mutex->__m_kind) {
  case PTHREAD_MUTEX_ADAPTIVE_NP:
    if (mutex->__m_protocol == PTHREAD_PRIO_INHERIT)
      res = __pthread_lock_pi_deadlock(&mutex->__m_pi_lock, thread_self());
    else
      __pthread_lock(&mutex->__m_lock, NULL);
    return res;

  case PTHREAD_MUTEX_RECURSIVE_NP:
    self = thread_self();
    if (mutex->__m_owner == self) {
      mutex->__m_count++;
      return 0;
    }
    if (mutex->__m_protocol == PTHREAD_PRIO_INHERIT)
      res = __pthread_lock_pi_deadlock(&mutex->__m_pi_lock, self);
    else
      __pthread_lock(&mutex->__m_lock, self);
    if (res == 0)
      {
        mutex->__m_owner = self;
        mutex->__m_count = 0;
      }
    return res;

  case PTHREAD_MUTEX_ERRORCHECK_NP:
    self = thread_self();
    if (mutex->__m_owner == self) return EDEADLK;
    if (mutex->__m_protocol == PTHREAD_PRIO_INHERIT)
      res = __pthread_lock_pi(&mutex->__m_pi_lock, self);
    else
      __pthread_alt_lock(&mutex->__m_lock, self);
    if (res == 0)
      mutex->__m_owner = self;
    return res;

  case PTHREAD_MUTEX_TIMED_NP:
    if (mutex->__m_protocol == PTHREAD_PRIO_INHERIT)
      res = __pthread_lock_pi_deadlock(&mutex->__m_pi_lock, thread_self());
    else
      __pthread_alt_lock(&mutex->__m_lock, NULL);
    return res;

  default:
    return EINVAL;
  }
}

L4_STRONG_ALIAS(__pthread_mutex_lock, pthread_mutex_lock)


int
L4_HIDDEN
__pthread_mutex_timedlock (pthread_mutex_t *mutex,
			       const struct timespec *abstime)
{
  pthread_descr self;
  int res = 0;

  if (__builtin_expect (abstime->tv_nsec, 0) < 0
      || __builtin_expect (abstime->tv_nsec, 0) >= 1000000000)
    return EINVAL;

  switch(mutex->__m_kind) {
  case PTHREAD_MUTEX_ADAPTIVE_NP:
    if (mutex->__m_protocol == PTHREAD_PRIO_INHERIT)
      return __pthread_timedlock_pi_deadlock(&mutex->__m_pi_lock, thread_self(),
                                             abstime);

    __pthread_lock(&mutex->__m_lock, NULL);
    return 0;

  case PTHREAD_MUTEX_RECURSIVE_NP:
    self = thread_self();
    if (mutex->__m_owner == self) {
      mutex->__m_count++;
      return 0;
    }
    if (mutex->__m_protocol == PTHREAD_PRIO_INHERIT)
    {
      res = __pthread_timedlock_pi_deadlock(&mutex->__m_pi_lock, thread_self(),
                                            abstime);
      if (res != 0)
        return res;
    }
    else
      __pthread_lock(&mutex->__m_lock, self);

    mutex->__m_owner = self;
    mutex->__m_count = 0;
    return 0;

  case PTHREAD_MUTEX_ERRORCHECK_NP:
    self = thread_self();
    if (mutex->__m_owner == self) return EDEADLK;
    if (mutex->__m_protocol == PTHREAD_PRIO_INHERIT)
      {
        res =
          __pthread_timedlock_pi(&mutex->__m_pi_lock, thread_self(), abstime);
        if (res != 0)
          return res;
      }
    else
      {
        res = __pthread_alt_timedlock(&mutex->__m_lock, self, abstime);
        if (res == 0)
          return ETIMEDOUT;
      }

    mutex->__m_owner = self;
    return 0;

  case PTHREAD_MUTEX_TIMED_NP:
    if (mutex->__m_protocol == PTHREAD_PRIO_INHERIT)
      return __pthread_timedlock_pi_deadlock(&mutex->__m_pi_lock, thread_self(),
                                             abstime);

    /* Only this type supports timed out lock. */
    return (__pthread_alt_timedlock(&mutex->__m_lock, NULL, abstime)
	    ? 0 : ETIMEDOUT);

  default:
    return EINVAL;
  }
}
L4_STRONG_ALIAS(__pthread_mutex_timedlock, pthread_mutex_timedlock)

int
L4_HIDDEN
__pthread_mutex_unlock(pthread_mutex_t * mutex)
{
  int res = 0;
  switch (mutex->__m_kind) {
  case PTHREAD_MUTEX_ADAPTIVE_NP:
    if (mutex->__m_protocol == PTHREAD_PRIO_INHERIT)
      res = __pthread_unlock_pi(&mutex->__m_pi_lock, thread_self());
    else
    __pthread_unlock(&mutex->__m_lock);
    return res;

  case PTHREAD_MUTEX_RECURSIVE_NP:
    if (mutex->__m_owner != thread_self())
      return EPERM;
    if (mutex->__m_count > 0) {
      mutex->__m_count--;
      return 0;
    }
    // Must reset owner BEFORE doing the unlock.
    mutex->__m_owner = NULL;
    if (mutex->__m_protocol == PTHREAD_PRIO_INHERIT)
      res = __pthread_unlock_pi(&mutex->__m_pi_lock, thread_self());
    else
      __pthread_unlock(&mutex->__m_lock);
    if (res != 0)
      // On failure, restore ownership.
      mutex->__m_owner = thread_self();
    return res;

  case PTHREAD_MUTEX_ERRORCHECK_NP:
    if (mutex->__m_owner != thread_self() || mutex->__m_lock.__status == 0)
      return EPERM;
    // Must reset owner BEFORE doing the unlock.
    mutex->__m_owner = NULL;
    if (mutex->__m_protocol == PTHREAD_PRIO_INHERIT)
      res = __pthread_unlock_pi(&mutex->__m_pi_lock, thread_self());
    else
      __pthread_alt_unlock(&mutex->__m_lock);
    if (res != 0)
      // On failure, restore ownership.
      mutex->__m_owner = thread_self();
    return res;

  case PTHREAD_MUTEX_TIMED_NP:
    if (mutex->__m_protocol == PTHREAD_PRIO_INHERIT)
      res = __pthread_unlock_pi(&mutex->__m_pi_lock, thread_self());
    else
      __pthread_alt_unlock(&mutex->__m_lock);
    return res;

  default:
    return EINVAL;
  }
}
L4_STRONG_ALIAS(__pthread_mutex_unlock, pthread_mutex_unlock)

int
L4_HIDDEN
__pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
  attr->__mutexkind = PTHREAD_MUTEX_TIMED_NP;
  attr->__mutexprotocol = PTHREAD_PRIO_NONE;
  return 0;
}
L4_STRONG_ALIAS(__pthread_mutexattr_init, pthread_mutexattr_init)

int
L4_HIDDEN
__pthread_mutexattr_destroy(pthread_mutexattr_t *attr __attribute__((unused)))
{
  return 0;
}
L4_STRONG_ALIAS(__pthread_mutexattr_destroy, pthread_mutexattr_destroy)

int
L4_HIDDEN
__pthread_mutexattr_settype(pthread_mutexattr_t *attr, int kind)
{
  if (kind != PTHREAD_MUTEX_ADAPTIVE_NP
      && kind != PTHREAD_MUTEX_RECURSIVE_NP
      && kind != PTHREAD_MUTEX_ERRORCHECK_NP
      && kind != PTHREAD_MUTEX_TIMED_NP)
    return EINVAL;
  attr->__mutexkind = kind;
  return 0;
}
L4_WEAK_ALIAS(__pthread_mutexattr_settype, pthread_mutexattr_settype)
L4_STRONG_ALIAS( __pthread_mutexattr_settype, __pthread_mutexattr_setkind_np)
L4_WEAK_ALIAS(__pthread_mutexattr_setkind_np, pthread_mutexattr_setkind_np)

int
L4_HIDDEN
__pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *kind)
{
  *kind = attr->__mutexkind;
  return 0;
}
L4_WEAK_ALIAS(__pthread_mutexattr_gettype, pthread_mutexattr_gettype)
L4_STRONG_ALIAS(__pthread_mutexattr_gettype, __pthread_mutexattr_getkind_np)
L4_WEAK_ALIAS(__pthread_mutexattr_getkind_np, pthread_mutexattr_getkind_np)

int
L4_HIDDEN
__pthread_mutexattr_getpshared (const pthread_mutexattr_t *attr __attribute__((unused)),
                                int *pshared)
{
  *pshared = PTHREAD_PROCESS_PRIVATE;
  return 0;
}
L4_WEAK_ALIAS(__pthread_mutexattr_getpshared, pthread_mutexattr_getpshared)

int
L4_HIDDEN
__pthread_mutexattr_setpshared (pthread_mutexattr_t *attr __attribute__((unused)),
                                int pshared)
{
  if (pshared != PTHREAD_PROCESS_PRIVATE && pshared != PTHREAD_PROCESS_SHARED)
    return EINVAL;

  /* For now it is not possible to shared a conditional variable.  */
  if (pshared != PTHREAD_PROCESS_PRIVATE)
    return ENOSYS;

  return 0;
}
L4_WEAK_ALIAS(__pthread_mutexattr_setpshared, pthread_mutexattr_setpshared)


/* Return in *PROTOCOL the mutex protocol attribute in *ATTR.  */
int
L4_HIDDEN
__pthread_mutexattr_getprotocol (const pthread_mutexattr_t * __restrict attr,
                                 int *__restrict protocol)
{
  *protocol = attr->__mutexprotocol;
  return 0;
}
L4_WEAK_ALIAS(__pthread_mutexattr_getprotocol, pthread_mutexattr_getprotocol)

/* Set the mutex protocol attribute in *ATTR to PROTOCOL (either
   PTHREAD_PRIO_NONE, PTHREAD_PRIO_INHERIT, or PTHREAD_PRIO_PROTECT).  */
int
L4_HIDDEN
__pthread_mutexattr_setprotocol (pthread_mutexattr_t *attr,
            int protocol)
{
    if (protocol != PTHREAD_PRIO_NONE
      && protocol != PTHREAD_PRIO_INHERIT)
    return EINVAL;

  attr->__mutexprotocol = protocol;
  return 0;
}
L4_WEAK_ALIAS(__pthread_mutexattr_setprotocol, pthread_mutexattr_setprotocol)

/* Return in *PRIOCEILING the mutex prioceiling attribute in *ATTR.  */
int
L4_HIDDEN
__pthread_mutexattr_getprioceiling (const pthread_mutexattr_t * __restrict attr,
                                    int *__restrict prioceiling)
{
  return EINVAL;
}
L4_WEAK_ALIAS(__pthread_mutexattr_getprioceiling, pthread_mutexattr_getprioceiling)

/* Set the mutex prioceiling attribute in *ATTR to PRIOCEILING.  */
int
L4_HIDDEN
__pthread_mutexattr_setprioceiling (pthread_mutexattr_t *attr, int prioceiling)
{
  return EINVAL;
}
L4_WEAK_ALIAS(__pthread_mutexattr_setprioceiling, pthread_mutexattr_setprioceiling)

/* Once-only execution */

static pthread_mutex_t once_masterlock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t once_finished = PTHREAD_COND_INITIALIZER;
static int fork_generation = 0;	/* Child process increments this after fork. */

enum { NEVER = 0, IN_PROGRESS = 1, DONE = 2 };

/* If a thread is canceled while calling the init_routine out of
   pthread once, this handler will reset the once_control variable
   to the NEVER state. */

static void pthread_once_cancelhandler(void *arg)
{
    pthread_once_t *once_control = arg;

    pthread_mutex_lock(&once_masterlock);
    *once_control = NEVER;
    pthread_mutex_unlock(&once_masterlock);
    pthread_cond_broadcast(&once_finished);
}

int
L4_HIDDEN
__pthread_once(pthread_once_t * once_control, void (*init_routine)(void))
{
  /* flag for doing the condition broadcast outside of mutex */
  int state_changed;

  /* Test without locking first for speed */
  if (*once_control == DONE) {
    READ_MEMORY_BARRIER();
    return 0;
  }
  /* Lock and test again */

  state_changed = 0;

  pthread_mutex_lock(&once_masterlock);

  /* If this object was left in an IN_PROGRESS state in a parent
     process (indicated by stale generation field), reset it to NEVER. */
  if ((*once_control & 3) == IN_PROGRESS && (*once_control & ~3) != fork_generation)
    *once_control = NEVER;

  /* If init_routine is being called from another routine, wait until
     it completes. */
  while ((*once_control & 3) == IN_PROGRESS) {
    pthread_cond_wait(&once_finished, &once_masterlock);
  }
  /* Here *once_control is stable and either NEVER or DONE. */
  if (*once_control == NEVER) {
    *once_control = IN_PROGRESS | fork_generation;
    pthread_mutex_unlock(&once_masterlock);
    pthread_cleanup_push(pthread_once_cancelhandler, once_control);
    init_routine();
    pthread_cleanup_pop(0);
    pthread_mutex_lock(&once_masterlock);
    WRITE_MEMORY_BARRIER();
    *once_control = DONE;
    state_changed = 1;
  }
  pthread_mutex_unlock(&once_masterlock);

  if (state_changed)
    pthread_cond_broadcast(&once_finished);

  return 0;
}
L4_STRONG_ALIAS(__pthread_once, pthread_once)

/*
 * Handle the state of the pthread_once mechanism across forks.  The
 * once_masterlock is acquired in the parent process prior to a fork to ensure
 * that no thread is in the critical region protected by the lock.  After the
 * fork, the lock is released. In the child, the lock and the condition
 * variable are simply reset.  The child also increments its generation
 * counter which lets pthread_once calls detect stale IN_PROGRESS states
 * and reset them back to NEVER.
 */

void
L4_HIDDEN
__pthread_once_fork_prepare(void)
{
  pthread_mutex_lock(&once_masterlock);
}

void
L4_HIDDEN
__pthread_once_fork_parent(void)
{
  pthread_mutex_unlock(&once_masterlock);
}

void
L4_HIDDEN
__pthread_once_fork_child(void)
{
  pthread_mutex_init(&once_masterlock, NULL);
  pthread_cond_init(&once_finished, NULL);
  if (fork_generation <= INT_MAX - 4)
    fork_generation += 4;	/* leave least significant two bits zero */
  else
    fork_generation = 0;
}
