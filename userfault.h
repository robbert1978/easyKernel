#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <linux/userfaultfd.h>
#include <sys/wait.h>
#include <poll.h>
#include <sys/mman.h>
#include <stddef.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>

#define PAGE_SIZE 0x1000

struct userfault_arg
{
    uint32_t ufd;
    uint64_t uf_page;
    void (*func)(void);
};

extern char uf_buffer[PAGE_SIZE];

int register_ufd(uint64_t page);
void *race_userfault(void *arg);
void createThreadUserFault(uint64_t page, void (*func)(void));
