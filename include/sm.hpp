/*
 * Semaphore
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2014-2015 Alexander Boettcher, Genode Labs GmbH.
 *
 * This file is part of the NOVA microhypervisor.
 *
 * NOVA is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NOVA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License version 2 for more details.
 */

#pragma once

#include "ec.hpp"
#include "si.hpp"

class Sm : public Kobject, public Refcount, public Queue<Ec>, public Queue<Si>, public Si
{
    private:
        mword counter;

        static void free (Rcu_elem * a) {
            Sm * sm = static_cast <Sm *>(a);

            if (sm->del_ref()) {
                Pd *pd = static_cast<Pd *>(static_cast<Space_obj *>(sm->space));

                Slab *slab = reinterpret_cast<Slab *>(reinterpret_cast<mword>(sm) & ~PAGE_MASK);
                assert (slab);
                assert (slab->cache);
                assert (slab->cache == &pd->sm_cache);

                destroy(sm, *pd);
            } else {
                sm->up();
            }
        }

    public:

        mword reset(bool l = false) {
            if (l) lock.lock();
            mword c = counter;
            counter = 0;
            if (l) lock.unlock();
            return c;
        }

        Sm (Pd *, mword, mword = 0, Sm * = nullptr, mword = 0);
        ~Sm ()
        {
            while (!counter)
                up (Ec::sys_finish<Sys_regs::BAD_CAP, true>);
        }

        ALWAYS_INLINE
        inline void dn (bool zero, uint64 t, Ec *ec = Ec::current, bool block = true)
        {
            {   Lock_guard <Spinlock> guard (lock);

                if (counter) {
                    counter = zero ? 0 : counter - 1;

                    Si * si;
                    if (Queue<Si>::dequeue(si = Queue<Si>::head()))
                        ec->set_si_regs(si->value, static_cast <Sm *>(si)->reset());

                    return;
                }

                if (!ec->add_ref()) {
                    Sc::schedule (block);
                    return;
                }

                Queue<Ec>::enqueue (ec);
            }

            if (!block)
                Sc::schedule (false);

            ec->set_timeout (t, this);

            ec->block_sc();

            ec->clr_timeout();
        }

        ALWAYS_INLINE
        inline void up (void (*c)() = nullptr, Sm * si = nullptr)
        {
            Ec *ec = nullptr;

            do {
                if (ec)
                    Rcu::call (ec);

                {   Lock_guard <Spinlock> guard (lock);

                    if (!Queue<Ec>::dequeue (ec = Queue<Ec>::head())) {

                        if (si) {
                           if (si->queued()) return;
                           Queue<Si>::enqueue(si);
                        }

                        counter++;
                        return;
                    }

                }

                if (si) ec->set_si_regs(si->value, si->reset(true));

                ec->release (c);

            } while (EXPECT_FALSE(ec->del_rcu()));
        }

        ALWAYS_INLINE
        inline void timeout (Ec *ec)
        {
            {   Lock_guard <Spinlock> guard (lock);

                if (!Queue<Ec>::dequeue (ec))
                    return;
            }

            ec->release (Ec::sys_finish<Sys_regs::COM_TIM>);
        }

        ALWAYS_INLINE
        inline void add_to_rcu()
        {
            if (!add_ref())
                return;

            if (!Rcu::call (this))
                /* enqueued ? - drop our ref and add to rcu if necessary */
                if (del_rcu())
                    Rcu::call (this);
        }

        ALWAYS_INLINE
        static inline void *operator new (size_t, Pd &pd) { return pd.sm_cache.alloc(pd.quota); }

        ALWAYS_INLINE
        static inline void destroy(Sm *obj, Pd &pd) { obj->~Sm(); pd.sm_cache.free (obj, pd.quota); }
};
