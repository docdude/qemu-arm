/*
 * Faraday A369 SoC
 *
 * Copyright (c) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This code is licensed under GNU GPL v2+.
 */

#include "hw/sysbus.h"
//#include "hw/arm-misc.h"
#include "hw/devices.h"
#include "hw/i2c/i2c.h"
#include "hw/boards.h"
#include "hw/block/flash.h"
#include "hw/char/serial.h"
#include "hw/ssi.h"
#include "net/net.h"
#include "sysemu/sysemu.h"
#include "sysemu/blockdev.h"

#include "hw/arm/faraday.h"

static void a369soc_reset(DeviceState *ds)
{
    int i;
    FaradaySoCState *s = FARADAY_SOC(SYS_BUS_DEVICE(ds));

    /* AHB slave base & window configuration */
    memset(s->ahb_slave, 0, sizeof(s->ahb_slave));
    s->ahb_slave[0] = 0x94050000;
    s->ahb_slave[1] = 0x96040000;
    s->ahb_slave[2] = 0x90f00000;
    s->ahb_slave[3] = 0x92050000; /* APB: base=0x92000000, size=32MB */
    s->ahb_slave[5] = 0xc0080000;
    s->ahb_slave[4] = 0x00080000; /* ROM: base=0x00000000, size=256MB */
    s->ahb_slave[6] = 0x10090000; /* RAM: base=0x10000000, size=512MB */
    for (i = 0; i < 15; ++i) {
        s->ahb_slave[7 + i] = 0x90000000 + (i << 20);
    }
    s->ahb_slave[22] = 0x40080000;
    s->ahb_slave[23] = 0x60080000;
    s->ahb_slave[24] = 0xa0000000; /* SRAM: base=0xA0000000, size=1MB */

    /* APB slave base & window configuration */
    memset(s->apb_slave, 0, sizeof(s->apb_slave));
    for (i = 0; i < 18; ++i) {
        s->apb_slave[i] = 0x12000000 + (i << 20);
    }
}

static void a369soc_chip_init(FaradaySoCState *s)
{
    int i;

    DeviceState *ds;
    DriveInfo *dinfo;
    qemu_irq ack, req;
    Error *local_errp = NULL;
//s = (FaradaySoCState *) g_malloc0(sizeof(FaradaySoCState));
    /* Remappable Memory Region Init */
    s->rmr = g_new0(MemoryRegion, 1);
    memory_region_init(s->rmr, NULL, "a369soc.rmr", 0x80000000);
    memory_region_add_subregion(s->as, 0x00000000, s->rmr);

    /* Embedded SRAM Init */
    s->sram = g_new0(MemoryRegion, 1);
    memory_region_init_ram(s->sram, NULL, "a369soc.sram", 0x4000);
    vmstate_register_ram_global(s->sram);
    memory_region_add_subregion(s->as, 0xA0000000, s->sram);
//MemoryRegion *rom_test = g_new0(MemoryRegion, 1);
    /* Interrupt Controller */
    ds = sysbus_create_varargs("ftintc020", 0x90100000,
                    qdev_get_gpio_in(DEVICE(s->cpu), ARM_CPU_IRQ),
                    qdev_get_gpio_in(DEVICE(s->cpu), ARM_CPU_FIQ),
                    NULL);
    for (i = 0; i < 64; ++i) {
        s->pic[i] = qdev_get_gpio_in(ds, i);
    }
    /* Embedded ROM Init (Emulated with a parallel NOR flash) */
    dinfo = drive_get_next(IF_PFLASH);
    s->rom = pflash_cfi01_register(
                    (hwaddr)-1,
                    NULL,
                    "a369soc.rom",
                    6 << 10,
                    dinfo ? dinfo->bdrv : NULL,
                    2048,               /* 1 KB sector */
                    6,                  /* sectors per chip */
                    4,                  /* 32 bits */
                    0, 0, 0, 0,         /* id */
                    0                   /* Little Endian */);
    if (!s->rom) {
        fprintf(stderr, "a369soc: Unable to init ROM device.\n");
        abort();
    }
//sysbus_mmio_map(SYS_BUS_DEVICE(s->rom), 0, 0x00000000);
//rom_test=pflash_cfi01_get_memory(s->rom);
   memory_region_add_subregion(s->rmr, s->rom_base,
                sysbus_mmio_get_region(SYS_BUS_DEVICE(s->rom), 0));
//memory_region_add_subregion(s->rmr, s->rom_base,
 //rom_test);

//sysbus_init_mmio(SYS_BUS_DEVICE(s->rom), s->rmr);
//printf("a369soc******: cpu %s.\n",(char *)s->cpu);

#if 1
    /* Serial (FTUART010 which is 16550A compatible) */
    if (serial_hds[0]) {
        serial_mm_init(s->as,
                       0x92b00000,
                       2,
                       s->pic[53],
                       18432000,
                       serial_hds[0],
                       DEVICE_LITTLE_ENDIAN);
    }
    if (serial_hds[1]) {
        serial_mm_init(s->as,
                       0x92c00000,
                       2,
                       s->pic[54],
                       18432000,
                       serial_hds[1],
                       DEVICE_LITTLE_ENDIAN);
    }
#endif
    /* ftscu010 */
    sysbus_create_simple("a369-scu", 0x92000000, NULL);

    /* ftkbc010 */
    sysbus_create_simple("a369-kpd", 0x92f00000, s->pic[21]);

    /* ftahbc020 */
    ds = sysbus_create_simple("ftahbc020", 0x94000000, NULL);
    object_property_set_link(OBJECT(ds), OBJECT(s), "soc", &local_errp);
    if (local_errp) {
        fprintf(stderr, "a369soc: Unable to set soc link for FTAHBC020\n");
        abort();
    }

    /* ftddrii030 */
    ds = sysbus_create_simple("ftddrii030", 0x93100000, NULL);
    object_property_set_link(OBJECT(ds), OBJECT(s), "soc", &local_errp);
    if (local_errp) {
        fprintf(stderr, "a369soc: Unable to set soc link for FTDDRII030\n");
        abort();
    }

    /* Timer */
    ds = qdev_create(NULL, "ftpwmtmr010");
    qdev_prop_set_uint32(ds, "freq", 66 * 1000000);
    qdev_init_nofail(ds);
    sysbus_mmio_map(SYS_BUS_DEVICE(ds), 0, 0x92300000);
    sysbus_connect_irq(SYS_BUS_DEVICE(ds), 0, s->pic[8]);
    sysbus_connect_irq(SYS_BUS_DEVICE(ds), 1, s->pic[9]);
    sysbus_connect_irq(SYS_BUS_DEVICE(ds), 2, s->pic[10]);
    sysbus_connect_irq(SYS_BUS_DEVICE(ds), 3, s->pic[11]);

    /* ftwdt010 */
    sysbus_create_simple("ftwdt010", 0x92200000, s->pic[46]);

    /* ftrtc011 */
    ds = qdev_create(NULL, "ftrtc011");
    /* Setup QOM path for QTest */
    object_property_add_child(OBJECT(s),
                              "ftrtc011",
                              OBJECT(ds),
                              NULL);
    qdev_init_nofail(ds);
    sysbus_mmio_map(SYS_BUS_DEVICE(ds), 0, 0x92100000);
    /* Alarm (Edge) */
    sysbus_connect_irq(SYS_BUS_DEVICE(ds), 1, s->pic[42]);
    /* Second (Edge) */
    sysbus_connect_irq(SYS_BUS_DEVICE(ds), 2, s->pic[43]);
    /* Minute (Edge) */
    sysbus_connect_irq(SYS_BUS_DEVICE(ds), 3, s->pic[44]);
    /* Hour (Edge) */
    sysbus_connect_irq(SYS_BUS_DEVICE(ds), 4, s->pic[45]);

    /* ftdmac020 */
    s->hdma[0] = sysbus_create_varargs("ftdmac020",
                                       0x90300000,
                                       s->pic[0],  /* ALL (NC in A369) */
                                       s->pic[15], /* TC */
                                       s->pic[16], /* ERR */
                                       NULL);
    s->hdma[1] = sysbus_create_varargs("ftdmac020",
                                       0x96100000,
                                       s->pic[0],  /* ALL (NC in A369) */
                                       s->pic[17], /* TC */
                                       s->pic[18], /* ERR */
                                       NULL);

    /* ftapbbrg020 */
    ds = sysbus_create_simple("ftapbbrg020", 0x90f00000, s->pic[14]);
    s->pdma[0] = ds;
    object_property_set_link(OBJECT(ds), OBJECT(s), "soc", &local_errp);
    if (local_errp) {
        fprintf(stderr, "a369soc: Unable to set soc link for FTAPBBRG020\n");
        abort();
    }

    /* ftnandc021 */
    ds = sysbus_create_simple("ftnandc021", 0x90200000, s->pic[30]);
    s->nandc[0] = ds;
    ack = qdev_get_gpio_in(ds, 0);
    req = qdev_get_gpio_in(s->hdma[0], 15);
    qdev_connect_gpio_out(s->hdma[0], 15, ack);
    qdev_connect_gpio_out(ds, 0, req);

    /* fti2c010 */
    ds = sysbus_create_simple("fti2c010", 0x92900000, s->pic[51]);
    s->i2c[0] = ds;
    ds = sysbus_create_simple("fti2c010", 0x92A00000, s->pic[52]);
    s->i2c[1] = ds;

    /* ftssp010 */
    ds = sysbus_create_simple("ftssp010", 0x92700000, s->pic[49]);
    s->spi[0] = ds;
    s->i2s[0] = ds;

    /* ftssp010 - DMA (Tx) */
    ack = qdev_get_gpio_in(ds, 0);
    req = qdev_get_gpio_in(s->pdma[0], 7);
    qdev_connect_gpio_out(s->pdma[0], 7, ack);
    qdev_connect_gpio_out(ds, 0, req);

    /* ftssp010 - DMA (Rx) */
    ack = qdev_get_gpio_in(ds, 1);
    req = qdev_get_gpio_in(s->pdma[0], 8);
    qdev_connect_gpio_out(s->pdma[0], 8, ack);
    qdev_connect_gpio_out(ds, 1, req);

    /* ftgmac100 */
    if (nb_nics > 0) {
        ftgmac100_init(&nd_table[0], 0x90c00000, s->pic[32]);
    }

    /* ftlcdc200 */
    sysbus_create_varargs("ftlcdc200",
                          0x94a00000,
                          s->pic[0],  /* ALL (NC in A369) */
                          s->pic[25], /* VSTATUS */
                          s->pic[24], /* Base Address Update */
                          s->pic[23], /* FIFO Under-Run */
                          s->pic[22], /* AHB Bus Error */
                          NULL);

    /* fttsc010 */
    sysbus_create_simple("fttsc010", 0x92400000, s->pic[19]);

    /* ftsdc010 */
    ds = sysbus_create_simple("ftsdc010", 0x90600000, s->pic[39]);
    ack = qdev_get_gpio_in(ds, 0);
    req = qdev_get_gpio_in(s->hdma[0], 13);
    qdev_connect_gpio_out(s->hdma[0], 13, ack);
    qdev_connect_gpio_out(ds, 0, req);
}

static void a369soc_realize(DeviceState *dev, Error **errp)
{
    FaradaySoCState *s = FARADAY_SOC(dev);

    a369soc_reset(dev);
    a369soc_chip_init(s);
}

static const VMStateDescription vmstate_a369soc = {
    .name = TYPE_FARADAY_SOC,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_END_OF_LIST(),
    }
};

static void a369soc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc  = TYPE_FARADAY_SOC;
    dc->vmsd  = &vmstate_a369soc;
    dc->reset = a369soc_reset;
    dc->realize = a369soc_realize;
//    dc->no_user = 1;
}

static const TypeInfo a369soc_info = {
    .name          = TYPE_FARADAY_SOC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(FaradaySoCState),
    .class_init    = a369soc_class_init,
};

static void a369soc_register_types(void)
{
    type_register_static(&a369soc_info);
}

type_init(a369soc_register_types)
