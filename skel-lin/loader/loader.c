/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "exec_parser.h"

static so_exec_t *exec;

// global file descriptor for the given pathname
static int fd;

// default handler is called for page faults that happen either
// at memory addresses not in a segment or in already mapped
// segments
void default_handler()
{
	// changes handler to default SIGSEGV handler
	signal(SIGSEGV, SIG_DFL);
	// throws SIGSEGV signal
	raise(SIGSEGV);
}

static void segv_handler(int signum, siginfo_t *info, void *context)
{
	int i, page_index, N, remainder;
	void *addr_buf;

	// iterates through all segments and, when it finds which
	// segment the page fault address is part of, it breaks the
	// loop such that the index i now points to the segment
	// which will be mapped in memory
	for (i = 0; i < exec->segments_no; i++)
		if ((void *)(exec->segments[i].vaddr) <= info->si_addr &&
			info->si_addr < (void *)(exec->segments[i].vaddr +
									exec->segments[i].mem_size))
			break;

	// if the index reached the end of the loop, then the page fault address
	// isn't part of any segment, so the default handler is called
	if (i == exec->segments_no) {
		default_handler();
		return;
	}

	// page_index is a number in range [0, mem_size / 0x1000], which
	// calculates the index of the page where the page fault address is
	page_index = (int) (info->si_addr - exec->segments[i].vaddr) / 0x1000;

	// N calculates the page index in which the last bytes of file_size are
	N = exec->segments[i].file_size / 0x1000;

	// remainder calculates the number of bytes remaining in page N
	remainder = exec->segments[i].file_size % 0x1000;

	// addr_buf uses the page index and the virtual address of the segment
	// to calculate the base address of the page that needs to be mapped
	addr_buf = (void *)(exec->segments[i].vaddr + page_index * 0x1000);

	// allocates memory for the page;
	// because of the flag MAP_ANON, which initializes the mapped
	// area with 0, the page is mapped with read-write permissions
	// in order to be able to copy the contents from the ELF file
	// to the mapped page
	mmap(addr_buf, 0x1000, PERM_R | PERM_W,
		 MAP_ANON | MAP_PRIVATE | MAP_FIXED_NOREPLACE, -1, 0);

	// the MAP_FIXED_NOREPLACE flag helps with pages that have
	// already been mapped;
	// the flag detects if a memory zone has already been
	// allocated, in which case is does NOT reinitialize it
	// with 0 (like MAP_FIXED does) and it throws the EEXIST
	// error to errno;
	// so the if statement checks if the value of errno indicates
	// that the page has already been mapped, in which case it
	// calls the default handler for SIGSEGV
	if (errno == EEXIST) {
		default_handler();
		return;
	}

	// the call to lseek sets the cursor of the ELF file to the base address
	// of the current mapped page
	lseek(fd, exec->segments[i].offset + page_index * 0x1000, SEEK_SET);

	// if the mapped page is the Nth page, only the remaining bytes in page N
	// are copied from the ELF file with a read syscall, at the offset
	// calculated by lseek
	if (page_index == N)
		read(fd, addr_buf, remainder);

	// if the page index is in range [0, N - 1], 4K bytes are copied from the
	// ELF file, at the offset calculated by lseek, using a read syscall
	else if (page_index < N)
		read(fd, addr_buf, 0x1000);

	// by using the MAP_ANON flag, all the other data-copy cases (notably the
	// space between the end of file_size and the end of mem_size) are already
	// initialized with 0, so they don't need to be treated separately

	// at the end, the permissions of the segment, specified by the ELF program
	// header, are set for the mapped page
	mprotect(addr_buf, 0x1000, exec->segments[i].perm);
}

int so_init_loader(void)
{
	int rc;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = segv_handler;
	sa.sa_flags = SA_SIGINFO;
	rc = sigaction(SIGSEGV, &sa, NULL);
	if (rc < 0) {
		perror("sigaction");
		return -1;
	}
	return 0;
}

int so_execute(char *path, char *argv[])
{
	exec = so_parse_exec(path);

	// opens the ELF file associated with path to the global file descriptor
	fd = open(path, O_RDONLY);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	return -1;
}
