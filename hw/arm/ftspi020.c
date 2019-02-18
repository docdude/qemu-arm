/*
 * Faraday FTSPI020 Flash Controller
 *
 * Copyright (c) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This code is licensed under GNU GPL v2+.
 */

#include "hw/hw.h"
#include "hw/sysbus.h"
#include "hw/ssi.h"
#include "sysemu/sysemu.h"

#include "hw/ftspi020.h"

#define TYPE_FTSPI020   "ftspi020"

typedef struct Ftspi020State {
    /*< private >*/
    SysBusDevice parent;

    /*< public >*/
    MemoryRegion iomem;
    qemu_irq irq;

    /* DMA hardware handshake */
    qemu_irq req;

    SSIBus *spi;
    qemu_irq *cs_lines;

    int wip;    /* SPI Flash Status: Write In Progress BIT shift */

    /* HW register caches */
    uint32_t cmd[4];
    uint32_t ctrl;
    uint32_t timing;
    uint32_t icr;
    uint32_t isr;
    uint32_t rdsr;
} Ftspi020State;

#define FTSPI020(obj) \
    OBJECT_CHECK(Ftspi020State, obj, TYPE_FTSPI020)

static void ftspi020_update_irq(Ftspi020State *s)
{
    qemu_set_irq(s->irq, s->isr ? 1 : 0);
}

static void ftspi020_handle_ack(void *opaque, int line, int level)
{
    Ftspi020State *s = FTSPI020(opaque);

    if (!(s->icr & ICR_DMA)) {
        return;
    }

    if (level) {
        qemu_set_irq(s->req, 0);
    } else if (s->cmd[2]) {
        qemu_set_irq(s->req, 1);
    }
}

static int ftspi020_do_command(Ftspi020State *s)
{
    uint32_t cs   = extract32(s->cmd[3],  8, 2);
    uint32_t cmd  = extract32(s->cmd[3], 24, 8);
    uint32_t ilen = extract32(s->cmd[1], 24, 2);
    uint32_t alen = extract32(s->cmd[1],  0, 3);
    uint32_t dcyc = extract32(s->cmd[1], 16, 8);

    if (dcyc % 8) {
        fprintf(stderr, "ftspi020: bad dummy clock (%u) to QEMU\n", dcyc);
        abort();
    }

    /* activate the spi flash */
    qemu_set_irq(s->cs_lines[cs], 0);

    /* if it's a SPI flash READ_STATUS command */
    if ((s->cmd[3] & (CMD3_RDSR | CMD3_WRITE)) == CMD3_RDSR) {
        uint32_t rdsr;

        ssi_transfer(s->spi, cmd);
        do {
            rdsr = ssi_transfer(s->spi, 0x00);
            if (s->cmd[3] & CMD3_RDSR_SW) {
                break;
            }
        } while (rdsr & (1 << s->wip));
        s->rdsr = rdsr;
    } else {
    /* otherwise */
        int i;

        ilen = MIN(ilen, 2);
        alen = MIN(alen, 4);

        /* command cycles */
        for (i = 0; i < ilen; ++i) {
            ssi_transfer(s->spi, cmd);
        }
        /* address cycles */
        for (i = alen - 1; i >= 0; --i) {
            ssi_transfer(s->spi, extract32(s->cmd[0], i * 8, 8));
        }
        /* dummy cycles */
        for (i = 0; i < (dcyc >> 3); ++i) {
            ssi_transfer(s->spi, 0x00);
        }
    }

    if (!s->cmd[2]) {
        qemu_set_irq(s->cs_lines[cs], 1);
    } else if (s->icr & ICR_DMA) {
        qemu_set_irq(s->req, 1);
    }

    if (s->cmd[3] & CMD3_INTR) {
        s->isr |= ISR_CMDFIN;
    }
    ftspi020_update_irq(s);

    return 0;
}

static void ftspi020_chip_reset(Ftspi020State *s)
{
    int i;

    for (i = 0; i < 4; ++i) {
        s->cmd[i] = 0;
    }
    s->wip = 0;
    s->ctrl = 0;
    s->timing = 0;
    s->icr = 0;
    s->isr = 0;
    s->rdsr = 0;

    qemu_set_irq(s->irq, 0);

    for (i = 0; i < CFG_NR_CSLINES; ++i) {
        /* de-activate the spi flash */
        qemu_set_irq(s->cs_lines[i], 1);
    }
}

static uint64_t
ftspi020_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    Ftspi020State *s = FTSPI020(opaque);
    uint64_t ret = 0;

    switch (addr) {
    case REG_CMD0 ... REG_CMD3:
        return s->cmd[(addr - REG_CMD0) / 4];
    case REG_CR:
        return s->ctrl;
    case REG_TR:
        return s->timing;
    case REG_SR:
        /* In QEMU, the data fifo is always ready for read/write */
        return SR_RX_READY | SR_TX_READY;
    case REG_ISR:
        return s->isr;
    case REG_ICR:
        return s->icr;
    case REG_RDSR:
        return s->rdsr;
    case REG_REVR:
        return 0x00010001;  /* rev. 1.0.1 */
    case REG_FEAR:
        return FEAR_CLKM_SYNC
            | FEAR_CMDQ(2) | FEAR_RXFIFO(32) | FEAR_TXFIFO(32);
    case REG_DR:
        if (!(s->cmd[3] & CMD3_WRITE)) {
            int i;
            uint32_t cs = extract32(s->cmd[3], 8, 2);
            for (i = 0; i < 4 && s->cmd[2]; i++, s->cmd[2]--) {
                ret = deposit32(ret, i * 8, 8,
                    ssi_transfer(s->spi, 0x00) & 0xff);
            }
            if (!s->cmd[2]) {
                qemu_set_irq(s->cs_lines[cs], 1);
                if (s->cmd[3] & CMD3_INTR) {
                    s->isr |= ISR_CMDFIN;
                }
                ftspi020_update_irq(s);
            }
        }
        break;
        /* we don't care */
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftspi020: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }

    return ret;
}

static void
ftspi020_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    Ftspi020State *s = FTSPI020(opaque);

    switch (addr) {
    case REG_CMD0 ... REG_CMD2:
        s->cmd[(addr - REG_CMD0) / 4] = (uint32_t)val;
        break;
    case REG_CMD3:
        s->cmd[3] = (uint32_t)val;
        ftspi020_do_command(s);
        break;
    case REG_CR:
        if (val & CR_ABORT) {
            ftspi020_chip_reset(s);
            val &= ~CR_ABORT;
        }
        s->ctrl = (uint32_t)val;
        s->wip  = extract32(val, 16, 3);
        break;
    case REG_TR:
        s->timing = (uint32_t)val;
        break;
    case REG_DR:
        if (s->cmd[3] & CMD3_WRITE) {
            int i;
            uint32_t cs = extract32(s->cmd[3], 8, 2);
            for (i = 0; i < 4 && s->cmd[2]; i++, s->cmd[2]--) {
                ssi_transfer(s->spi, extract32((uint32_t)val, i * 8, 8));
            }
            if (!s->cmd[2]) {
                qemu_set_irq(s->cs_lines[cs], 1);
                if (s->cmd[3] & CMD3_INTR) {
                    s->isr |= ISR_CMDFIN;
                }
                ftspi020_update_irq(s);
            }
        }
        break;
    case REG_ISR:
        s->isr &= ~((uint32_t)val);
        ftspi020_update_irq(s);
        break;
    case REG_ICR:
        s->icr = (uint32_t)val;
        break;
        /* we don't care */
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftspi020: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }
}

static const MemoryRegionOps mmio_ops = {
    .read  = ftspi020_mem_read,
    .write = ftspi020_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    }
};

static void ftspi020_reset(DeviceState *ds)
{
    Ftspi020State *s = FTSPI020(SYS_BUS_DEVICE(ds));

    ftspi020_chip_reset(s);
}

static void ftspi020_realize(DeviceState *dev, Error **errp)
{
    Ftspi020State *s = FTSPI020(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    int i;

    memory_region_init_io(&s->iomem,
			  OBJECT(s),
                          &mmio_ops,
                          s,
                          TYPE_FTSPI020,
                          0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);

    s->spi = ssi_create_bus(dev, "spi");
    s->cs_lines = g_new0(qemu_irq, CFG_NR_CSLINES);
    ssi_auto_connect_slaves(dev, s->cs_lines, s->spi);
    for (i = 0; i < CFG_NR_CSLINES; ++i) {
        sysbus_init_irq(sbd, &s->cs_lines[i]);
    }

    qdev_init_gpio_in(dev, ftspi020_handle_ack, 1);
    qdev_init_gpio_out(dev, &s->req, 1);
}

static const VMStateDescription vmstate_ftspi020 = {
    .name = TYPE_FTSPI020,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(cmd, Ftspi020State, 4),
        VMSTATE_UINT32(ctrl, Ftspi020State),
        VMSTATE_UINT32(timing, Ftspi020State),
        VMSTATE_UINT32(icr, Ftspi020State),
        VMSTATE_UINT32(isr, Ftspi020State),
        VMSTATE_UINT32(rdsr, Ftspi020State),
        VMSTATE_END_OF_LIST(),
    }
};

static void ftspi020_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd    = &vmstate_ftspi020;
    dc->reset   = ftspi020_reset;
    dc->realize = ftspi020_realize;
//    dc->no_user = 1;
}

static const TypeInfo ftspi020_info = {
    .name          = TYPE_FTSPI020,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Ftspi020State),
    .class_init    = ftspi020_class_init,
};

static void ftspi020_register_types(void)
{
    type_register_static(&ftspi020_info);
}

type_init(ftspi020_register_types)
