/*
 * Scheduling Context
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2012-2020 Alexander Boettcher, Genode Labs GmbH
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

#include "ec.hpp"
#include "lapic.hpp"
#include "stdio.hpp"
#include "timeout_budget.hpp"
#include "vectors.hpp"

INIT_PRIORITY (PRIO_LOCAL)
Sc::Rq Sc::rq;

Sc *        Sc::current;
unsigned    Sc::ctr_link;
unsigned    Sc::ctr_loop;
uint64      Sc::long_loop;
uint64      Sc::cross_time[NUM_CPU];
uint64      Sc::killed_time[NUM_CPU];

Sc *Sc::list[Sc::priorities];

unsigned Sc::prio_top;

Sc::Sc (Pd *own, mword sel, Ec *e) : Kobject (SC, static_cast<Space_obj *>(own), sel, 0x1, free), ec (e), cpu (static_cast<unsigned>(sel)), prio (0), budget (Lapic::freq_tsc * 1000), left (0)
{
    trace (TRACE_SYSCALL, "SC:%p created (PD:%p Kernel)", this, own);
}

Sc::Sc (Pd *own, mword sel, Ec *e, unsigned c, unsigned p, unsigned q) : Kobject (SC, static_cast<Space_obj *>(own), sel, 0x1, free, pre_free), ec (e), cpu (c), prio (static_cast<uint16>(p)), budget (Lapic::freq_tsc / 1000 * q), left (0)
{
    trace (TRACE_SYSCALL, "SC:%p created (EC:%p CPU:%#x P:%#x Q:%#x)", this, e, c, p, q);
}

Sc::Sc (Pd *own, Ec *e, unsigned c, Sc *x) : Kobject (SC, static_cast<Space_obj *>(own), 0, 0x1, free_x), ec (e), cpu (c), prio (x->prio), budget (x->budget), left (x->left)
{
    trace (TRACE_SYSCALL, "SC:%p created (EC:%p CPU:%#x P:%#x Q:%#llx) - xCPU", this, e, c, prio, budget / (Lapic::freq_bus / 1000));
}

Sc::Sc (Pd *own, Ec *e, Sc &s) : Kobject (SC, static_cast<Space_obj *>(own), s.node_base, 0x1, free, pre_free), ec (e), cpu (e->cpu), prio (s.prio), disable (s.disable), budget (s.budget), time (s.time), time_m (s.time_m), left (s.left)
{ }

void Sc::ready_enqueue (uint64 t, bool inc_ref, bool use_left)
{
    assert (prio < priorities);
    assert (cpu == Cpu::id);

    if (inc_ref) {
        bool ok = add_ref();
        assert (ok);
        if (!ok)
            return;
    }

    if (prio > prio_top)
        prio_top = prio;

    if (!list[prio])
        list[prio] = prev = next = this;
    else {
        next = list[prio];
        prev = list[prio]->prev;
        next->prev = prev->next = this;
        if (use_left && left)
            list[prio] = this;
    }

    trace (TRACE_SCHEDULE, "ENQ:%p (%llu) PRIO:%#x TOP:%#x %s", this, left, prio, prio_top, prio > current->prio ? "reschedule" : "");

    if (prio > current->prio || (this != current && prio == current->prio && (use_left && left)))
        Cpu::hazard |= HZD_SCHED;

    if (!left)
        left = budget;

    tsc = t;
}

void Sc::ready_dequeue (uint64 t)
{
    assert (prio < priorities);
    assert (cpu == Cpu::id);
    assert (prev && next);

    if (list[prio] == this)
        list[prio] = next == this ? nullptr : next;

    next->prev = prev;
    prev->next = next;
    prev = next = nullptr;

    while (!list[prio_top] && prio_top)
        prio_top--;

    trace (TRACE_SCHEDULE, "DEQ:%p (%llu) PRIO:%#x TOP:%#x", this, left, prio, prio_top);

    ec->add_tsc_offset (tsc - t);

    tsc = t;
}

void Sc::schedule (bool suspend, bool use_left)
{
    do {
        Counter::print<1,16> (++Counter::schedule, Console_vga::COLOR_LIGHT_CYAN, SPN_SCH);

        assert (current);
        assert (suspend || !current->prev);

        uint64 t = rdtsc();
        uint64 d = Timeout_budget::budget.dequeue();

        current->time += t - current->tsc;
        current->left = d > t ? d - t : 0;

        Cpu::hazard &= ~HZD_SCHED;

        if (EXPECT_FALSE(current->disable) && current->ec == Ec::current)
            suspend = true;

        if (EXPECT_TRUE (!suspend))
            current->ready_enqueue (t, false, use_left);
        else
            if (current->del_rcu())
                Rcu::call (current);

        Sc *sc = list[prio_top];
        assert (sc);

        Timeout_budget::budget.enqueue (t + sc->left);

        ctr_loop = 0;

        current = sc;
        current->ready_dequeue (t);
    } while (EXPECT_FALSE(current->disable) && current->ec == Ec::current);

    current->ec->activate();
}

void Sc::remote_enqueue(bool inc_ref)
{
    if (Cpu::id == cpu)
        ready_enqueue (rdtsc(), inc_ref);

    else {
        if (inc_ref) {
            bool ok = add_ref();
            assert (ok);
            if (!ok)
                return;
        }

        Sc::Rq *r = remote (cpu);

        Lock_guard <Spinlock> guard (r->lock);

        if (r->queue) {
            next = r->queue;
            prev = r->queue->prev;
            next->prev = prev->next = this;
        } else {
            r->queue = prev = next = this;
            Lapic::send_ipi (cpu, VEC_IPI_RRQ);
        }
    }
}

void Sc::rrq_handler()
{
    uint64 t = rdtsc();

    Lock_guard <Spinlock> guard (rq.lock);

    for (Sc *ptr = rq.queue; ptr; ) {

        ptr->next->prev = ptr->prev;
        ptr->prev->next = ptr->next;

        Sc *sc = ptr;

        ptr = ptr->next == ptr ? nullptr : ptr->next;

        sc->ready_enqueue (t, false);
    }

    rq.queue = nullptr;
}

void Sc::rke_handler()
{
    if (Sc::current->disable)
        Cpu::hazard |= HZD_SCHED;

    if (Pd::current->Space_mem::htlb.chk (Cpu::id))
        Cpu::hazard |= HZD_SCHED;
}

void Sc::operator delete (void *ptr)
{
    Pd * pd = static_cast<Sc *>(ptr)->ec->pd;
    pd->sc_cache.free (ptr, pd->quota);
}

void Sc::pre_free(Rcu_elem * a)
{
    Sc * s = static_cast<Sc *>(a);
    s->disable = true;

    if (Sc::current == s)
        Cpu::hazard |= HZD_SCHED;

    if (s->cpu != Sc::current->cpu)
        Lapic::send_ipi (s->cpu, VEC_IPI_RKE);
}
