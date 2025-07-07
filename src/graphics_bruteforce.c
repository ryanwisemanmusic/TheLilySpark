#include "graphics_bruteforce.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <dirent.h>
#include <sys/xattr.h>
#include <poll.h>
#include <stdlib.h>

/*
TODO: Document this section extensively.
*/

static void try_open_device(const char *path, FILE *log) 
{
    int flags[] = 
    {
        O_RDWR, 
        O_WRONLY, 
        O_RDONLY, 
        O_RDWR|O_EXCL
    };
    const char *flagnames[] = 
    {
        "O_RDWR", 
        "O_WRONLY", 
        "O_RDONLY", 
        "O_RDWR|O_EXCL"
    };
    for (int i = 0; i < 4; ++i) 
    {
        int fd = open(path, flags[i]);
        if (fd >= 0) 
        {
            fprintf(log, 
                "[brute] open %s with %s: success\n", 
                path, flagnames[i]);
            close(fd);
        } 
        else 
        {
            fprintf(log, 
                "[brute] open %s with %s: %s\n", 
                path, flagnames[i], strerror(errno));
        }
    }
}

static void try_mmap_device(const char *path, FILE *log) 
{
    int fd = open(path, O_RDWR);
    if (fd < 0) return;
    size_t sizes[] = {4096, 65536, 1048576};
    off_t offsets[] = {0, 4096, 65536};
    for (int s = 0; s < 3; ++s) 
    {
        for (int o = 0; o < 3; ++o) 
        {
            void *p = mmap(NULL, sizes[s], 
                PROT_READ|PROT_WRITE, MAP_SHARED, 
                fd, offsets[o]);
            if (p == MAP_FAILED) 
            {
                fprintf(log, 
                    "[brute] mmap %s size %zu offset %lld: %s\n", 
                    path, sizes[s], (long long)offsets[o], 
                    strerror(errno));
            } 
            else 
            {
                fprintf(log, 
                    "[brute] mmap %s size %zu offset %lld: success\n", 
                    path, sizes[s], (long long)offsets[o]);
                munmap(p, sizes[s]);
            }
        }
    }
    close(fd);
}

static void try_ioctl_fcntl(const char *path, FILE *log) 
{
    int fd = open(path, O_RDWR);
    if (fd < 0) 
        return;
    ioctl(fd, 0, NULL);
    fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, O_NONBLOCK);
    lseek(fd, 0, SEEK_SET);
    char buf[1];
    read(fd, buf, 1);
    write(fd, buf, 1);
    struct stat st;
    fstat(fd, &st);
    listxattr(path, NULL, 0, 0);
    getxattr(path, "", NULL, 0, 0, 0);
    struct pollfd pfd = 
    {
        fd, 
        POLLIN|POLLOUT, 
        0
    };
    poll(&pfd, 1, 10);
    close(fd);
}

static void brute_force_device_names(FILE *log) 
{
    char path[64];
    for (int i = 0; i < 100; ++i) 
    {
        snprintf(path, sizeof(path), "/dev/fb%d", i);
        try_open_device(path, log);
        try_mmap_device(path, log);
    }
    for (int i = 0; i < 100; ++i) 
    {
        snprintf(path, sizeof(path), "/dev/graphics%d", i);
        try_open_device(path, log);
        try_mmap_device(path, log);
    }
}

void graphics_bruteforce_all(FILE *log) 
{
    const char *patterns[] = 
    {
        "/dev/fb*", "/dev/graphics*", 
        "/dev/dri/*", "/dev/ttm*", 
        "/dev/video*", "/dev/display*", NULL
    };
    for (int i = 0; patterns[i]; ++i) 
    {
        glob_t g;
        if (glob(patterns[i], 0, NULL, &g) == 0) 
        {
            for (size_t j = 0; j < g.gl_pathc; ++j) 
            {
                try_open_device(g.gl_pathv[j], log);
                try_mmap_device(g.gl_pathv[j], log);
                try_ioctl_fcntl(g.gl_pathv[j], log);
            }
            globfree(&g);
        }
    }
    brute_force_device_names(log);
    const char *specials[] = 
    {
        "/dev/mem", "/dev/kmem", 
        "/dev/ttm", "/dev/zero", 
        "/dev/null", NULL};
    for (int i = 0; specials[i]; ++i) 
    {
        try_open_device(specials[i], log);
        try_mmap_device(specials[i], log);
    }
    char *sysctls[] = 
    {
        "hw.model", "hw.gpu", 
        "hw.graphics", "hw.framebuffer", 
        NULL};
    for (int i = 0; sysctls[i]; ++i) 
    {
        char buf[256]; 
        size_t len = sizeof(buf);
        if (sysctlbyname(sysctls[i], buf, &len, NULL, 0) == 0) 
        {
            fprintf(log, 
                "[brute] sysctl %s: %.*s\n", 
                sysctls[i], (int)len, buf);
        }
    }
    DIR *d = opendir("/System/Library/Extensions");
    if (d) 
    {
        struct dirent *e;
        while ((e = readdir(d)) != NULL) 
        {
            if (strstr(e->d_name, "framebuffer") || 
                strstr(e->d_name, "display") || 
                strstr(e->d_name, "graphics") || 
                strstr(e->d_name, "AGX")) 
            {
                fprintf(log, 
                    "[brute] Found kext: %s\n", 
                    e->d_name);
            }
        }
        closedir(d);
    }
}
