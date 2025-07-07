#include "macho_probe.h"
#include "sysprobe.h"
#include <mach-o/loader.h>
#include <mach-o/fat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <mach-o/dyld.h>

/*
TODO: Document this section extensively.
*/

void macho_print_info(const char *path, FILE *log) 
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) 
    { 
        fprintf(log, "[macho] open %s failed: %s\n", 
            path, strerror(errno)); return; 
    }
    uint8_t buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n < 4) 
    { 
        fprintf(log, "[macho] %s: too small\n", 
            path); close(fd); return; 
    }
    uint32_t magic = *(uint32_t*)buf;
    if (magic == FAT_CIGAM || magic == FAT_MAGIC) 
    {
        struct fat_header *fh = (struct fat_header*)buf;
        fprintf(log, "[macho] %s: FAT binary, narch=%u\n", 
            path, ntohl(fh->nfat_arch));
    } 
    else if (magic == MH_MAGIC_64 || magic == MH_CIGAM_64) 
    {
        struct mach_header_64 *mh = (struct mach_header_64*)buf;
        fprintf(log, 
            "[macho] %s: 64-bit Mach-O, cpu=%x, type=%x, ncmds=%u\n", 
            path, mh->cputype, mh->filetype, mh->ncmds);
    } 
    else if (magic == MH_MAGIC || magic == MH_CIGAM) 
    {
        struct mach_header *mh = (struct mach_header*)buf;
        fprintf(log,
            "[macho] %s: 32-bit Mach-O, cpu=%x, type=%x, ncmds=%u\n", 
            path, mh->cputype, mh->filetype, mh->ncmds);
    } 
    else 
    {
        fprintf(log, "[macho] %s: not a Mach-O\n", path);
    }
    close(fd);
}

int macho_patch_first_byte(const char *path, uint8_t new_byte, FILE *log) 
{
    int fd = open(path, O_RDWR);
    if (fd < 0) 
    { 
        fprintf(log, "[patch] open %s failed: %s\n", 
            path, strerror(errno)); return -1; 
    }
    uint8_t orig;
    if (read(fd, &orig, 1) != 1) 
    { 
        fprintf(log, "[patch] read %s failed\n", 
            path); 
            close(fd); 
            return -1; 
    }
    lseek(fd, 0, SEEK_SET);
    if (write(fd, &new_byte, 1) != 1) 
    { 
        fprintf(log, "[patch] write %s failed\n", path); 
        close(fd); 
        return -1; 
    }
    fprintf(log, "[patch] Patched %s: %02x -> %02x\n", 
        path, orig, new_byte);
    close(fd);
    return 0;
}

void macho_compare_dirs(const char *dir1, const char *dir2, FILE *log) 
{
    DIR *d1 = opendir(dir1);
    DIR *d2 = opendir(dir2);
    if (!d1 || !d2) 
    { 
        fprintf(log, "[compare] Could not open directories\n"); 
        if (d1) closedir(d1); 
        if (d2) closedir(d2); 
        return; 
    }
    struct dirent *e1;
    while ((e1 = readdir(d1)) != NULL) 
    {
        if (e1->d_name[0] == '.') continue;
        char path1[512], path2[512];
        snprintf(path1, sizeof(path1), 
        "%s/%s", dir1, e1->d_name);
        snprintf(path2, sizeof(path2), 
        "%s/%s", dir2, e1->d_name);
        struct stat st1, st2;

        if (stat(path1, &st1) == 0 && 
        S_ISREG(st1.st_mode) && 
        stat(path2, &st2) == 0 && 
        S_ISREG(st2.st_mode)) 
        {
            int fd1 = open(path1, O_RDONLY), fd2 = open(path2, O_RDONLY);
            if (fd1 >= 0 && fd2 >= 0) 
            {
                uint8_t buf1[4096], buf2[4096];
                ssize_t n1 = read(fd1, buf1, sizeof(buf1));
                ssize_t n2 = read(fd2, buf2, sizeof(buf2));
                if (n1 > 4 && n2 > 4 && 
                    *(uint32_t*)buf1 == *(uint32_t*)buf2) 
                {
                    if (memcmp(buf1, buf2, 32) != 0) 
                    {
                        fprintf(log, 
                            "[compare] %s differs in Mach-O header\n", 
                            e1->d_name);
                    }
                }
                close(fd1); close(fd2);
            }
        }
    }
    closedir(d1); closedir(d2);
}

void macho_extract_dir(const char *dir, FILE *log) 
{
    DIR *d = opendir(dir);
    if (!d) 
    { 
        fprintf(log, "[extract] Could not open %s\n", dir); 
        return; 
    }
    struct dirent *e;
    char path[512];
    while ((e = readdir(d)) != NULL) 
    {
        if (e->d_name[0] == '.') 
            continue;
        snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) 
        {
            macho_print_info(path, log);
        }
    }
    closedir(d);
}

void macho_deep_probe(const char *dir, FILE *log) 
{
    DIR *d = opendir(dir);
    if (!d) 
    { 
        fprintf(log, "[deep] Could not open %s\n", dir); 
        return; 
    }
    struct dirent *e;
    char path[512];
    while ((e = readdir(d)) != NULL) 
    {
        if (e->d_name[0] == '.') continue;
        snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) 
        {
            int fd = open(path, O_RDONLY);
            if (fd >= 0) 
            {
                uint8_t buf[4096];
                ssize_t n = read(fd, buf, sizeof(buf));
                if (n > 4) 
                {
                    uint32_t magic = *(uint32_t*)buf;
                    if (magic == FAT_CIGAM || magic == FAT_MAGIC || 
                        magic == MH_MAGIC_64 || magic == MH_CIGAM_64 || 
                        magic == MH_MAGIC || magic == MH_CIGAM) 
                    {
                        fprintf(log, "[deep] Mach-O found: %s\n", path);
                        macho_print_info(path, log);
                    }
                }
                close(fd);
            }
        }
    }
    closedir(d);
}

void macho_print_load_commands(const char *path, FILE *log) 
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) 
    { 
        fprintf(log, "[macho] open %s failed: %s\n", 
            path, strerror(errno)); 
            return; 
    }
    uint8_t buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n < 4) 
    { 
        fprintf(log, "[macho] %s: too small\n", path); 
        close(fd); 
        return; 
    }
    uint32_t magic = *(uint32_t*)buf;
    if (magic == MH_MAGIC_64 || magic == MH_CIGAM_64) 
    {
        struct mach_header_64 *mh = (struct mach_header_64*)buf;
        struct load_command *lc = 
            (struct load_command*)(buf + sizeof(struct mach_header_64));
        fprintf(log, "[macho] %s: Load Commands:\n", path);
        for (uint32_t i = 0; i < mh->ncmds; ++i) 
        {
            fprintf(log, "  cmd=0x%x, size=%u\n", lc->cmd, lc->cmdsize);
            lc = (struct load_command*)((uint8_t*)lc + lc->cmdsize);
        }
    } 
    else 
    {
        fprintf(log, "[macho] %s: Not a 64-bit Mach-O\n", path);
    }
    close(fd);
}

void macho_print_signature(const char *path, FILE *log) 
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) 
    { 
        fprintf(log, "[macho] open %s failed: %s\n", 
            path, strerror(errno)); 
            return; 
    }
    uint8_t buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n < 4) 
    { 
        fprintf(log, "[macho] %s: too small\n", path); 
        close(fd); 
        return; 
    }
    uint32_t magic = *(uint32_t*)buf;
    if (magic == MH_MAGIC_64 || magic == MH_CIGAM_64) 
    {
        struct mach_header_64 *mh = (struct mach_header_64*)buf;
        struct load_command *lc = 
            (struct load_command*)(buf + 
                sizeof(struct mach_header_64));
        for (uint32_t i = 0; i < mh->ncmds; ++i) 
        {
            if (lc->cmd == 0x1d) 
            {
                fprintf(log, "[macho] %s: Has code signature\n", path);
                close(fd);
                return;
            }
            lc = (struct load_command*)((uint8_t*)lc + lc->cmdsize);
        }
        fprintf(log, "[macho] %s: No code signature\n", path);
    } 
    else 
    {
        fprintf(log, "[macho] %s: Not a 64-bit Mach-O\n", path);
    }
    close(fd);
}

void macho_list_linked_libs(const char *path, FILE *log) 
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) 
    { 
        fprintf(log, "[macho] open %s failed: %s\n", 
            path, strerror(errno)); 
            return; 
    }
    uint8_t buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n < 4) 
    { 
        fprintf(log, "[macho] %s: too small\n", path); 
        close(fd); 
        return; 
    }
    uint32_t magic = *(uint32_t*)buf;
    if (magic == MH_MAGIC_64 || magic == MH_CIGAM_64) 
    {
        struct mach_header_64 *mh = (struct mach_header_64*)buf;
        struct load_command *lc = 
        (struct load_command*)(buf + sizeof(struct mach_header_64));
        for (uint32_t i = 0; i < mh->ncmds; ++i) 
        {
            if (lc->cmd == 0xc || lc->cmd == 0x80000018) 
            {
                struct dylib_command *dc = (struct dylib_command*)lc;
                const char *name = (const char*)lc + dc->dylib.name.offset;
                fprintf(log, "[macho] %s: Linked library: %s\n", path, name);
            }
            lc = (struct load_command*)((uint8_t*)lc + lc->cmdsize);
        }
    } 
    else 
    {
        fprintf(log, "[macho] %s: Not a 64-bit Mach-O\n", path);
    }
    close(fd);
}

void macho_list_architectures(const char *path, FILE *log) 
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) 
    { 
        fprintf(log, "[macho] open %s failed: %s\n", 
            path, strerror(errno)); return; 
    }
    uint8_t buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n < 4) 
    { 
        fprintf(log, "[macho] %s: too small\n", path); 
        close(fd); 
        return; 
    }
    uint32_t magic = *(uint32_t*)buf;
    if (magic == FAT_MAGIC || magic == FAT_CIGAM) 
    {
        struct fat_header *fh = (struct fat_header*)buf;
        uint32_t narch = ntohl(fh->nfat_arch);
        fprintf(log, 
            "[macho] %s: Universal binary with %u architectures\n", 
            path, narch);
        struct fat_arch *arch = 
            (struct fat_arch*)(buf + sizeof(struct fat_header));
        for (uint32_t i = 0; i < narch; ++i) 
        {
            fprintf(log, 
                "  arch %u: cputype=0x%x, cpusubtype=0x%x\n", 
                i, ntohl(arch->cputype), ntohl(arch->cpusubtype));
            arch++;
        }
    } 
    else if (magic == MH_MAGIC_64 || magic == MH_CIGAM_64) 
    {
        struct mach_header_64 *mh = (struct mach_header_64*)buf;
        fprintf(log, 
            "[macho] %s: Single arch, cputype=0x%x, cpusubtype=0x%x\n", 
            path, mh->cputype, mh->cpusubtype);
    } 
    else 
    {
        fprintf(log, "[macho] %s: Not a Mach-O\n", path);
    }
    close(fd);
}

static void macho_probe_cb(const char *path, void *logptr) 
{
    FILE *log = (FILE*)logptr;
    int fd = open(path, O_RDONLY);
    if (fd < 0) return;
    uint8_t buf[4];
    if (read(fd, buf, 4) == 4) 
    {
        uint32_t magic = *(uint32_t*)buf;
        if (magic == FAT_MAGIC || magic == FAT_CIGAM || 
            magic == MH_MAGIC_64 || magic == MH_CIGAM_64 || 
            magic == MH_MAGIC || magic == MH_CIGAM) 
        {
            fprintf(log, "[deep] Mach-O found: %s\n", path);
            macho_print_info(path, log);
        }
    }
    close(fd);
}

void macho_recursive_scan(const char *dir, FILE *log) 
{
    sysprobe_recursive_dir(dir, macho_probe_cb, log, 0);
}

static void dylib_cb(const char *path, void *logptr) 
{
    FILE *log = (FILE*)logptr;
    if (strstr(path, ".dylib")) 
    {
        macho_print_info(path, log);
    }
}

void macho_scan_dylibs(const char *dir, FILE *log) 
{
    sysprobe_recursive_dir(dir, dylib_cb, log, 0);
}

static void agx_cb(const char *path, void *logptr) 
{
    FILE *log = (FILE*)logptr;
    if (strstr(path, "AGX") || strstr(path, "agx")) 
    {
        macho_print_info(path, log);
    }
}

void macho_scan_agx(const char *dir, FILE *log) 
{
    sysprobe_recursive_dir(dir, agx_cb, log, 0);
}
