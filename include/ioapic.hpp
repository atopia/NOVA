/*
 * I/O Advanced Programmable Interrupt Controller (IOAPIC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#include "list.hpp"
#include "lock_guard.hpp"
#include "iommu.hpp"
#include "slab.hpp"

class Ioapic : public List<Ioapic>
{
    private:
        mword    const      reg_base;
        unsigned const      gsi_base;
        unsigned const      id;
        Iommu::Interface   *iommu { nullptr };
        uint16              rid;
        Spinlock            lock { };
        mword               saved_entries { };

        static Ioapic *     list;
        static Slab_cache   cache;

        enum
        {
            IOAPIC_IDX  = 0x0,
            IOAPIC_WND  = 0x10,
            IOAPIC_PAR  = 0x20,
            IOAPIC_EOI  = 0x40,
        };

        enum Register
        {
            IOAPIC_ID   = 0x0,
            IOAPIC_VER  = 0x1,
            IOAPIC_ARB  = 0x2,
            IOAPIC_BCFG = 0x3,
            IOAPIC_IRT  = 0x10,
        };

        ALWAYS_INLINE
        inline void index (Register reg)
        {
            *reinterpret_cast<uint8 volatile *>(reg_base + IOAPIC_IDX) = reg;
        }

        ALWAYS_INLINE
        inline uint32 read (Register reg)
        {
            Lock_guard <Spinlock> guard (lock);
            index (reg);
            return *reinterpret_cast<uint32 volatile *>(reg_base + IOAPIC_WND);
        }

        ALWAYS_INLINE
        inline void write (Register reg, uint32 val)
        {
            Lock_guard <Spinlock> guard (lock);
            index (reg);
            *reinterpret_cast<uint32 volatile *>(reg_base + IOAPIC_WND) = val;
        }

    public:
        INIT
        Ioapic (Paddr, unsigned, unsigned);

        ALWAYS_INLINE
        static inline void *operator new (size_t, Quota &quota) { return cache.alloc(quota); }

        ALWAYS_INLINE
        static inline bool claim_dev (unsigned r, unsigned i, Iommu::Interface *iommu)
        {
            for (Ioapic *ioapic = list; ioapic; ioapic = ioapic->next)
                if (ioapic->rid == 0 && ioapic->id == i) {
                    ioapic->rid   = static_cast<uint16>(r);
                    ioapic->iommu = iommu;
                    return true;
                }

            return false;
        }

        template <typename T>
        static void for_each (T const fn) {
            for (Ioapic *ioapic = list; ioapic; ioapic = ioapic->next) {
                fn(ioapic);
            }
        };

        ALWAYS_INLINE
        inline uint16 get_rid() const { return rid; }

        ALWAYS_INLINE
        inline unsigned get_gsi() const { return gsi_base; }

        ALWAYS_INLINE
        inline unsigned version() { return read (IOAPIC_VER) & 0xff; }

        ALWAYS_INLINE
        inline unsigned prq() { return read (IOAPIC_VER) >> 15 & 0x1; }

        ALWAYS_INLINE
        inline unsigned irt_max() { return read (IOAPIC_VER) >> 16 & 0xff; }

        ALWAYS_INLINE
        inline void set_irt (unsigned gsi, unsigned val)
        {
            unsigned pin = gsi - gsi_base;
            write (Register (IOAPIC_IRT + 2 * pin), val);
        }

        ALWAYS_INLINE
        inline void set_cpu (unsigned gsi, unsigned cpu, bool ire)
        {
            unsigned pin = gsi - gsi_base;
            write (Register (IOAPIC_IRT + 2 * pin + 1), ire ? (gsi << 17 | 1ul << 16) : (cpu << 24));
        }

        ALWAYS_INLINE
        inline uint64 get_irt(unsigned entry)
        {
            unsigned index = (IOAPIC_IRT + 2 * entry);
            return (uint64(read (Register (index))) << 32) |
                    uint64(read (Register (index+1)));
        }

        bool suspend(Quota &quota)
        {
            if (!saved_entries)
                saved_entries = mword(Buddy::allocator.alloc (0, quota, Buddy::FILL_0));

            auto const max = irt_max();

            if (!saved_entries || (max * sizeof(uint64) > PAGE_SIZE))
                return false;

            uint64 * const save = reinterpret_cast<uint64 *>(saved_entries);

            for (unsigned entry = 0; entry < max; entry++) {
                save[entry] = get_irt(entry);
            }

            return true;
        }

        bool resume()
        {
            auto const max = irt_max();

            if (!saved_entries || (max * sizeof(uint64) > PAGE_SIZE))
                return false;

            uint64 const * const saved = reinterpret_cast<uint64 *>(saved_entries);

            for (unsigned entry = 0; entry < max; entry++) {
                unsigned index = (IOAPIC_IRT + 2 * entry);
                write (Register (index),     uint32(saved[entry] >> 32));
                write (Register (index + 1), uint32(saved[entry]));
            }

            return true;
        }

        ALWAYS_INLINE
        Iommu::Interface * io_mmu() const { return iommu; };
};
