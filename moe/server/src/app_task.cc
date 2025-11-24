/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include <l4/cxx/iostream>

#include "app_task.h"
#include "globals.h"
#include <l4/sigma0/sigma0.h>

extern "C" void cov_print(void) __attribute__((weak));

l4_ret_t
App_task::op_signal(L4Re::Parent::Rights, unsigned long sig, unsigned long val)
{
  switch (sig)
    {
    case 0: // exit
      {
        if (val != 0)
          L4::cout << "MOE: task " << this << " exited with " << val
                   << '\n';

        if (cov_print)
          {
            // dump moe's coverage
            cov_print();

            // let sigma0 dump its coverage
            l4sigma0_print_cov_data(Sigma0_cap);

            // let the kernel dump its coverage
            l4_msg_regs_t *v = l4_utcb_mr_u(l4_utcb());
            v->mr[0] = 0x400;
            l4_ipc_call(L4_BASE_DEBUGGER_CAP, l4_utcb(),
                        l4_msgtag(L4_PROTO_DEBUGGER, 1, 0, 0),
                        L4_IPC_NEVER);
          }

        // Invoke DTOR to remove init-task, its resources and redirect
        // in-flight IPCs.
        delete this;

        return -L4_ENOREPLY;
      }
    default: break;
    }
  return L4_EOK;
}

App_task::App_task()
  : _task(L4::Cap<L4::Task>::Invalid),
    _thread(L4::Cap<L4::Thread>::Invalid),
    _alloc(Allocator::root_allocator()),
    _rm(_alloc->make_obj<Region_map>())
{
  auto c = object_pool.cap_alloc()->alloc(_rm.get(), "moe-rm");
  c->dec_refcnt(1);
}

App_task::~App_task()
{
  if (_rm)
    delete _rm.get();

  object_pool.cap_alloc()->free(_thread, L4_FP_DELETE_OBJ);
  object_pool.cap_alloc()->free(_task, L4_FP_DELETE_OBJ);
}
