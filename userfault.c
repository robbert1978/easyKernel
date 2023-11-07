#include "userfault.h"

#ifdef DEBUG

#define logOK(msg, ...) dprintf(STDERR_FILENO, "[+] " msg "\n", ##__VA_ARGS__)

#define logInfo(msg, ...) dprintf(STDERR_FILENO, "[!] " msg "\n", ##__VA_ARGS__)

#define logErr(msg, ...) dprintf(STDERR_FILENO, "[-] " msg "\n", ##__VA_ARGS__)

#define errExit(msg, ...)                                      \
    do                                                         \
    {                                                          \
        dprintf(STDERR_FILENO, "[-] " msg " ", ##__VA_ARGS__); \
        perror("");                                            \
        exit(-1);                                              \
    } while (0)

#define WAIT()                                        \
    do                                                \
    {                                                 \
        write(STDERR_FILENO, "[WAITTING ...]\n", 16); \
        getchar();                                    \
    } while (0)

#else

#define logOK(...) \
    do             \
    {              \
    } while (0)
#define logInfo(...) \
    do               \
    {                \
    } while (0)
#define logErr(...) \
    do              \
    {               \
    } while (0)
#define errExit(...) \
    do               \
    {                \
    } while (0)

#endif

#define userfaultfd(flags) syscall(SYS_userfaultfd, flags)

char uf_buffer[PAGE_SIZE];

int register_ufd(uint64_t page)
{
    int fd = 0;
    uint64_t uf_page = page;
    struct uffdio_api api = {.api = UFFD_API};

    uf_page = (uint64_t)mmap((void *)uf_page, PAGE_SIZE, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if ((void *)uf_page == MAP_FAILED)
    {
        perror("mmap uf_page");
        exit(2);
    }

    if ((fd = userfaultfd(O_NONBLOCK)) == -1)
    {
        errExit("userfaultfd failed");
    }

    if (ioctl(fd, UFFDIO_API, &api))
    {
        errExit("+ ioctl(fd, UFFDIO_API, ...) failed");
    }
    if (api.api != UFFD_API)
    {
        errExit("unexepcted UFFD api version.");
    }

    /* mmap some pages, set them up with the userfaultfd. */
    struct uffdio_register reg = {
        .mode = UFFDIO_REGISTER_MODE_MISSING,
        .range = {
            .start = uf_page,
            .len = PAGE_SIZE}};

    if (ioctl(fd, UFFDIO_REGISTER, &reg) == -1)
    {
        errExit("ioctl(fd, UFFDIO_REGISTER, ...) failed");
    }

    return fd;
}

void *race_userfault(void *arg_)
{
    struct userfault_arg *arg = (struct userfault_arg *)arg_;

    struct pollfd evt = {.fd = arg->ufd, .events = POLLIN};

    while (poll(&evt, 1, -1) > 0)
    {
        /* unexpected poll events */
        if (evt.revents & POLLERR)
        {
            perror("poll");
            exit(-1);
        }
        else if (evt.revents & POLLHUP)
        {
            perror("pollhup");
            exit(-1);
        }
        struct uffd_msg fault_msg = {0};
        if (read(arg->ufd, &fault_msg, sizeof(fault_msg)) != sizeof(fault_msg))
        {
            perror("read");
            exit(-1);
        }
        char *place = (char *)fault_msg.arg.pagefault.address;
        if (fault_msg.event != UFFD_EVENT_PAGEFAULT || (place != (void *)arg->uf_page && place != (void *)arg->uf_page + PAGE_SIZE))
        {
            fprintf(stderr, "unexpected pagefault?.\n");
            exit(-1);
        }
        if (place == (void *)arg->uf_page)
        {
            logInfo("Page fault at address %p, nice!\n", place);
            arg->func();
            /* release by copying some data to faulting address */
            struct uffdio_copy copy = {
                .dst = (long)place,
                .src = (long)uf_buffer,
                .len = PAGE_SIZE};
            if (ioctl(arg->ufd, UFFDIO_COPY, &copy) < 0)
            {
                perror("ioctl(UFFDIO_COPY)");
                exit(-1);
            }
            break;
        }
    }
    close(arg->ufd);

    free(arg);

    return NULL;
}

void createThreadUserFault(uint64_t page, void (*func)(void))
{
    struct userfault_arg *arg = malloc(sizeof(struct userfault_arg));
    arg->uf_page = page;
    arg->ufd = register_ufd(page);
    arg->func = func;
    pthread_t th;
    pthread_create(&th, NULL, race_userfault, arg);
}