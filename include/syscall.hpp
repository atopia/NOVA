/*
 * System-Call Interface
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2014-2020 Alexander Boettcher, Genode Labs GmbH.
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

#include "qpd.hpp"

class Sys_call : public Sys_regs
{
    public:
        enum
        {
            DISABLE_BLOCKING    = 1ul << 0,
            DISABLE_DONATION    = 1ul << 1,
            DISABLE_REPLYCAP    = 1ul << 2
        };

        ALWAYS_INLINE
        inline unsigned long pt() const { return ARG_1 >> 8; }
};

class Sys_create_pd : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sel() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline unsigned long pd() const { return ARG_2; }

        ALWAYS_INLINE
        inline Crd crd() const { return Crd (ARG_3); }

        ALWAYS_INLINE
        inline unsigned long limit_lower() const { return ARG_4 & (~0UL >> (sizeof(mword) * 4)); }

        ALWAYS_INLINE
        inline unsigned long limit_upper() const { return ARG_4 >> (sizeof(mword) * 4); }
};

class Sys_create_ec : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sel() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline unsigned long pd() const { return ARG_2; }

        ALWAYS_INLINE
        inline unsigned cpu() const { return ARG_3 & 0xfff; }

        ALWAYS_INLINE
        inline mword utcb() const { return ARG_3 & ~0xfff; }

        ALWAYS_INLINE
        inline mword esp() const { return ARG_4; }

        ALWAYS_INLINE
        inline unsigned evt() const { return static_cast<unsigned>(ARG_5); }
};

class Sys_create_sc : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sel() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline unsigned long pd() const { return ARG_2; }

        ALWAYS_INLINE
        inline unsigned long ec() const { return ARG_3; }

        ALWAYS_INLINE
        inline Qpd qpd() const { return Qpd (ARG_4); }
};

class Sys_create_pt : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sel() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline unsigned long pd() const { return ARG_2; }

        ALWAYS_INLINE
        inline unsigned long ec() const { return ARG_3; }

        ALWAYS_INLINE
        inline Mtd mtd() const { return Mtd (ARG_4); }

        ALWAYS_INLINE
        inline mword eip() const { return ARG_5; }
};

class Sys_create_sm : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sel() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline unsigned long pd() const { return ARG_2; }

        ALWAYS_INLINE
        inline mword cnt() const { return ARG_3; }

        ALWAYS_INLINE
        inline unsigned long sm() const { return ARG_4; }
};

class Sys_revoke : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline Crd crd() const { return Crd (ARG_2); }

        ALWAYS_INLINE
        inline bool self() const { return flags() & 0x1; }

        ALWAYS_INLINE
        inline bool remote() const { return flags() & 0x2; }

        ALWAYS_INLINE
        inline bool keep() const { return flags() & 0x4; }

        ALWAYS_INLINE
        inline mword pd() const { return ARG_3; }

        ALWAYS_INLINE
        inline mword sm() const { return ARG_1 >> 8; }

        inline void rem(Pd * p) { ARG_3 = reinterpret_cast<mword>(p); }
};

class Sys_misc : public Sys_regs
{
    public:
        enum { SYS_LOOKUP = 0, SYS_DELEGATE = 1, SYS_ACPI_SUSPEND };

        ALWAYS_INLINE
        inline Crd & crd() { return reinterpret_cast<Crd &>(ARG_2); }

        ALWAYS_INLINE
        inline mword pd_snd() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline mword pd_dst() const { return ARG_3; }

        ALWAYS_INLINE
        inline mword sleep_type_a() const { return ARG_2; }

        ALWAYS_INLINE
        inline mword sleep_type_b() const { return ARG_3; }
};

class Sys_reply : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sm() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline mword sm_kern() const { return ARG_1; }
};

class Sys_ec_ctrl : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long ec() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline unsigned long cnt() const { return ARG_2; }

        ALWAYS_INLINE
        inline unsigned op() const { return flags() & 0x7; }

        ALWAYS_INLINE
        inline bool state() const { return ARG_2 == 1; }

        ALWAYS_INLINE
        inline unsigned cpu() const { return ARG_2 & 0xfff; }

        ALWAYS_INLINE
        inline Crd crd() const { return Crd (ARG_3); }

        inline void set_time (uint64 val)
        {
            ARG_2 = static_cast<mword>(val >> 32);
            ARG_3 = static_cast<mword>(val);
        }
};

class Sys_sc_ctrl : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sc() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline unsigned long ec() const { return ARG_2; }

        ALWAYS_INLINE
        inline unsigned op() const { return flags() & 0x3; }

        ALWAYS_INLINE
        inline void set_time (uint64 val)
        {
            ARG_2 = static_cast<mword>(val >> 32);
            ARG_3 = static_cast<mword>(val);
        }

        ALWAYS_INLINE
        inline void set_time (uint64 val, uint64 val2)
        {
            ARG_2 = static_cast<mword>(val >> 32);
            ARG_3 = static_cast<mword>(val);
            ARG_4 = static_cast<mword>(val2 >> 32);
            ARG_5 = static_cast<mword>(val2);
        }
};

class Sys_pt_ctrl : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long pt() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline mword id() const { return ARG_2; }
};

class Sys_sm_ctrl : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sm() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline unsigned op() const { return flags() & 0x1; }

        ALWAYS_INLINE
        inline unsigned zc() const { return flags() & 0x2; }

        ALWAYS_INLINE
        inline uint64 time() const { return static_cast<uint64>(ARG_2) << 32 | ARG_3; }
};

class Sys_pd_ctrl : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long src() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline unsigned dbg() const { return flags() & 0x2; }

        ALWAYS_INLINE
        inline unsigned long dst() const { return ARG_2; }

        ALWAYS_INLINE
        inline unsigned long tra() const { return ARG_3; }

        ALWAYS_INLINE
        inline void dump (mword l, mword u)
        {
            ARG_2 = l;
            ARG_3 = u;
        }
};

class Sys_assign_pci : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long pd() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline mword dev() const { return ARG_2; }

        ALWAYS_INLINE
        inline mword hnt() const { return ARG_3; }
};

class Sys_assign_gsi : public Sys_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned long sm() const { return ARG_1 >> 8; }

        ALWAYS_INLINE
        inline mword dev() const { return ARG_2; }

        ALWAYS_INLINE
        inline unsigned cpu() const { return static_cast<unsigned>(ARG_3); }

        ALWAYS_INLINE
        inline mword si() const { return ARG_4; }

        ALWAYS_INLINE
        inline bool cfg() const { return flags() & 0b100; }

        ALWAYS_INLINE
        inline bool trg() const { return flags() & 0b010; }

        ALWAYS_INLINE
        inline bool pol() const { return flags() & 0b001; }

        ALWAYS_INLINE
        inline void set_msi (uint64 val)
        {
            ARG_2 = static_cast<mword>(val >> 32);
            ARG_3 = static_cast<mword>(val);
        }
};
