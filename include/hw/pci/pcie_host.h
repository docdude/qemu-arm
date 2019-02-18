/*
 * pcie_host.h
 *
 * Copyright (c) 2009 Isaku Yamahata <yamahata at valinux co jp>
 *                    VA Linux Systems Japan K.K.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PCIE_HOST_H
#define PCIE_HOST_H

#include "hw/pci/pci_host.h"
#include "exec/memory.h"

#define TYPE_PCIE_HOST_BRIDGE "pcie-host-bridge"
#define PCIE_HOST_BRIDGE(obj) \
    OBJECT_CHECK(PCIExpressHost, (obj), TYPE_PCIE_HOST_BRIDGE)

#define PCIE_HOST_MCFG_BASE "MCFG"
#define PCIE_HOST_MCFG_SIZE "mcfg_size"

/* pcie_host::base_addr == PCIE_BASE_ADDR_UNMAPPED when it isn't mapped. */
#define PCIE_BASE_ADDR_UNMAPPED  ((hwaddr)-1ULL)

struct PCIExpressHost {
    PCIHostState pci;

    /* express part */

    /* base address where MMCONFIG area is mapped. */
    hwaddr  base_addr;

    /* the size of MMCONFIG area. It's host bridge dependent */
    hwaddr  size;

    /* MMCONFIG mmio area */
    MemoryRegion mmio;
};

int pcie_host_init(PCIExpressHost *e);
void pcie_host_mmcfg_unmap(PCIExpressHost *e);
void pcie_host_mmcfg_map(PCIExpressHost *e, hwaddr addr, uint32_t size);
void pcie_host_mmcfg_update(PCIExpressHost *e,
                            int enable,
                            hwaddr addr,
                            uint32_t size);
void pcie_mmcfg_data_write(void *opaque, hwaddr mmcfg_addr, uint64_t val, unsigned len);
uint64_t pcie_mmcfg_data_read(void *opaque, hwaddr mmcfg_addr, unsigned len);

/*
 * PCI express ECAM (Enhanced Configuration Address Mapping) format.
 * AKA mmcfg address
 * bit 20 - 28: bus number
 * bit 15 - 19: device number
 * bit 12 - 14: function number
 * bit  0 - 11: offset in configuration space of a given device
 */
#define PCIE_MMCFG_SIZE_MAX             (1ULL << 28)
#define PCIE_MMCFG_SIZE_MIN             (1ULL << 20)
#define PCIE_MMCFG_BUS_BIT              20
#define PCIE_MMCFG_BUS_MASK             0x1ff
#define PCIE_MMCFG_DEVFN_BIT            12
#define PCIE_MMCFG_DEVFN_MASK           0xff
#define PCIE_MMCFG_CONFOFFSET_MASK      0xfff
#define PCIE_MMCFG_BUS(addr)            (((addr) >> PCIE_MMCFG_BUS_BIT) & \
                                         PCIE_MMCFG_BUS_MASK)
#define PCIE_MMCFG_DEVFN(addr)          (((addr) >> PCIE_MMCFG_DEVFN_BIT) & \
                                         PCIE_MMCFG_DEVFN_MASK)
#define PCIE_MMCFG_CONFOFFSET(addr)     ((addr) & PCIE_MMCFG_CONFOFFSET_MASK)


/* PCIE Config space accessing MACROS ---Broadcom*/

#define	PCIECFG_BUS_SHIFT	24	/* Bus shift */
#define	PCIECFG_SLOT_SHIFT	19	/* Slot/Device shift */
#define	PCIECFG_FUN_SHIFT	16	/* Function shift */
#define	PCIECFG_OFF_SHIFT	0	/* Register shift */

#define	PCIECFG_BUS_MASK	0xff	/* Bus mask */
#define	PCIECFG_SLOT_MASK	0x1f	/* Slot/Device mask */
#define	PCIECFG_FUN_MASK	7	/* Function mask */
#define	PCIECFG_OFF_MASK	0xfff	/* Register mask */

#define	PCIE_CONFIG_ADDR(b, s, f, o)					\
		((((b) & PCIECFG_BUS_MASK) << PCIECFG_BUS_SHIFT)		\
		 | (((s) & PCIECFG_SLOT_MASK) << PCIECFG_SLOT_SHIFT)	\
		 | (((f) & PCIECFG_FUN_MASK) << PCIECFG_FUN_SHIFT)	\
		 | (((o) & PCIECFG_OFF_MASK) << PCIECFG_OFF_SHIFT))

#define	PCIE_CONFIG_BUS(a)	(((a) >> PCIECFG_BUS_SHIFT) & PCIECFG_BUS_MASK)
#define	PCIE_CONFIG_SLOT(a)	(((a) >> PCIECFG_SLOT_SHIFT) & PCIECFG_SLOT_MASK)
#define	PCIE_CONFIG_FUN(a)	(((a) >> PCIECFG_FUN_SHIFT) & PCIECFG_FUN_MASK)
#define	PCIE_CONFIG_OFF(a)	(((a) >> PCIECFG_OFF_SHIFT) & PCIECFG_OFF_MASK)

#endif /* PCIE_HOST_H */
