#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <liburing.h>
#include <pthread.h>
#include <sys/capability.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <linux/capability.h>
#include <linux/types.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define DEBUG
#ifdef DEBUG
#define errExit(msg)        \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)
#define WAIT()                \
    do                        \
    {                         \
        puts("[WAITING...]"); \
        getchar();            \
    } while (0)

#define logOK(msg, ...) dprintf(2, "[+] " msg "\n", ##__VA_ARGS__);
#define logInfo(msg, ...) dprintf(2, "[*] " msg "\n", ##__VA_ARGS__);
#define logErr(msg, ...) dprintf(2, "[!] " msg "\n", ##__VA_ARGS__);
#else
#define errExit(...) \
    do               \
    {                \
    } while (0)

#define WAIT(...) errExit(...)
#define logOK(...) errExit(...)
#define logInfo(...) errExit(...)
#define logErr(...) errExit(...)
#endif
u64 user_ip;
u64 user_cs;
u64 user_rflags;
u64 user_sp;
u64 user_ss;
void get_shell()
{
    if (getuid())
    {
        errExit("NO ROOT");
    }
    logOK("Rooted!");
    char *argv[] = {"/bin/sh", NULL};
    char *envp[] = {NULL};
    execve(argv[0], argv, envp);
}
void save_state()
{
    __asm__(
        "mov user_cs, cs;"
        "mov user_ss, ss;"
        "mov user_sp, rsp;"
        "pushf;"
        "pop qword ptr[rip+user_rflags];");
    user_ip = (u64)get_shell;
    logInfo("Saved user state");
}

int main(int argc, char **argv, char **envp)
{
}