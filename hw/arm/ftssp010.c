/*
 * QEMU model of the FTSSP010 Controller
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 */

#include "hw/sysbus.h"
#include "hw/i2c/i2c.h"
#include "hw/ssi.h"
#include "hw/audio.h"
#include "sysemu/sysemu.h"
#include "qemu/fifo8.h"

#include "hw/arm/faraday.h"
#include "hw/ftssp010.h"

#define CFG_FIFO_DEPTH  16

#define TYPE_FTSSP010   "ftssp010"

typedef struct Ftssp010State {
    /*< private >*/
    SysBusDevice parent;

    /*< public >*/
    MemoryRegion mmio;

    qemu_irq irq;
    SSIBus *spi;
    AudioCodecState *codec;

    uint8_t num_cs;
    qemu_irq *cs_lines;

    Fifo8 rx_fifo;
    Fifo8 tx_fifo;

    uint8_t tx_thres;
    uint8_t rx_thres;

    int busy;
    uint8_t bw;

    /* DMA hardware handshake */
    qemu_irq req[2];    /* 0 - Tx, 1 - Rx */

    /* HW register caches */

    uint32_t cr0;
    uint32_t cr1;
    uint32_t cr2;
    uint32_t icr;    /* interrupt control register */
    uint32_t isr;    /* interrupt status register */

} Ftssp010State;

#define FTSSP010(obj) \
    OBJECT_CHECK(Ftssp010State, obj, TYPE_FTSSP010)

/* Update interrupts.  */
static void ftssp010_update(Ftssp010State *s)
{
    if (!(s->cr2 & CR2_SSPEN)) {
        return;
    }

    /* tx fifo status update */
    if ((s->tx_fifo.num / (s->bw >> 3)) <= s->tx_thres) {
        s->isr |=  ISR_TFTHI;
        if (s->icr & ICR_TFDMA) {
            qemu_set_irq(s->req[0], 1);
        }
    } else {
        s->isr &= ~ISR_TFTHI;
    }

    /* rx fifo status update */
    switch (s->cr0 & CR0_FFMT_MASK) {
    case CR0_FFMT_SPI:
        s->isr |=  ISR_RFTHI;
        if (s->icr & ICR_RFDMA) {
            qemu_set_irq(s->req[1], 1);
        }
        break;
    default:
        if ((s->rx_fifo.num / (s->bw >> 3)) >= s->rx_thres) {
            s->isr |=  ISR_RFTHI;
            if (s->icr & ICR_RFDMA) {
                qemu_set_irq(s->req[1], 1);
            }
        } else {
            s->isr &= ~ISR_RFTHI;
        }
        break;
    }

    /* update the interrupt signal */
    qemu_set_irq(s->irq, (s->icr & s->isr) ? 1 : 0);
}

static void ftssp010_handle_ack(void *opaque, int line, int level)
{
    Ftssp010State *s = FTSSP010(opaque);

    switch (line) {
    case 0:    /* Tx */
        if (s->icr & ICR_TFDMA) {
            if (level) {
                qemu_set_irq(s->req[0], 0);
            } else if ((s->tx_fifo.num / (s->bw >> 3)) <= s->tx_thres) {
                qemu_set_irq(s->req[0], 1);
            }
        }
        break;
    case 1:    /* Rx */
        if (s->icr & ICR_RFDMA) {
            if (level) {
                qemu_set_irq(s->req[1], 0);
            } else {
                switch (s->cr0 & CR0_FFMT_MASK) {
                case CR0_FFMT_SPI:
                    qemu_set_irq(s->req[1], 1);
                    break;
                default:
                    if ((s->rx_fifo.num / (s->bw >> 3)) >= s->rx_thres) {
                        qemu_set_irq(s->req[1], 1);
                    }
                    break;
                }
            }
        }
        break;
    default:
        break;
    }
}

void ftssp010_i2s_data_req(void *opaque, int tx, int rx)
{
    int i, len;
    uint32_t sample;
    Ftssp010State *s = FTSSP010(opaque);

    if (!(s->cr2 & CR2_SSPEN)) {
        return;
    }

    if ((s->cr0 & CR0_FFMT_MASK) != CR0_FFMT_I2S) {
        return;
    }

    s->busy = 1;

    if ((s->cr2 & CR2_TXEN) && (s->cr2 & CR2_TXDOE)) {
        len = tx * (s->bw / 8);
        while (!fifo8_is_empty(&s->tx_fifo) && len > 0) {
            sample = 0;
            for (i = 0; i < (s->bw >> 3) && len > 0; i++, len--) {
                sample = deposit32(sample, i * 8, 8, fifo8_pop(&s->tx_fifo));
            }
            audio_codec_dac_dat(s->codec, sample);
        }

        if (fifo8_is_empty(&s->tx_fifo) && len > 0) {
            s->isr |= ISR_TFURI;
        }
    }

    if (s->cr2 & CR2_RXEN) {
        len = rx * (s->bw / 8);
        while (!fifo8_is_full(&s->rx_fifo) && len > 0) {
            sample = audio_codec_adc_dat(s->codec);
            for (i = 0; i < (s->bw >> 3) && len > 0; i++, len--) {
                fifo8_push(&s->rx_fifo, extract32(sample, i * 8, 8));
            }
        }

        if (fifo8_is_full(&s->rx_fifo) && len > 0) {
            s->isr |= ISR_RFORI;
        }
    }

    s->busy = 0;

    ftssp010_update(s);
}

static void ftssp010_spi_tx(Ftssp010State *s)
{
    if (!(s->cr2 & CR2_TXEN)) {
        return;
    }

    s->busy = 1;

    if (fifo8_is_empty(&s->tx_fifo)) {
        s->isr |= ISR_TFURI;
    }

    while (!fifo8_is_empty(&s->tx_fifo)) {
        ssi_transfer(s->spi, fifo8_pop(&s->tx_fifo));
    }

    s->busy = 0;
}

static uint64_t
ftssp010_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    Ftssp010State *s = FTSSP010(opaque);
    uint32_t i, ret = 0;

    if (addr & 0x03) {
        fprintf(stderr, "ftssp010: "
                 "Although ftssp010 supports byte/half-word access, "
                 "the target address still needs to be word aligned\n");
        abort();
    }

    switch (addr) {
    case REG_CR0:    /* Control Register 0 */
        return s->cr0;
    case REG_CR1:    /* Control Register 1 */
        return s->cr1;
    case ReG_CR2:    /* Control Register 2 */
        return s->cr2;
    case REG_SR:    /* Status Register */
        ret |= s->busy ? SR_BUSY : 0;
        /* tx fifo status */
        ret |= SR_TFVE(s->tx_fifo.num / (s->bw >> 3));
        if (!fifo8_is_full(&s->tx_fifo)) {
            ret |= SR_TFNF;
        }
        /* rx fifo status */
        switch (s->cr0 & CR0_FFMT_MASK) {
        case CR0_FFMT_SPI:
            ret |= SR_RFF | SR_RFVE(CFG_FIFO_DEPTH);
            break;
        case CR0_FFMT_I2S:
            ret |= SR_RFVE(s->rx_fifo.num / (s->bw >> 3));
            if (fifo8_is_full(&s->rx_fifo)) {
                ret |= SR_RFF;
            }
            break;
        default:
            break;
        }
        break;
    case REG_ICR:    /* Interrupt Control Register */
        return s->icr;
    case REG_ISR:    /* Interrupt Status Register */
        ret = s->isr;
        s->isr &= ~ISR_RCMASK;
        ftssp010_update(s);
        break;
    case REG_DR:    /* Data Register */
        if (!(s->cr2 & CR2_SSPEN)) {
            break;
        }
        if (!(s->cr2 & CR2_RXEN)) {
            break;
        }
        switch (s->cr0 & CR0_FFMT_MASK) {
        case CR0_FFMT_SPI:
            for (i = 0; i < (s->bw >> 3); i++) {
                ret = deposit32(ret, i * 8, 8, ssi_transfer(s->spi, 0));
            }
            break;
        case CR0_FFMT_I2S:
            for (i = 0; i < (s->bw >> 3); i++) {
                if (fifo8_is_empty(&s->rx_fifo)) {
                    break;
                }
                ret = deposit32(ret, i * 8, 8, fifo8_pop(&s->rx_fifo));
            }
            break;
        default:
            break;
        }
        ftssp010_update(s);
        break;
    case REG_REVR:
        return 0x00011901;    /* ver. 1.19.1 */
    case REG_FEAR:
        return 0x660f0f1f;    /* SPI+I2S, FIFO=16 */
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftssp010: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }

    return ret;
}

static void
ftssp010_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    int i;
    Ftssp010State *s = FTSSP010(opaque);

    if (addr & 0x03) {
        fprintf(stderr, "ftssp010: "
                 "Although ftssp010 supports byte/half-word access, "
                 "the target address still needs to be word aligned\n");
        abort();
    }

    switch (addr) {
    case REG_CR0:    /* Control Register 0 */
        s->cr0 = (uint32_t)val;
        break;
    case REG_CR1:    /* Control Register 1 */
        s->cr1 = (uint32_t)val;
        s->bw  = extract32(s->cr1, 16, 7) + 1;
        break;
    case ReG_CR2:    /* Control Register 2 */
        s->cr2 = (uint32_t)val;
        if (s->cr2 & CR2_SSPRST) {
            fifo8_reset(&s->tx_fifo);
            fifo8_reset(&s->rx_fifo);
            s->busy = 0;
            s->cr2 &= ~(CR2_SSPRST | CR2_TXFCLR | CR2_RXFCLR);
            if (s->cr0 & CR0_FLASH) {
                int cs = extract32(s->cr2, 10, 2);
                qemu_set_irq(s->cs_lines[cs], 1);
                s->cr2 |= CR2_FS;
            };
        }
        if (s->cr2 & CR2_TXFCLR) {
            fifo8_reset(&s->tx_fifo);
            s->cr2 &= ~CR2_TXFCLR;
        }
        if (s->cr2 & CR2_RXFCLR) {
            fifo8_reset(&s->rx_fifo);
            s->cr2 &= ~CR2_RXFCLR;
        }
        if (s->cr0 & CR0_FLASH) {
            int cs = extract32(s->cr2, 10, 2);
            qemu_set_irq(s->cs_lines[cs], (s->cr2 & CR2_FS) ? 1 : 0);
        }
        if (s->cr2 & CR2_SSPEN) {
            switch (s->cr0 & CR0_FFMT_MASK) {
            case CR0_FFMT_SPI:
                ftssp010_spi_tx(s);
                break;
            default:
                break;
            }
        }
        ftssp010_update(s);
        break;
    case REG_ICR:    /* Interrupt Control Register */
        s->icr = (uint32_t)val;
        s->tx_thres = extract32(s->icr, 12, 5);
        s->rx_thres = extract32(s->icr,  7, 5);
        break;
    case REG_DR:    /* Data Register */
        if (!(s->cr2 & CR2_SSPEN)) {
            break;
        }
        for (i = 0; i < (s->bw >> 3) && !fifo8_is_full(&s->tx_fifo); i++) {
            fifo8_push(&s->tx_fifo, extract32((uint32_t)val, i * 8, 8));
        }
        switch (s->cr0 & CR0_FFMT_MASK) {
        case CR0_FFMT_SPI:
            ftssp010_spi_tx(s);
            break;
        default:
            break;
        }
        ftssp010_update(s);
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftssp010: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }
}

static const MemoryRegionOps mmio_ops = {
    .read  = ftssp010_mem_read,
    .write = ftssp010_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 4
    }
};

static void ftssp010_reset(DeviceState *ds)
{
    Ftssp010State *s = FTSSP010(SYS_BUS_DEVICE(ds));
    Error *local_errp = NULL;

    s->codec = AUDIO_CODEC(object_property_get_link(OBJECT(s),
                            "codec", &local_errp));
    if (local_errp) {
        fprintf(stderr, "ftssp010: Unable to get codec link\n");
        abort();
    }

    s->busy = 0;
    s->bw   = 8;    /* 8-bit */

    s->cr0 = 0;
    s->cr1 = 0;
    s->cr2 = 0;
    s->tx_thres = 4;
    s->rx_thres = 4;
    s->icr = ICR_TFTHOD(4) | ICR_RFTHOD(4);
    s->isr = ISR_TFTHI;

    fifo8_reset(&s->tx_fifo);
    fifo8_reset(&s->rx_fifo);

    ftssp010_update(s);
}

static void ftssp010_realize(DeviceState *dev, Error **errp)
{
    Ftssp010State *s = FTSSP010(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    int i;

    s->spi = ssi_create_bus(dev, "spi");

    fifo8_create(&s->tx_fifo, CFG_FIFO_DEPTH * 4);
    fifo8_create(&s->rx_fifo, CFG_FIFO_DEPTH * 4);

    memory_region_init_io(&s->mmio,
			  OBJECT(s),
                          &mmio_ops,
                          s,
                          TYPE_FTSSP010,
                          0x1000);
    sysbus_init_mmio(sbd, &s->mmio);
    sysbus_init_irq(sbd, &s->irq);

    s->num_cs = 4;
    s->cs_lines = g_new(qemu_irq, s->num_cs);
    ssi_auto_connect_slaves(dev, s->cs_lines, s->spi);
    for (i = 0; i < s->num_cs; ++i) {
        sysbus_init_irq(sbd, &s->cs_lines[i]);
    }

    /* DMA hardware handshake */
    qdev_init_gpio_in(dev, ftssp010_handle_ack, 2);
    qdev_init_gpio_out(dev, s->req, 2);
}

static const VMStateDescription vmstate_ftssp010 = {
    .name = TYPE_FTSSP010,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(cr0, Ftssp010State),
        VMSTATE_UINT32(cr1, Ftssp010State),
        VMSTATE_UINT32(cr2, Ftssp010State),
        VMSTATE_UINT32(icr, Ftssp010State),
        VMSTATE_UINT32(isr, Ftssp010State),
        VMSTATE_FIFO8(tx_fifo, Ftssp010State),
        VMSTATE_FIFO8(rx_fifo, Ftssp010State),
        VMSTATE_END_OF_LIST()
    }
};

static void ftssp010_instance_init(Object *obj)
{
    Ftssp010State *s = FTSSP010(obj);

    object_property_add_link(obj,
                             "codec",
                             TYPE_AUDIO_CODEC,
                             (Object **) &s->codec,
                             object_property_allow_set_link,
                             OBJ_PROP_LINK_UNREF_ON_RELEASE,
                             &error_abort);
}

static void ftssp010_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd    = &vmstate_ftssp010;
    dc->reset   = ftssp010_reset;
    dc->realize = ftssp010_realize;
//    dc->no_user = 1;
}

static const TypeInfo ftssp010_info = {
    .name          = TYPE_FTSSP010,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Ftssp010State),
    .instance_init = ftssp010_instance_init,
    .class_init    = ftssp010_class_init,
};

static void ftssp010_register_types(void)
{
    type_register_static(&ftssp010_info);
}

type_init(ftssp010_register_types)
