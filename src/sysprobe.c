#include "sysprobe.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/syscall.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <IOKit/IOKitLib.h>
#include <glob.h>

/*
TODO: Document this section extensively.
*/

static jmp_buf jmpbuf;
static volatile sig_atomic_t faulted = 0;

static void sigill_handler(int signo) 
{ 
    (void)signo; 
    faulted = 1; 
    longjmp(jmpbuf, 1); 
}
static void sigsegv_handler(int signo) 
{ 
    (void)signo; 
    faulted = 2; 
    longjmp(jmpbuf, 1); 
}

int sysprobe_mem_read(uint64_t start_addr, size_t length, void *out_buf) 
{
    int fd = open("/dev/mem", O_RDONLY);
    if (fd < 0) 
    {
        perror("open /dev/mem");
        return -1;
    }
    if (lseek(fd, (off_t)start_addr, SEEK_SET) == (off_t)-1) 
    {
        perror("lseek");
        close(fd);
        return -1;
    }
    ssize_t n = read(fd, out_buf, length);
    if (n < 0) 
    {
        perror("read");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

int sysprobe_mem_map(uint64_t addr, size_t length, void *out_buf) 
{
    int fd = open("/dev/mem", O_RDONLY);
    if (fd < 0) return -1;
    void *map = mmap(NULL, length, PROT_READ, MAP_SHARED, fd, addr);
    if (map == MAP_FAILED) 
    { 
        close(fd); return -1; 
    }
    memcpy(out_buf, map, length);
    munmap(map, length);
    close(fd);
    return 0;
}

int sysprobe_exec_opcodes(const uint8_t *code, size_t len, FILE *log) 
{
    fprintf(log, 
        "[sysprobe] Would execute %zu bytes of code at %p\n", 
        len, code);
    return 0;
}

// Try to execute a custom ARM64 instruction (dangerous!)
int sysprobe_exec_custom(const uint8_t *code, size_t len, FILE *log) 
{
    struct sigaction old_ill, old_segv, act = {0};
    act.sa_handler = sigill_handler;
    sigaction(SIGILL, &act, &old_ill);
    act.sa_handler = sigsegv_handler;
    sigaction(SIGSEGV, &act, &old_segv);
    faulted = 0;
    void *mem = mmap(NULL, len, 
        PROT_READ|PROT_WRITE|PROT_EXEC, 
        MAP_ANON|MAP_PRIVATE, -1, 0);
    int result = 0;
    if (mem == MAP_FAILED) 
    { 
        fprintf(log, "mmap failed\n"); 
        goto cleanup; 
    }
    memcpy(mem, code, len);
    if (setjmp(jmpbuf) == 0) 
    {
        ((void(*)())mem)();
        fprintf(log, "[sysprobe] Executed custom code, no fault\n");
    } 
    else 
    {
        fprintf(log, "[sysprobe] Faulted: %d\n", faulted);
        result = -1;
    }
    munmap(mem, len);
cleanup:
    sigaction(SIGILL, &old_ill, NULL);
    sigaction(SIGSEGV, &old_segv, NULL);
    return result;
}

int sysprobe_devfile(const char *path, FILE *log) 
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) 
    { 
        fprintf(log, "[sysprobe] open %s failed: %s\n", 
            path, strerror(errno)); 
            return -1; 
    }
    char buf[16];
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n > 0) 
    {
        fprintf(log, "[sysprobe] %s: ", path);
        for (int i = 0; i < n; ++i) 
            fprintf(log, "%02x ", (unsigned char)buf[i]);
        fprintf(log, "\n");
    }
    close(fd);
    return 0;
}

void sysprobe_dev_enumerate(FILE *log) 
{
    DIR *d = opendir("/dev");
    if (!d) 
    { 
        fprintf(log, "[sysprobe] Could not open /dev\n"); 
        return; 
    }
    struct dirent *entry;
    char path[512];
    int count = 0;
    while ((entry = readdir(d)) != NULL && count < 1000) 
    {
        snprintf(path, sizeof(path), "/dev/%s", entry->d_name);
        int fd = open(path, O_RDONLY);
        if (fd >= 0) 
        {
            char buf[16];
            ssize_t n = read(fd, buf, sizeof(buf));
            fprintf(log, "[sysprobe] %s: ", path);
            if (n > 0) 
            {
                for (ssize_t i = 0; i < n; ++i) 
                    fprintf(log, "%02x ", (unsigned char)buf[i]);
                fprintf(log, "\n");
            } 
            else 
            {
                fprintf(log, "(empty or unreadable)\n");
            }
            close(fd);
        } 
        else 
        {
            fprintf(log, "[sysprobe] open %s failed: %s\n", 
                path, strerror(errno));
        }
        count++;
    }
    closedir(d);
}

void sysprobe_dir_enumerate(const char *dirpath, FILE *log) 
{
    DIR *d = opendir(dirpath);
    if (!d) 
    { 
        fprintf(log, "[sysprobe] Could not open %s\n", dirpath); 
        return; 
    }
    struct dirent *entry;
    char path[1024];
    int count = 0;
    while ((entry = readdir(d)) != NULL && count < 1000) 
    {
        snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) 
        {
            int fd = open(path, O_RDONLY);
            if (fd >= 0) 
            {
                char buf[16];
                ssize_t n = read(fd, buf, sizeof(buf));
                fprintf(log, "[sysprobe] %s: ", path);
                if (n > 0) 
                {
                    for (ssize_t i = 0; i < n; ++i) 
                        fprintf(log, "%02x ", (unsigned char)buf[i]);
                    fprintf(log, "\n");
                } 
                else 
                {
                    fprintf(log, "(empty or unreadable)\n");
                }
                close(fd);
            } 
            else 
            {
                fprintf(log, "[sysprobe] open %s failed: %s\n", 
                    path, strerror(errno));
            }
        }
        count++;
    }
    closedir(d);
}

void sysprobe_recursive_dir(
    const char *dir, 
    void (*file_cb)(const char*, void*), 
    void *userdata, int depth) 
{
    if (depth > 8) return; 
    DIR *d = opendir(dir);
    if (!d) return;
    struct dirent *e;
    char path[1024];
    while ((e = readdir(d)) != NULL) 
    {
        if (e->d_name[0] == '.') 
            continue;
        snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
        struct stat st;
        if (stat(path, &st) == 0) 
        {
            if (S_ISREG(st.st_mode)) 
            {
                file_cb(path, userdata);
            } 
            else if (S_ISDIR(st.st_mode)) 
            {
                sysprobe_recursive_dir(path, file_cb, userdata, depth+1);
            }
        }
    }
    closedir(d);
}

uint64_t sysprobe_time_op(void (*fn)(void), FILE *log) 
{
    struct timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    fn();
    clock_gettime(CLOCK_MONOTONIC, &t2);
    uint64_t ns = 
        (t2.tv_sec-t1.tv_sec)*1000000000ULL 
        + (t2.tv_nsec-t1.tv_nsec);
    fprintf(log, "[sysprobe] op took %llu ns\n", ns);
    return ns;
}

void sysprobe_print_summary(FILE *out) 
{
    fprintf(out, 
    "[sysprobe] Probing summary: (implement logging of results here)\n");
}

void sysprobe_print_sysctl(const char *name, FILE *log) 
{
    char buf[1024];
    size_t len = sizeof(buf);
    if (sysctlbyname(name, buf, &len, NULL, 0) == 0) 
    {
        fprintf(log, "[sysctl] %s: %.*s\n", name, (int)len, buf);
    } 
    else 
    {
        fprintf(log, "[sysctl] %s: failed\n", name);
    }
}

void sysprobe_print_cpu_info(FILE *log) 
{
    sysprobe_print_sysctl("machdep.cpu.brand_string", log);
    sysprobe_print_sysctl("hw.ncpu", log);
    sysprobe_print_sysctl("hw.cpufrequency", log);
    sysprobe_print_sysctl("hw.cpufamily", log);
    sysprobe_print_sysctl("hw.cputype", log);
    sysprobe_print_sysctl("hw.cpusubtype", log);
}

void sysprobe_print_mem_info(FILE *log) 
{
    sysprobe_print_sysctl("hw.memsize", log);
}

void sysprobe_print_gpu_info(FILE *log) 
{
    sysprobe_print_sysctl("hw.model", log);
    sysprobe_print_sysctl("hw.gpu", log); // May not exist on all systems
}

void sysprobe_list_kexts(FILE *log) 
{
    system("kextstat | head -20"); // Print first 20 loaded kexts
}

void sysprobe_list_iokit_devices(FILE *log) 
{
    io_iterator_t iter;
    kern_return_t kr = 
        IOServiceGetMatchingServices(
            kIOMasterPortDefault, 
            IOServiceMatching("IOService"), 
            &iter);
    if (kr == KERN_SUCCESS) 
    {
        io_object_t obj;
        int count = 0;
        while ((obj = IOIteratorNext(iter)) && count < 20) 
        {
            CFStringRef name = 
                IORegistryEntryCreateCFProperty(obj, 
                    CFSTR("IOName"), kCFAllocatorDefault, 0);
            if (name) 
            {
                char cname[256];
                if (CFStringGetCString(name, 
                    cname, sizeof(cname), kCFStringEncodingUTF8)) 
                {
                    fprintf(log, "[iokit] Device: %s\n", cname);
                }
                CFRelease(name);
            }
            IOObjectRelease(obj);
            count++;
        }
        IOObjectRelease(iter);
    } 
    else 
    {
        fprintf(log, "[iokit] Failed to enumerate devices\n");
    }
}

void sysprobe_list_framebuffer_devices(FILE *log) 
{
    glob_t g;
    int found = 0;
    if (glob("/dev/fb*", 0, NULL, &g) == 0) 
    {
        for (size_t i = 0; i < g.gl_pathc; ++i) 
        {
            fprintf(log, 
                "[fb] Found framebuffer device: %s\n", 
                g.gl_pathv[i]);
            found = 1;
        }
        globfree(&g);
    }
    if (glob("/dev/graphics*", 0, NULL, &g) == 0) 
    {
        for (size_t i = 0; i < g.gl_pathc; ++i) 
        {
            fprintf(log, 
                "[fb] Found graphics device: %s\n", 
                g.gl_pathv[i]);
            found = 1;
        }
        globfree(&g);
    }
    if (glob("/dev/dri/*", 0, NULL, &g) == 0) 
    {
        for (size_t i = 0; i < g.gl_pathc; ++i) 
        {
            fprintf(log, "[fb] Found DRI device: %s\n", g.gl_pathv[i]);
            found = 1;
        }
        globfree(&g);
    }
    if (!found) fprintf(log, "[fb] No framebuffer/graphics devices found\n");
}

void sysprobe_search_graphics_devices(FILE *log) 
{
    const char *patterns[] = 
    {
        "/dev/fb*", "/dev/graphics*", 
        "/dev/dri/*", "/dev/ttm*", 
        "/dev/video*", "/dev/display*", 
        NULL
    };
    for (int i = 0; patterns[i]; ++i) 
    {
        glob_t g;
        if (glob(patterns[i], 0, NULL, &g) == 0) 
        {
            for (size_t j = 0; j < g.gl_pathc; ++j) 
            {
                fprintf(log, 
                    "[graphics_search] Found: %s\n", 
                    g.gl_pathv[j]);
            }
            globfree(&g);
        }
    }
}

void sysprobe_list_display_dirs(FILE *log) 
{
    const char *dirs[] = 
    {
        "/System/Library/Displays",
        "/Library/Displays",
        "/System/Library/Extensions",
        "/System/Library/Frameworks/CoreGraphics.framework/Versions/Current/Resources",
        NULL
    };
    for (int i = 0; dirs[i]; ++i) 
    {
        struct stat st;
        if (stat(dirs[i], &st) == 0 && S_ISDIR(st.st_mode)) 
        {
            fprintf(log,
                 "[display] Found display/config dir: %s\n", 
                 dirs[i]);
        }
    }
}

void sysprobe_log_findings(const char *msg, FILE *log) 
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char timestr[64];
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", tm);
    fprintf(log, "[%s] %s\n", timestr, msg);
}

void sysprobe_save_report(const char *filename, const char *content) 
{
    FILE *f = fopen(filename, "w");
    if (f) 
    {
        fputs(content, f);
        fclose(f);
    }
}

void sysprobe_prepare_graphics_test(const char *device, FILE *log) 
{
    fprintf(log, 
        "[graphics] Would attempt to mmap or write to device: %s\n", 
        device);
}

void sysprobe_prepare_arm64_asm(FILE *log) 
{
    fprintf(log, 
        "[asm] Would prepare ARM64 assembly for register/memory access\n");
}
