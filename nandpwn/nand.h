#define NAND_MEM_REGS		0xdff00000
#define NAND_REGS		0xdff10000
#define CMD_MODE_10		(2 << 26)
#define IRQ_STS_DMA_DONE	(1 << 2)

typedef unsigned int u32;

struct nfc_mem_regs {
	u32 cmd;
	u32 __rsrv0[0x3];
	u32 io;
	u32 __rsrv1[0xb];
	u32 data;
};

struct nfc_regs {
	u32 reset;
	u32 __rsrv0[0x3];
	u32 transfer_spare;
	u32 __rsrv1[0x13];
	u32 rb_pin_en;
	u32 __rsrv2[0x1b];
	u32 ce_dont_care;
	u32 __rsrv3[0x3];
	u32 ecc_en;
	u32 __rsrv4[0x3];
	u32 global_irq_en;
	u32 __rsrv5[0x3];
	u32 we_2_re;
	u32 __rsrv6[0x3];
	u32 addr_2_data;
	u32 __rsrv7[0x3];
	u32 re_2_we;
	u32 __rsrv8[0x3];
	u32 acc_clks;
	u32 __rsrv9[0x7];
	u32 pages_per_block;
	u32 __rsrv10[0x7];
	u32 device_main_area_size;
	u32 __rsrv11[0x3];
	u32 device_spare_area_size;
	u32 __rsrv12[0xb];
	u32 ecc_correction;
	u32 __rsrv13[0xf];
	u32 rdwr_en_lo;
	u32 __rsrv14[0x3];
	u32 rdwr_en_hi;
	u32 __rsrv15[0x7];
	u32 cs_setup;
	u32 __rsrv16[0x3];
	u32 spare_area_skip_bytes;
	u32 __rsrv17[0x3];
	u32 spare_area_marker;
	u32 __rsrv18[0x13];
	u32 re_2_re;
	u32 __rsrv19[0x37];
	u32 revision;
	u32 __rsrv20[0xb];
	u32 onfi_timing_modes;
	u32 __rsrv21[0x13];
	u32 features;
	u32 __rsrv22[0x7];

	struct {
		u32 sts;
		u32 __rsrv23[0x3];
		u32 en;
		u32 __rsrv24[0xf];
	} irq[4];

	u32 __rsrv25[0x38];
	u32 ecc_error_address;
	u32 __rsrv26[0x3];
	u32 ecc_correction_info;
	u32 __rsrv27[0x2f];
	u32 dma_en;
};
