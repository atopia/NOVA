/*
 * Advanced Configuration and Power Interface (ACPI)
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

#include "acpi_gas.hpp"

class Acpi
{
    friend class Acpi_table_fadt;
    friend class Acpi_table_rsdt;
    friend class Acpi_rsdp;

    private:
        enum Register
        {
            PM1_STS,
            PM1_ENA,
            PM1_CNT,
            PM2_CNT,
            GPE0_STS,
            GPE0_ENA,
            GPE1_STS,
            GPE1_ENA,
            PM_TMR,
            RESET
        };

        enum PM1_Status
        {
            PM1_STS_TMR         = 1U << 0,      // 0x1
            PM1_STS_BM          = 1U << 4,      // 0x10
            PM1_STS_GBL         = 1U << 5,      // 0x20
            PM1_STS_PWRBTN      = 1U << 8,      // 0x100
            PM1_STS_SLPBTN      = 1U << 9,      // 0x200
            PM1_STS_RTC         = 1U << 10,     // 0x400
            PM1_STS_PCIE_WAKE   = 1U << 14,     // 0x4000
            PM1_STS_WAKE        = 1U << 15      // 0x8000
        };

        enum PM1_Enable
        {
            PM1_ENA_TMR         = 1U << 0,      // 0x1
            PM1_ENA_GBL         = 1U << 5,      // 0x20
            PM1_ENA_PWRBTN      = 1U << 8,      // 0x100
            PM1_ENA_SLPBTN      = 1U << 9,      // 0x200
            PM1_ENA_RTC         = 1U << 10,     // 0x400
            PM1_ENA_PCIE_WAKE   = 1U << 14      // 0x4000
        };

        enum PM1_Control
        {
            PM1_CNT_SLP_MASK    = 7,
            PM1_CNT_SLP_SHIFT   = 10,

            PM1_CNT_SCI_EN      = 1U << 0,      // 0x1
            PM1_CNT_BM_RLD      = 1U << 1,      // 0x2
            PM1_CNT_GBL_RLS     = 1U << 2,      // 0x4
            PM1_CNT_SLP_EN      = 1U << 13      // 0x2000
        };

        static unsigned const timer_frequency = 3579545;

        static Paddr dmar, fadt, facs, hpet, madt, mcfg, rsdt, xsdt, ivrs;

        static Acpi_gas pm1a_sts;
        static Acpi_gas pm1b_sts;
        static Acpi_gas pm1a_ena;
        static Acpi_gas pm1b_ena;
        static Acpi_gas pm1a_cnt;
        static Acpi_gas pm1b_cnt;
        static Acpi_gas pm2_cnt;
        static Acpi_gas pm_tmr;
        static Acpi_gas gpe0_sts;
        static Acpi_gas gpe1_sts;
        static Acpi_gas gpe0_ena;
        static Acpi_gas gpe1_ena;
        static Acpi_gas reset_reg;

        static uint32   feature;
        static uint8    reset_val;

        static unsigned hw_read (Acpi_gas *);
        static unsigned read (Register);

        static void hw_write (Acpi_gas *, unsigned, bool = false);
        static void write (Register, unsigned);
        static void clear (Register);

    public:
        static unsigned irq;
        static unsigned gsi;

        static uint64   resume_time;

        ALWAYS_INLINE
        static inline Paddr p_rsdt() { return rsdt; }

        ALWAYS_INLINE
        static inline Paddr p_xsdt() { return xsdt; }

        static void delay (unsigned);
        static void reset();
        static bool suspend (uint8, uint8);

        INIT
        static void setup();

        INIT
        static void init();
};
