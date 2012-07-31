#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#define RET0 "\x31\xc0\xc3"	/* xor eax,eax; ret */
#define REAL "\x55\x89\xe5"	/* original rsa_verify prologue */
#define PAGE_OFFSET 0x80000000	/* No idea why Revue uses 2gb/2gb config */

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

int main(int argc, char **argv)
{

	void *page;
	int fd;
	unsigned char *ptr;
	unsigned long rsa_verify;

	if (argc != 2 || (strcmp(argv[1], "disable") && 
	                 (strcmp(argv[1], "enable")))) {
		printf("[-] Usage: %s [disable|enable]\n", argv[0]);
		return 1;
	}

	if (getuid()) {
		printf("[-] Must be run as root.\n");
		return 1;
	}

	fd = open("/dev/devmem", O_RDWR | O_SYNC);

	if (fd < 0) {
		printf("[-] Failed to open devmem.\n");
		return 1;
	}

	rsa_verify = get_kernel_addr("rsa_verify");

	if (!rsa_verify) {
		printf("[-] Failed to resolve kernel symbols.\n");
		return 1;
	}

	page = mmap(NULL, 2*4096, PROT_READ | PROT_WRITE, MAP_SHARED,
			fd, (rsa_verify - PAGE_OFFSET)  & 0xfffff000);

	if (page == MAP_FAILED) {
		printf("[-] Failed to map memory: %s\n", strerror(errno));
		return 1;
	}

	ptr = page + (rsa_verify & 0xfff);

	if (!strcmp(argv[1], "enable")) {
		memcpy(ptr, REAL, 3);
		printf("[+] Module signing enabled.\n");
	} else if (!strcmp(argv[1], "disable")) {
		memcpy(ptr, RET0, 3);
		printf("[+] Module signing disabled.\n");
	}

	munmap(page, 2*4096);
	close(fd);

}
