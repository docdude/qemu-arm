/*
 *      PCI Class, Vendor and Device IDs
 *
 *      Please keep sorted.
 *
 *      Abbreviated version of linux/pci_ids.h
 *
 *      QEMU-specific definitions belong in pci.h
 */
#ifndef HW_PCI_IDS_H
#define HW_PCI_IDS_H 1

/* Device classes and subclasses */

#define PCI_BASE_CLASS_STORAGE           0x01
#define PCI_BASE_CLASS_NETWORK           0x02

#define PCI_CLASS_STORAGE_SCSI           0x0100
#define PCI_CLASS_STORAGE_IDE            0x0101
#define PCI_CLASS_STORAGE_RAID           0x0104
#define PCI_CLASS_STORAGE_SATA           0x0106
#define PCI_CLASS_STORAGE_EXPRESS        0x0108
#define PCI_CLASS_STORAGE_OTHER          0x0180

#define PCI_CLASS_NETWORK_ETHERNET       0x0200

#define PCI_CLASS_DISPLAY_VGA            0x0300
#define PCI_CLASS_DISPLAY_OTHER          0x0380

#define PCI_CLASS_MULTIMEDIA_AUDIO       0x0401

#define PCI_CLASS_MEMORY_RAM             0x0500

#define PCI_CLASS_SYSTEM_OTHER           0x0880

#define PCI_CLASS_SERIAL_USB             0x0c03
#define PCI_CLASS_SERIAL_SMBUS           0x0c05

#define PCI_CLASS_BRIDGE_HOST            0x0600
#define PCI_CLASS_BRIDGE_ISA             0x0601
#define PCI_CLASS_BRIDGE_PCI             0x0604
#define PCI_CLASS_BRIDGE_PCI_INF_SUB     0x01
#define PCI_CLASS_BRIDGE_OTHER           0x0680

#define PCI_CLASS_COMMUNICATION_SERIAL   0x0700
#define PCI_CLASS_COMMUNICATION_OTHER    0x0780

#define PCI_CLASS_PROCESSOR_CO           0x0b40
#define PCI_CLASS_PROCESSOR_POWERPC      0x0b20

#define PCI_CLASS_OTHERS                 0xff

/* Vendors and devices.  Sort key: vendor first, device next. */

#define PCI_VENDOR_ID_LSI_LOGIC          0x1000
#define PCI_DEVICE_ID_LSI_53C810         0x0001
#define PCI_DEVICE_ID_LSI_53C895A        0x0012
#define PCI_DEVICE_ID_LSI_SAS1078        0x0060

#define PCI_VENDOR_ID_DEC                0x1011
#define PCI_DEVICE_ID_DEC_21154          0x0026

#define PCI_VENDOR_ID_CIRRUS             0x1013

#define PCI_VENDOR_ID_IBM                0x1014

#define PCI_VENDOR_ID_AMD                0x1022
#define PCI_DEVICE_ID_AMD_LANCE          0x2000
#define PCI_DEVICE_ID_AMD_SCSI           0x2020

#define PCI_VENDOR_ID_TI                 0x104c

#define PCI_VENDOR_ID_MOTOROLA           0x1057
#define PCI_DEVICE_ID_MOTOROLA_MPC106    0x0002
#define PCI_DEVICE_ID_MOTOROLA_RAVEN     0x4801

#define PCI_VENDOR_ID_APPLE              0x106b
#define PCI_DEVICE_ID_APPLE_UNI_N_AGP    0x0020
#define PCI_DEVICE_ID_APPLE_U3_AGP       0x004b

#define PCI_VENDOR_ID_SUN                0x108e
#define PCI_DEVICE_ID_SUN_EBUS           0x1000
#define PCI_DEVICE_ID_SUN_SIMBA          0x5000
#define PCI_DEVICE_ID_SUN_SABRE          0xa000

#define PCI_VENDOR_ID_CMD                0x1095
#define PCI_DEVICE_ID_CMD_646            0x0646

#define PCI_VENDOR_ID_REALTEK            0x10ec
#define PCI_DEVICE_ID_REALTEK_8139       0x8139

#define PCI_VENDOR_ID_XILINX             0x10ee

#define PCI_VENDOR_ID_VIA                0x1106
#define PCI_DEVICE_ID_VIA_ISA_BRIDGE     0x0686
#define PCI_DEVICE_ID_VIA_IDE            0x0571
#define PCI_DEVICE_ID_VIA_UHCI           0x3038
#define PCI_DEVICE_ID_VIA_ACPI           0x3057
#define PCI_DEVICE_ID_VIA_AC97           0x3058
#define PCI_DEVICE_ID_VIA_MC97           0x3068

#define PCI_VENDOR_ID_MARVELL            0x11ab

#define PCI_VENDOR_ID_ENSONIQ            0x1274
#define PCI_DEVICE_ID_ENSONIQ_ES1370     0x5000

#define PCI_VENDOR_ID_FREESCALE          0x1957
#define PCI_DEVICE_ID_MPC8533E           0x0030

#define PCI_VENDOR_ID_INTEL              0x8086
#define PCI_DEVICE_ID_INTEL_82378        0x0484
#define PCI_DEVICE_ID_INTEL_82441        0x1237
#define PCI_DEVICE_ID_INTEL_82801AA_5    0x2415
#define PCI_DEVICE_ID_INTEL_82801BA_11   0x244e
#define PCI_DEVICE_ID_INTEL_82801D       0x24CD
#define PCI_DEVICE_ID_INTEL_ESB_9        0x25ab
#define PCI_DEVICE_ID_INTEL_82371SB_0    0x7000
#define PCI_DEVICE_ID_INTEL_82371SB_1    0x7010
#define PCI_DEVICE_ID_INTEL_82371SB_2    0x7020
#define PCI_DEVICE_ID_INTEL_82371AB_0    0x7110
#define PCI_DEVICE_ID_INTEL_82371AB      0x7111
#define PCI_DEVICE_ID_INTEL_82371AB_2    0x7112
#define PCI_DEVICE_ID_INTEL_82371AB_3    0x7113

#define PCI_DEVICE_ID_INTEL_ICH9_0       0x2910
#define PCI_DEVICE_ID_INTEL_ICH9_1       0x2917
#define PCI_DEVICE_ID_INTEL_ICH9_2       0x2912
#define PCI_DEVICE_ID_INTEL_ICH9_3       0x2913
#define PCI_DEVICE_ID_INTEL_ICH9_4       0x2914
#define PCI_DEVICE_ID_INTEL_ICH9_5       0x2919
#define PCI_DEVICE_ID_INTEL_ICH9_6       0x2930
#define PCI_DEVICE_ID_INTEL_ICH9_7       0x2916
#define PCI_DEVICE_ID_INTEL_ICH9_8       0x2918

#define PCI_DEVICE_ID_INTEL_82801I_UHCI1 0x2934
#define PCI_DEVICE_ID_INTEL_82801I_UHCI2 0x2935
#define PCI_DEVICE_ID_INTEL_82801I_UHCI3 0x2936
#define PCI_DEVICE_ID_INTEL_82801I_UHCI4 0x2937
#define PCI_DEVICE_ID_INTEL_82801I_UHCI5 0x2938
#define PCI_DEVICE_ID_INTEL_82801I_UHCI6 0x2939
#define PCI_DEVICE_ID_INTEL_82801I_EHCI1 0x293a
#define PCI_DEVICE_ID_INTEL_82801I_EHCI2 0x293c
#define PCI_DEVICE_ID_INTEL_82599_SFP_VF 0x10ed

#define PCI_DEVICE_ID_INTEL_Q35_MCH      0x29c0

#define PCI_VENDOR_ID_XEN                0x5853
#define PCI_DEVICE_ID_XEN_PLATFORM       0x0001

#define PCI_VENDOR_ID_NEC                0x1033
#define PCI_DEVICE_ID_NEC_UPD720200      0x0194

#define PCI_VENDOR_ID_TEWS               0x1498
#define PCI_DEVICE_ID_TEWS_TPCI200       0x30C8

#define	PCI_VENDOR_ID_BROADCOM		0x14e4
/* PCI Device IDs */
#define	PCI_DEVICE_ID_BCM421		0x1072		/* never used */
#define	PCI_DEVICE_ID_BCM4230		0x1086		/* never used */
#define	PCI_DEVICE_ID_BCM4401_ENET	0x170c		/* 4401b0 production enet cards */
#define	PCI_DEVICE_ID_BCM3352		0x3352		/* DEVICE_ID_DEVICE_ID_BCM3352 device id */
#define	PCI_DEVICE_ID_BCM3360		0x3360		/* DEVICE_ID_DEVICE_ID_BCM3360 device id */
#define	PCI_DEVICE_ID_BCM4211		0x4211
#define	PCI_DEVICE_ID_BCM4231		0x4231
#define	PCI_DEVICE_ID_BCM4303_D11B	0x4303		/* 4303 802.11b */
#define	PCI_DEVICE_ID_BCM4311_D11G	0x4311		/* 4311 802.11b/g id */
#define	PCI_DEVICE_ID_BCM4311_D11DUAL	0x4312		/* 4311 802.11a/b/g id */
#define	PCI_DEVICE_ID_BCM4311_D11A	0x4313		/* 4311 802.11a id */
#define	PCI_DEVICE_ID_BCM4328_D11DUAL	0x4314		/* 4328/4312 802.11a/g id */
#define	PCI_DEVICE_ID_BCM4328_D11G	0x4315		/* 4328/4312 802.11g id */
#define	PCI_DEVICE_ID_BCM4328_D11A	0x4316		/* 4328/4312 802.11a id */
#define	PCI_DEVICE_ID_BCM4318_D11G	0x4318		/* 4318 802.11b/g id */
#define	PCI_DEVICE_ID_BCM4318_D11DUAL	0x4319		/* 4318 802.11a/b/g id */
#define	PCI_DEVICE_ID_BCM4318_D11A	0x431a		/* 4318 802.11a id */
#define	PCI_DEVICE_ID_BCM4325_D11DUAL	0x431b		/* 4325 802.11a/g id */
#define	PCI_DEVICE_ID_BCM4325_D11G	0x431c		/* 4325 802.11g id */
#define	PCI_DEVICE_ID_BCM4325_D11A	0x431d		/* 4325 802.11a id */
#define	PCI_DEVICE_ID_BCM4306_D11G	0x4320		/* 4306 802.11g */
#define	PCI_DEVICE_ID_BCM4306_D11A	0x4321		/* 4306 802.11a */
#define	PCI_DEVICE_ID_BCM4306_UART	0x4322		/* 4306 uart */
#define	PCI_DEVICE_ID_BCM4306_V90	0x4323		/* 4306 v90 codec */
#define	PCI_DEVICE_ID_BCM4306_D11DUAL	0x4324		/* 4306 dual A+B */
#define	PCI_DEVICE_ID_BCM4306_D11G_ID2	0x4325		/* PCI_DEVICE_ID_BCM4306_D11G_ID; INF w/loose binding war */
#define	PCI_DEVICE_ID_BCM4321_D11N	0x4328		/* 4321 802.11n dualband id */
#define	PCI_DEVICE_ID_BCM4321_D11N2G	0x4329		/* 4321 802.11n 2.4Ghz band id */
#define	PCI_DEVICE_ID_BCM4321_D11N5G	0x432a		/* 4321 802.11n 5Ghz band id */
#define PCI_DEVICE_ID_BCM4322_D11N	0x432b		/* 4322 802.11n dualband device */
#define PCI_DEVICE_ID_BCM4322_D11N2G	0x432c		/* 4322 802.11n 2.4GHz device */
#define PCI_DEVICE_ID_BCM4322_D11N5G	0x432d		/* 4322 802.11n 5GHz device */
#define PCI_DEVICE_ID_BCM4329_D11N	0x432e		/* 4329 802.11n dualband device */
#define PCI_DEVICE_ID_BCM4329_D11N2G	0x432f		/* 4329 802.11n 2.4G device */
#define PCI_DEVICE_ID_BCM4329_D11N5G	0x4330		/* 4329 802.11n 5G device */
#define	PCI_DEVICE_ID_BCM4315_D11DUAL	0x4334		/* 4315 802.11a/g id */
#define	PCI_DEVICE_ID_BCM4315_D11G	0x4335		/* 4315 802.11g id */
#define	PCI_DEVICE_ID_BCM4315_D11A	0x4336		/* 4315 802.11a id */
#define PCI_DEVICE_ID_BCM4319_D11N	0x4337		/* 4319 802.11n dualband device */
#define PCI_DEVICE_ID_BCM4319_D11N2G	0x4338		/* 4319 802.11n 2.4G device */
#define PCI_DEVICE_ID_BCM4319_D11N5G	0x4339		/* 4319 802.11n 5G device */
#define PCI_DEVICE_ID_BCM43231_D11N2G	0x4340		/* 43231 802.11n 2.4GHz device */
#define PCI_DEVICE_ID_BCM43221_D11N2G	0x4341		/* 43221 802.11n 2.4GHz device */
#define PCI_DEVICE_ID_BCM43222_D11N	0x4350		/* 43222 802.11n dualband device */
#define PCI_DEVICE_ID_BCM43222_D11N2G	0x4351		/* 43222 802.11n 2.4GHz device */
#define PCI_DEVICE_ID_BCM43222_D11N5G	0x4352		/* 43222 802.11n 5GHz device */
#define PCI_DEVICE_ID_BCM43224_D11N	0x4353		/* 43224 802.11n dualband device */
#define PCI_DEVICE_ID_BCM43224_D11N_VEN1	0x0576		/* Vendor specific 43224 802.11n db device */
#define PCI_DEVICE_ID_BCM43226_D11N	0x4354		/* 43226 802.11n dualband device */
#define PCI_DEVICE_ID_BCM43236_D11N	0x4346		/* 43236 802.11n dualband device */
#define PCI_DEVICE_ID_BCM43236_D11N2G	0x4347		/* 43236 802.11n 2.4GHz device */
#define PCI_DEVICE_ID_BCM43236_D11N5G	0x4348		/* 43236 802.11n 5GHz device */
#define PCI_DEVICE_ID_BCM43225_D11N2G	0x4357		/* 43225 802.11n 2.4GHz device */
#define PCI_DEVICE_ID_BCM43421_D11N	0xA99D		/* 43421 802.11n dualband device */
#define PCI_DEVICE_ID_BCM4313_D11N2G	0x4727		/* 4313 802.11n 2.4G device */
#define PCI_DEVICE_ID_BCM4330_D11N      0x4360          /* 4330 802.11n dualband device */
#define PCI_DEVICE_ID_BCM4330_D11N2G    0x4361          /* 4330 802.11n 2.4G device */
#define PCI_DEVICE_ID_BCM4330_D11N5G    0x4362          /* 4330 802.11n 5G device */
#define PCI_DEVICE_ID_BCM4336_D11N	0x4343		/* 4336 802.11n 2.4GHz device */
#define PCI_DEVICE_ID_BCM6362_D11N	0x435f		/* 6362 802.11n dualband device */
#define PCI_DEVICE_ID_BCM4331_D11N	0x4331		/* 4331 802.11n dualband id */
#define PCI_DEVICE_ID_BCM4331_D11N2G	0x4332		/* 4331 802.11n 2.4Ghz band id */
#define PCI_DEVICE_ID_BCM4331_D11N5G	0x4333		/* 4331 802.11n 5Ghz band id */
#define PCI_DEVICE_ID_BCM43237_D11N	0x4355		/* 43237 802.11n dualband device */
#define PCI_DEVICE_ID_BCM43237_D11N5G	0x4356		/* 43237 802.11n 5GHz device */
#define PCI_DEVICE_ID_BCM43227_D11N2G	0x4358		/* 43228 802.11n 2.4GHz device */
#define PCI_DEVICE_ID_BCM43228_D11N	0x4359		/* 43228 802.11n DualBand device */
#define PCI_DEVICE_ID_BCM43228_D11N5G	0x435a		/* 43228 802.11n 5GHz device */
#define PCI_DEVICE_ID_BCM43362_D11N	0x4363		/* 43362 802.11n 2.4GHz device */
#define PCI_DEVICE_ID_BCM43239_D11N	0x4370		/* 43239 802.11n dualband device */
#define PCI_DEVICE_ID_BCM4324_D11N	0x4374		/* 4324 802.11n dualband device */
#define PCI_DEVICE_ID_BCM43217_D11N2G	0x43a9		/* 43217 802.11n 2.4GHz device */
#define PCI_DEVICE_ID_BCM43131_D11N2G	0x43aa		/* 43131 802.11n 2.4GHz device */
#define PCI_DEVICE_ID_BCM4314_D11N2G	0x4364		/* 4314 802.11n 2.4G device */
#define PCI_DEVICE_ID_BCM43142_D11N2G	0x4365		/* 43142 802.11n 2.4G device */
#define PCI_DEVICE_ID_BCM43143_D11N2G	0x4366		/* 43143 802.11n 2.4G device */
#define PCI_DEVICE_ID_BCM4334_D11N	0x4380		/* 4334 802.11n dualband device */
#define PCI_DEVICE_ID_BCM4334_D11N2G	0x4381		/* 4334 802.11n 2.4G device */
#define PCI_DEVICE_ID_BCM4334_D11N5G	0x4382		/* 4334 802.11n 5G device */
#define PCI_DEVICE_ID_BCM4360_D11AC	0x43a0
#define PCI_DEVICE_ID_BCM4360_D11AC2G	0x43a1
#define PCI_DEVICE_ID_BCM4360_D11AC5G	0x43a2
#define PCI_DEVICE_ID_BCM4335_D11AC	0x43ae
#define PCI_DEVICE_ID_BCM4335_D11AC2G	0x43af
#define PCI_DEVICE_ID_BCM4335_D11AC5G	0x43b0
#define PCI_DEVICE_ID_BCM4352_D11AC	0x43b1		/* 4352 802.11ac dualband device */
#define PCI_DEVICE_ID_BCM4352_D11AC2G	0x43b2		/* 4352 802.11ac 2.4G device */
#define PCI_DEVICE_ID_BCM4352_D11AC5G	0x43b3		/* 4352 802.11ac 5G device */
#define	PCI_VENDOR_ID_3COM		0x10b7
#define	PCI_VENDOR_ID_NETGEAR		0x1385

/* PCI Subsystem ID */
#define BCM943228HMB_SSID_VEN1	0x0607
#define BCM94313HMGBL_SSID_VEN1	0x0608
#define BCM94313HMG_SSID_VEN1	0x0609
#define BCM943142HM_SSID_VEN1	0x0611

#define BCM43143_D11N2G_ID	0x4366		/* 43143 802.11n 2.4G device */

#define BCM43242_D11N_ID	0x4367		/* 43242 802.11n dualband device */
#define BCM43242_D11N2G_ID	0x4368		/* 43242 802.11n 2.4G device */
#define BCM43242_D11N5G_ID	0x4369		/* 43242 802.11n 5G device */

#define BCM4350_D11AC_ID	0x43a3
#define BCM4350_D11AC2G_ID	0x43a4
#define BCM4350_D11AC5G_ID	0x43a5


#define	BCMGPRS_UART_ID		0x4333		/* Uart id used by 4306/gprs card */
#define	BCMGPRS2_UART_ID	0x4344		/* Uart id used by 4306/gprs card */
#define FPGA_JTAGM_ID		0x43f0		/* FPGA jtagm device id */
#define BCM_JTAGM_ID		0x43f1		/* BCM jtagm device id */
#define SDIOH_FPGA_ID		0x43f2		/* sdio host fpga */
#define BCM_SDIOH_ID		0x43f3		/* BCM sdio host id */
#define SDIOD_FPGA_ID		0x43f4		/* sdio device fpga */
#define SPIH_FPGA_ID		0x43f5		/* PCI SPI Host Controller FPGA */
#define BCM_SPIH_ID		0x43f6		/* Synopsis SPI Host Controller */
#define MIMO_FPGA_ID		0x43f8		/* FPGA mimo minimacphy device id */
#define BCM_JTAGM2_ID		0x43f9		/* BCM alternate jtagm device id */
#define SDHCI_FPGA_ID		0x43fa		/* Standard SDIO Host Controller FPGA */
#define	BCM4402_ENET_ID		0x4402		/* 4402 enet */
#define	BCM4402_V90_ID		0x4403		/* 4402 v90 codec */
#define	BCM4410_DEVICE_ID	0x4410		/* bcm44xx family pci iline */
#define	BCM4412_DEVICE_ID	0x4412		/* bcm44xx family pci enet */
#define	BCM4430_DEVICE_ID	0x4430		/* bcm44xx family cardbus iline */
#define	BCM4432_DEVICE_ID	0x4432		/* bcm44xx family cardbus enet */
#define	BCM4704_ENET_ID		0x4706		/* 4704 enet (Use 47XX_ENET_ID instead!) */
#define	BCM4710_DEVICE_ID	0x4710		/* 4710 primary function 0 */
#define	BCM47XX_AUDIO_ID	0x4711		/* 47xx audio codec */
#define	BCM47XX_V90_ID		0x4712		/* 47xx v90 codec */
#define	BCM47XX_ENET_ID		0x4713		/* 47xx enet */
#define	BCM47XX_EXT_ID		0x4714		/* 47xx external i/f */
#define	BCM47XX_GMAC_ID		0x4715		/* 47xx Unimac based GbE */
#define	BCM47XX_USBH_ID		0x4716		/* 47xx usb host */
#define	BCM47XX_USBD_ID		0x4717		/* 47xx usb device */
#define	BCM47XX_IPSEC_ID	0x4718		/* 47xx ipsec */
#define	BCM47XX_ROBO_ID		0x4719		/* 47xx/53xx roboswitch core */
#define	BCM47XX_USB20H_ID	0x471a		/* 47xx usb 2.0 host */
#define	BCM47XX_USB20D_ID	0x471b		/* 47xx usb 2.0 device */
#define	BCM47XX_ATA100_ID	0x471d		/* 47xx parallel ATA */
#define	BCM47XX_SATAXOR_ID	0x471e		/* 47xx serial ATA & XOR DMA */
#define	BCM47XX_GIGETH_ID	0x471f		/* 47xx GbE (5700) */
#define	BCM4712_MIPS_ID		0x4720		/* 4712 base devid */
#define	BCM4716_DEVICE_ID	0x4722		/* 4716 base devid */
#define	BCM47XX_USB30H_ID	0x472a		/* 47xx usb 3.0 host */
#define	BCM47XX_USB30D_ID	0x472b		/* 47xx usb 3.0 device */
#define BCM47XX_SMBUS_EMU_ID	0x47fe		/* 47xx emulated SMBus device */
#define	BCM47XX_XOR_EMU_ID	0x47ff		/* 47xx emulated XOR engine */
#define	EPI41210_DEVICE_ID	0xa0fa		/* bcm4210 */
#define	EPI41230_DEVICE_ID	0xa10e		/* bcm4230 */
#define JINVANI_SDIOH_ID	0x4743		/* Jinvani SDIO Gold Host */
#define BCM27XX_SDIOH_ID	0x2702		/* BCM27xx Standard SDIO Host */
#define PCIXX21_FLASHMEDIA_ID	0x803b		/* TI PCI xx21 Standard Host Controller */
#define PCIXX21_SDIOH_ID	0x803c		/* TI PCI xx21 Standard Host Controller */
#define R5C822_SDIOH_ID		0x0822		/* Ricoh Co Ltd R5C822 SD/SDIO/MMC/MS/MSPro Host */
#define JMICRON_SDIOH_ID	0x2381		/* JMicron Standard SDIO Host Controller */
#define BCM501_UNKNOWN		0x0501		/* TEMP UNTIL I FIXUP */

#endif
