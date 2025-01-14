/*
 * Virtual Translation Lookaside Buffer (VTLB)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "pte.hpp"
#include "user.hpp"

class Exc_regs;

#ifdef __i386__
class Vtlb : public Pte<Vtlb, uint32, 2, 10, false, false>
#else
class Vtlb : public Pte<Vtlb, uint64, 3,  9, false, false>
#endif
{
    private:
        ALWAYS_INLINE
        inline bool mark() const { return val & TLB_M; }

        ALWAYS_INLINE
        inline bool frag() const { return val & TLB_F; }

        ALWAYS_INLINE
        static inline bool mark_pte (uint32 *pte, uint32 old, uint32 bits)
        {
            return EXPECT_TRUE ((old & bits) == bits) || User::cmp_swap (pte, old, old | bits) == ~0UL;
        }

        void flush_ptab (bool);

    public:
        static size_t gwalk (Exc_regs *, mword, mword &, mword &, mword &);
        static size_t hwalk (mword, mword &, mword &, mword &);

        enum
        {
            TLB_P   = 1UL << 0,
            TLB_W   = 1UL << 1,
            TLB_U   = 1UL << 2,
            TLB_UC  = 1UL << 4,
            TLB_A   = 1UL << 5,
            TLB_D   = 1UL << 6,
            TLB_S   = 1UL << 7,
            TLB_G   = 1UL << 8,
            TLB_F   = 1UL << 9,
            TLB_M   = 1UL << 10,

            PTE_P   = TLB_P,
        };

        enum Reason
        {
            SUCCESS,
            GLA_GPA,
            GPA_HPA
        };

        ALWAYS_INLINE
        inline bool super() const { return val & TLB_S; }

        ALWAYS_INLINE
        static inline mword pte_s(unsigned long const l) { return l ? TLB_S : 0; }

        ALWAYS_INLINE
        inline Vtlb()
        {
            for (unsigned i = 0; i < 1UL << bpl(); i++)
                this[i].val = TLB_S;
        }

        void flush (mword);
        void flush (bool);

        static Reason miss (Exc_regs *, mword, mword &);

        ALWAYS_INLINE
        static inline void *operator new (size_t, Quota &quota) { return Buddy::allocator.alloc (0, quota, Buddy::NOFILL); }

        ALWAYS_INLINE
        static inline void destroy(Vtlb *obj, Quota &quota) { obj->~Vtlb(); Buddy::allocator.free (reinterpret_cast<mword>(obj), quota); }
};
