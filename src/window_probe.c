#include "window_probe.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif

/*
TODO: Document this section extensively.
*/

static const char* explain_errno(int err, const char **desc) 
{
    switch (err) 
    {
        case EACCES: *desc = 
            "Permission denied. You lack the required privileges."; 
            return "EACCES";
        case ENOENT: *desc = 
            "No such file or directory. Device path does not exist."; 
            return "ENOENT";
        case ENOTTY: *desc = 
            "Not a typewriter. Device does not support requested operation."; 
            return "ENOTTY";
        case EBUSY:  *desc = 
            "Device or resource busy. Device is in use."; 
            return "EBUSY";
        case EINVAL: *desc = 
            "Invalid argument. Parameters or flags are not supported."; 
            return "EINVAL";
        case EPERM:  *desc = 
            "Operation not permitted. Insufficient permissions."; 
            return "EPERM";
        case ENODEV: *desc = 
            "No such device. Device does not exist or is not configured."; 
            return "ENODEV";
        case ENOMEM: *desc = 
            "Out of memory. mmap or allocation failed."; 
            return "ENOMEM";
        case EIO:    *desc = 
            "I/O error. Hardware or driver failure."; 
            return "EIO";
        default:     *desc = 
            "Unknown error."; 
            return "UNKNOWN";
    }
}

// Intensified brute-force window probing
int window_probe_attempt(const char *device, 
    int width, int height, FILE *log) 
{
    struct stat st;
    if (stat(device, &st) != 0) 
    {
        const char *desc; 
        const char *ename = explain_errno(errno, &desc);
        fprintf(log, 
            "[window_probe] stat() failed for %s: %s (%s)\n", 
            device, ename, desc);
        return -1;
    }
    int open_flags[] = 
    { 
        O_RDWR, 
        O_WRONLY, 
        O_RDONLY 
    };
    int mmap_prots[] = 
    { 
        PROT_READ|PROT_WRITE, 
        PROT_READ, 
        PROT_WRITE 
    };
    int mmap_flags[] = 
    { 
        MAP_SHARED, 
        MAP_PRIVATE 
    };
    int fd = -1, open_err = 0, mmap_err = 0, found = 0;
    for (int f = 0; f < 3; ++f) 
    {
        fd = open(device, open_flags[f]);
        if (fd < 0) 
        {
            const char *desc; 
            const char *ename = 
            explain_errno(errno, &desc);
            fprintf(log, 
                "[window_probe] open(%s, 0x%x) failed: %s (%s)\n", 
                device, open_flags[f], ename, desc);
            open_err = errno;
            continue;
        }
        off_t size = width * height * 4;
        for (int p = 0; p < 3; ++p) 
        {
            for (int m = 0; m < 2; ++m) 
            {
                void *fb = mmap(NULL, 
                    size, mmap_prots[p], 
                    mmap_flags[m], fd, 0);
                if (fb == MAP_FAILED) 
                {
                    const char *desc; 
                    const char *ename = explain_errno(errno, &desc);
                    fprintf(log, 
                        "[window_probe] mmap(NULL, %lld, 0x%x, 0x%x, fd, 0) failed: %s (%s)\n", 
                        (long long)size, mmap_prots[p], mmap_flags[m], ename, desc);
                    mmap_err = errno;
                    continue;
                }
#ifdef FBIOGET_VSCREENINFO
                struct fb_var_screeninfo vinfo;
                if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == 0) 
                {
                    fprintf(log, 
                        "[window_probe] ioctl FBIOGET_VSCREENINFO succeeded.\n");
                } 
                else 
                {
                    const char *desc; 
                    const char *ename = explain_errno(errno, &desc);
                    fprintf(log, 
                        "[window_probe] ioctl FBIOGET_VSCREENINFO failed: %s (%s)\n", 
                        ename, desc);
                }
#endif
                // Write test pattern
                unsigned char *pbuf = (unsigned char*)fb;
                for (int y = 0; y < height; ++y) 
                {
                    for (int x = 0; x < width; ++x) 
                    {
                        int idx = (y * width + x) * 4;
                        pbuf[idx+0] = (x ^ y) & 0xFF;
                        pbuf[idx+1] = (x * 2) & 0xFF;
                        pbuf[idx+2] = (y * 2) & 0xFF;
                        pbuf[idx+3] = 0xFF;
                    }
                }
                if (msync(fb, size, MS_SYNC) != 0) 
                {
                    const char *desc; 
                    const char *ename = explain_errno(errno, &desc);
                    fprintf(log, 
                        "[window_probe] msync failed: %s (%s)\n", 
                        ename, desc);
                }
                munmap(fb, size);
                found = 1;
                fprintf(log, 
                    "[window_probe] SUCCESS: Wrote test pattern to %s (flags 0x%x, prot 0x%x, mmap 0x%x)\n", 
                    device, open_flags[f], mmap_prots[p], mmap_flags[m]);
            }
        }
        close(fd);
    }
    if (!found) 
    {
        fprintf(log, 
            "[window_probe] All attempts failed for %s. Last open error: %d, mmap error: %d\n", 
            device, open_err, mmap_err);
    }
    return found ? 0 : -2;
}

// Try privilege escalation and log results
void try_privilege_escalation(FILE *log) 
{
    fprintf(log, "[escalation] Attempting setuid(0)...\n");
    if (setuid(0) == 0) 
        fprintf(log, "[escalation] setuid(0) succeeded!\n");
    else 
        fprintf(log, 
            "[escalation] setuid(0) failed: %s\n", 
            strerror(errno));
        fprintf(log, 
            "[escalation] Attempting seteuid(0)...\n");
    if (seteuid(0) == 0) 
        fprintf(log, 
            "[escalation] seteuid(0) succeeded!\n");
    else 
        fprintf(log, 
            "[escalation] seteuid(0) failed: %s\n", 
            strerror(errno));
#ifdef PR_SET_KEEPCAPS
    fprintf(log, 
        "[escalation] Attempting prctl(PR_SET_KEEPCAPS, 1)...\n");
    if (prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0) == 0) 
        fprintf(log, 
            "[escalation] prctl(PR_SET_KEEPCAPS) succeeded!\n");
    else 
        fprintf(log, 
            "[escalation] prctl(PR_SET_KEEPCAPS) failed: %s\n", 
            strerror(errno));
#endif
}

// Attempt to open a window using register searching and log all attempts/errors
void window_probe_bruteforce(
    const char **devices, int num_devices, 
    int width, int height, const char *logfile) 
{
    FILE *log = fopen(logfile, "w");
    if (!log) 
    {
        perror("Cannot open window probe log file");
        return;
    }
    try_privilege_escalation(log);
    fprintf(log, 
        "[window_probe_bruteforce] Starting window probing...\n");
    int found = 0;
    for (int i = 0; i < num_devices; ++i) 
    {
        fprintf(log, 
            "[window_probe_bruteforce] Attempting device: %s\n", 
            devices[i]);
        int res = window_probe_attempt(devices[i], width, height, log);
        if (res == 0) 
        {
            fprintf(log, 
                "[window_probe_bruteforce] SUCCESS: Device %s accepted test pattern.\n", 
                devices[i]);
            found = 1;
            break;
        } 
        else 
        {
            fprintf(log, 
                "[window_probe_bruteforce] FAILURE: Device %s, error code %d.\n", 
                devices[i], res);
        }
    }
    if (!found) 
    {
        fprintf(log, 
            "[window_probe_bruteforce] No device accepted the test pattern.\n");
    }
    fclose(log);
}

int enumerate_dev_candidates(char candidates[][256], int max_candidates) 
{
    DIR *d = opendir("/dev");
    if (!d) return 0;
    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(d)) && count < max_candidates) 
    {
        if (strstr(entry->d_name, "fb") || 
                strstr(entry->d_name, "graphics") ||
            strstr(entry->d_name, "card") || 
                strstr(entry->d_name, "tty") ||
            strstr(entry->d_name, "drm") || 
                strstr(entry->d_name, "video") ||
            strstr(entry->d_name, "block")) 
        {
            snprintf(candidates[count], 256, "/dev/%s", entry->d_name);
            ++count;
        }
    }
    closedir(d);
    return count;
}