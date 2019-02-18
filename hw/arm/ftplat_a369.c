/*
 * Faraday A369 Evalution Board
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
#include "hw/audio.h"
#include "hw/boards.h"
#include "hw/ssi.h"
#include "net/net.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"
#include "qemu/error-report.h"
#include "hw/arm/faraday.h"
#include "hw/arm/arm.h"

static void a369_system_reset(void *opaque)
{
    cpu_reset(CPU(opaque));
}

/* Board init.  */

static void a369_board_init(QEMUMachineInitArgs *args)
{
    int i, nr_flash;
//struct FaradaySoCState *s = g_malloc0(sizeof(*s));
   DeviceState *ds;
    FaradaySoCState *s;

    ObjectClass *cpu_oc;
//s = (FaradaySoCState *) g_malloc0(sizeof(FaradaySoCState));
//s->cpu = g_malloc0(sizeof(ARMCPU));
    DriveInfo *dinfo;
    Error *local_errp = NULL;

    if (!args->cpu_model) {
        args->cpu_model = "fa626te";
    }

    if (!args->ram_size) {
        args->ram_size = 512 << 20;
    }

    /* SoC */
    ds = qdev_create(NULL, TYPE_FARADAY_SOC);

    s = FARADAY_SOC(SYS_BUS_DEVICE(ds));

    /* Setup QOM path for the SoC object (i.e. /machine/faraday.soc) */
    object_property_add_child(qdev_get_machine(),
                              TYPE_FARADAY_SOC,
                              OBJECT(ds),
                              NULL);


    /* CPU */
    cpu_oc = cpu_class_by_name(TYPE_ARM_CPU, args->cpu_model);

    s->cpu = ARM_CPU(object_new(object_class_get_name(cpu_oc)));

    object_property_set_bool(OBJECT(s->cpu), true, "realized", &local_errp);
    if (local_errp) {
        error_report("%s", error_get_pretty(local_errp));
        exit(1);
    }

    if (!s->cpu) {
        fprintf(stderr, "a369: Unable to find CPU definition\n");
        abort();
    }

    s->as  = get_system_memory();
    s->ram = g_new0(MemoryRegion, 1);
    memory_region_init_ram(s->ram, NULL, "faraday-soc.ram", args->ram_size);
    vmstate_register_ram_global(s->ram);
    qdev_init_nofail(ds);
  MemoryRegion *ram_visible = g_new0(MemoryRegion, 1);
            memory_region_init_alias(ram_visible,NULL,
                                     "faraday-soc.ram_visible",
                                     s->ram,
                                     0,
                                     ram_size);
       
//s->ram_base = s->ahb_slave[6] & 0xfff00000;  printf("===============");
      memory_region_add_subregion(s->as, 0x10000000, ram_visible);

    /* Interrupt Controller */
//   ds = sysbus_create_varargs("ftintc020", 0x90100000,
 //                   qdev_get_gpio_in(DEVICE(cpu), ARM_CPU_IRQ),
 //                   qdev_get_gpio_in(DEVICE(cpu), ARM_CPU_FIQ),
  //                  NULL);


    /* Customized system reset */
    qemu_register_reset(a369_system_reset, s->cpu);

    /* Attach the nand flash to ftnandc021 */
    dinfo = drive_get_next(IF_MTD);
    ds = nand_init(dinfo ? dinfo->bdrv : NULL, NAND_MFR_SAMSUNG, 0xda);
    object_property_set_link(OBJECT(s->nandc[0]),
                             OBJECT(ds),
                             "flash",
                             &local_errp);
    if (local_errp) {
        fprintf(stderr, "a369: Unable to set flash link for FTNANDC021\n");
        abort();
    }

    /* Attach the spi flash to ftssp010.0 */
    nr_flash = 1;
    for (i = 0; i < nr_flash; i++) {
        SSIBus *ssi = (SSIBus *)qdev_get_child_bus(s->spi[0], "spi");
        DeviceState *fl = ssi_create_slave_no_init(ssi, "w25q64");
        qemu_irq cs_line;

        qdev_init_nofail(fl);
        cs_line = qdev_get_gpio_in(fl, 0);
        sysbus_connect_irq(SYS_BUS_DEVICE(s->spi[0]), i + 1, cs_line);
    }

    /* Attach the wm8731 to fti2c010.0 & ftssp010.0 */
    for (i = 0; i < 1; ++i) {
        I2CBus *i2c = (I2CBus *)qdev_get_child_bus(s->i2c[0], "i2c");
        ds = i2c_create_slave(i2c, "wm8731", 0x1B);
        object_property_set_link(OBJECT(s->i2s[0]),
                                 OBJECT(ds),
                                 "codec",
                                 &local_errp);
        if (local_errp) {
            fprintf(stderr, "a369: Unable to set codec link for FTSSP010\n");
            abort();
        }
        audio_codec_data_req_set(ds,
                                 ftssp010_i2s_data_req,
                                 s->i2s[0]);
    }

    /* External AHB devices */

    /* Ethernet: FTMAC110 */
    if (nb_nics > 1) {
        ftmac110_init(&nd_table[1], 0xC0100000, s->pic[5]);
    }

    /* Timer: FTTMR010 */
    ds = qdev_create(NULL, "fttmr010");
    qdev_prop_set_uint32(ds, "freq", 33 * 1000000);
    qdev_init_nofail(ds);
    sysbus_mmio_map(SYS_BUS_DEVICE(ds), 0, 0xC0200000);
    sysbus_connect_irq(SYS_BUS_DEVICE(ds), 1, s->pic[6]);
    sysbus_connect_irq(SYS_BUS_DEVICE(ds), 2, s->pic[7]);

    /* SPI: FTSPI020 */
    ds = sysbus_create_simple("ftspi020", 0xC0000000, s->pic[4]);
    s->spi_fl[0] = ds;

    /* Attach the spi flash to ftspi020.0 */
    nr_flash = 1;
    for (i = 0; i < nr_flash; i++) {
        SSIBus *ssi = (SSIBus *)qdev_get_child_bus(s->spi_fl[0], "spi");
        DeviceState *fl = ssi_create_slave_no_init(ssi, "w25q64");
        qemu_irq cs_line;

        qdev_init_nofail(fl);
        cs_line = qdev_get_gpio_in(fl, 0);
        sysbus_connect_irq(SYS_BUS_DEVICE(s->spi_fl[0]), i + 1, cs_line);
    }

    /* System start-up */

  // s->ram_boot = false;
 //    faraday_soc_ahb_remap(s, false);
     //   faraday_soc_ram_setup(s, true);
   
    if (args->kernel_filename) {
        struct arm_boot_info *bi = g_new0(struct arm_boot_info, 1);

        s->ram_boot = true;

        faraday_soc_ram_setup(s, true);

        faraday_soc_ahb_remap(s, true);

        /* Boot Info */
        bi->ram_size = args->ram_size;
        bi->kernel_filename = args->kernel_filename;
        bi->kernel_cmdline = args->kernel_cmdline;
        bi->initrd_filename = args->initrd_filename;
        bi->board_id = 0x3369;
        arm_load_kernel(s->cpu, bi);
    } else if (!drive_get(IF_PFLASH, 0, 0)) {
        fprintf(stderr, "a369: Unable to load ROM image!\n");
        abort();
    }
}

static QEMUMachine a369_machine = {
    .name = "a369",
    .desc = "Faraday A369 (fa626te)",
    .init = a369_board_init,
 //   DEFAULT_MACHINE_OPTIONS,
};

static void a369_machine_init(void)
{
    qemu_register_machine(&a369_machine);
}

machine_init(a369_machine_init);
