#include "c_lib.h"
#include "mach_lib.h"
#include "net_lib.h"
#include "syscall.h"
#include "framebuffer.h"
#include "render.h"
#include "armOpCodes.h"
#include "sysprobe.h"
#include "macho_probe.h"
#include "window_probe.h"
#include "graphics_bruteforce.h"

#ifndef __kernel_ptr_semantics
#define __kernel_ptr_semantics
#endif

/*
TODO: Document this section extensively.
*/

static void dummy_fn(void);

static Framebuffer* fb = NULL;
static int kq = -1;
static struct kevent change;
static struct kevent event;
static Pixel red = {255, 0, 0};
static Pixel black = {0, 0, 0};

//Testing our buffer into Apple Hardware

/*
    This is the true test of whether or not we have successfully worked
    with said registers. These values are the stuff we need, or else
    we cannot get the GPU to do math for us at whim.

    Any problems here are essential if we want to access the GPU directly.
*/
uint8_t test_code[16] = 
{
    /*
    Some important notes, 0x85 is byte 2 and 0x08 is byte 3
    As a reminder, the 0x08 byte is shared, meaning that byte 4 must be
    something that accounts for the shared byte of 3 in order for floating
    point to work.
    */
    0xAA, 0x81,
    0x85, 0x08,
    0xA0, 0xA0,
    0x96, 0x02, 
    0x45, 0x36, 
    0x27, 0x00, 
    0x00, 0x80, 
    0x00, 0x00 
};

// Initialize the framebuffer and event queue
int init_system(void)
{
    fb = fb_create(640, 480);
    if (!fb) 
    {
        fprintf(stderr, 
            "Failed to create framebuffer\n");
        return -1;
    }
    
    kq = kqueue();
    if (kq == -1) 
    {
        perror("kqueue");
        fb_destroy(fb);
        return -1;
    }
    
    EV_SET(&change, STDIN_FILENO, EVFILT_READ, EV_ADD, 0, 0, NULL);
    
    return 0;
}

int process_input(void)
{
    int nev = kevent(kq, &change, 1, &event, 1, NULL);
    if (nev == -1) 
    {
        perror("kevent");
        return -1;
    }
    
    if (nev > 0 && event.filter == EVFILT_READ) 
    {
        char buffer[128];
        int n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
        
        if (n > 0) 
        {
            buffer[n] = '\0';
            printf("Input received: %s\n", buffer);
            
            if (buffer[0] == 'd') 
            {
                render_clear(fb, black);
                render_rect(fb, 100, 100, 50, 50, red);
                fb_save_ppm(fb, "output.ppm");
                printf("Framebuffer saved to output.ppm\n");
            }
            else if (buffer[0] == 'a')
            {
                printf("Here are some relevant options:\n");
                printf("a - Runs agx_disassemble_instr\n");
                printf("b - Runs agx_print_fadd_f32\n");
                printf("c - Runs agx_print_ld_compute\n");
                printf("d - Runs agx_print_src\n");
                printf("e - Runs agx_print_float_src\n");
                printf("f - Runs agx_instr_bytes\n");
                printf("z - Runs agx_disassemble\n");
                printf("h - Print Mach-O header info for a file\n");
                printf("p - Patch first byte of a Mach-O binary (dangerous, research only!)\n");
                printf("l - List load commands for a Mach-O file\n");
                printf("s - Show code signature presence for a Mach-O file\n");
                printf("b - List linked libraries for a Mach-O file\n");
                printf("a - List architectures in a Mach-O file\n");
                printf("Enter option: \n");

                char suboption[128];
                int m = read(STDIN_FILENO, 
                    suboption, sizeof(suboption) - 1);

                if (m > 0)
                {
                    suboption[m] = '\0';
                    
                    /*
                    There are so many issues with this code in particular,
                    but that has more to do with making sure I am doing stuff correctly.

                    This is an entirely new territory, so it's a lot of work to
                    find out exactly how to communicate with the OS. It's also
                    a lot of fun to brainstorm these fixes.

                    The important part here is that these run the commands I've setup
                    in my OpCode files. They all execute, meaning I can begin
                    to diagnose some of the problems when working with these
                    */
                    if (suboption[0] == 'a')
                    {
                        bool stop = false;
                        printf("Testing one instruction\n");
                        agx_disassemble_instr(
                            test_code, &stop, true, stdout);
                        printf("\n");
                    }

                    if (suboption[0] == 'b')
                    {
                        printf("Testing floating point addition\n");
                        agx_print_fadd_f32(stdout, test_code);
                        printf("\n");
                    }
                    if (suboption[0] == 'c')
                    {
                        printf("Testing M2's ARM load compute");
                        agx_print_ld_compute(test_code, stdout);
                        printf("\n");
                    }
                    if (suboption[0] == 'd')
                    {
                        printf("Testing source printing\n");
                        struct agx_src test_src = 
                            {
                                .type = 0,
                                .reg = 42,
                                .size32 = true,
                                .abs = false,
                                .neg = false,
                                .unk = 0
                            };
                        agx_print_src(stdout, test_src);
                        printf("\n");
                    }
                    if (suboption[0] == 'e')
                    {
                        printf("Testing floating point source printing\n");
                        agx_print_float_src(stdout, 0, 42, true, false, false);
                        printf("\n");
                    }
                    if (suboption[0] == 'f')
                    {
                        printf("Testing instruction byte length\n");
                        agx_instr_bytes(0, 42);
                        printf("\n");
                    }

                    if (suboption[0] == 'z')
                    {
                        printf("Disassembly of test code: \n");
                        agx_disassemble(test_code, sizeof(test_code), stdout);
                        printf("\n");
                    }
                    else if (suboption[0] == 'h') 
                    {
                        char path[256];
                        printf("Enter path to Mach-O file: ");
                        fflush(stdout);
                        int pn = read(STDIN_FILENO, 
                            path, sizeof(path) - 1);
                        if (pn > 0) 
                        {
                            path[pn] = '\0';
                            char *nl = strchr(path, '\n');
                            if (nl) *nl = '\0';
                        }
                    } 
                    else if (suboption[0] == 'p') 
                    {
                        char path[256];
                        printf("Enter path to Mach-O file to patch: ");
                        fflush(stdout);
                        int pn = read(STDIN_FILENO, 
                            path, sizeof(path) - 1);
                        if (pn > 0) 
                        {
                            path[pn] = '\0';
                            char *nl = strchr(path, '\n');
                            if (nl) *nl = '\0';
                            printf(
                                "Enter new byte value (hex, e.g. ca): ");
                            char hex[8];
                            printf(
                                "Enter new first byte in hex (e.g. 90): ");
                            fflush(stdout);
                            int hn = read(STDIN_FILENO, 
                                hex, sizeof(hex) - 1);
                            if (hn > 0) 
                            { 
                                hex[hn] = '\0'; 
                                char *nl = strchr(hex, '\n'); 
                                if (nl) *nl = '\0'; 
                            }
                            uint8_t val = 
                                (uint8_t)strtol(hex, 
                                    NULL, 16);
                            macho_patch_first_byte(path, 
                                val, stdout);
                        }
                    } 
                    else if (suboption[0] == 'l') 
                    {
                        char path[256];
                        printf("Enter path to Mach-O file: ");
                        fflush(stdout);
                        int pn = read(STDIN_FILENO, 
                            path, sizeof(path) - 1);
                        if (pn > 0) 
                        {
                            path[pn] = '\0';
                            char *nl = strchr(path, '\n');
                            if (nl) *nl = '\0';
                            macho_print_load_commands(path, stdout);
                        }
                    } 
                    else if (suboption[0] == 's') 
                    {
                        char path[256];
                        printf("Enter path to Mach-O file: ");
                        fflush(stdout);
                        int pn = read(STDIN_FILENO, path, sizeof(path) - 1);
                        if (pn > 0) 
                        {
                            path[pn] = '\0';
                            char *nl = strchr(path, '\n');
                            if (nl) *nl = '\0';
                            macho_print_signature(path, stdout);
                        }
                    } 
                    else if (suboption[0] == 'b') 
                    {
                        char path[256];
                        printf("Enter path to Mach-O file: ");
                        fflush(stdout);
                        int pn = read(STDIN_FILENO, 
                            path, sizeof(path) - 1);
                        if (pn > 0) 
                        {
                            path[pn] = '\0';
                            char *nl = strchr(path, '\n');
                            if (nl) *nl = '\0';
                            macho_list_linked_libs(path, stdout);
                        }
                    } 
                    else if (suboption[0] == 'a') 
                    {
                        char path[256];
                        printf("Enter path to Mach-O file: ");
                        fflush(stdout);
                        int pn = read(STDIN_FILENO, 
                            path, sizeof(path) - 1);
                        if (pn > 0) 
                        {
                            path[pn] = '\0';
                            char *nl = strchr(path, '\n');
                            if (nl) *nl = '\0';
                            macho_list_architectures(path, stdout);
                        }
                    }
                }
                
            }
            else if (buffer[0] == 'q') 
            {
                printf("Exiting program...\n");
                return 1; 
            }
            else if (buffer[0] == 'p') 
            {
                printf(
                    "[sysprobe] Attempting memory read and opcode execution...\n");
                uint8_t mem_buf[32] = {0};
                if (sysprobe_mem_read(
                    0x0, sizeof(mem_buf), 
                    mem_buf) == 0) 
                {
                    printf("[sysprobe] Memory read from 0x0: ");
                    for (size_t i = 0; i < sizeof(mem_buf); ++i) 
                        printf("%02x ", mem_buf[i]);
                    printf("\n");
                } 
                else 
                {
                    printf(
                        "[sysprobe] Memory read failed (likely due to permissions)\n");
                }
                sysprobe_exec_opcodes(test_code, 
                    sizeof(test_code), stdout);
                sysprobe_print_summary(stdout);
            }
            else if (buffer[0] == 's')
            {
                printf("Sysprobe Menu:\n");
                printf("m - Map and read memory region\n");
                printf("i - Execute custom ARM64 instruction\n");
                printf("y - Try undocumented syscall\n");
                printf("d - Probe device files\n");
                printf("t - Timing/performance probe\n");
                printf("e - Enumerate and probe all /dev device files\n");
                printf("f - Enumerate and probe all files in a directory\n");
                printf("Enter sysprobe option: ");
                char sysopt[128];
                int sm = read(STDIN_FILENO, sysopt, sizeof(sysopt) - 1);
                if (sm > 0) 
                {
                    sysopt[sm] = '\0';
                    if (sysopt[0] == 'm') 
                    {
                        uint8_t buf[16] = {0};
                        if (sysprobe_mem_map(
                            0x0, sizeof(buf), buf) == 0) 
                        {
                            printf("[sysprobe] Mapped 0x0: ");
                            for (size_t i = 0; i < sizeof(buf); ++i) 
                                printf("%02x ", buf[i]);
                                printf("\n");
                        } 
                        else 
                        {
                            printf("[sysprobe] Memory map failed\n");
                        }
                    } 
                    else if (sysopt[0] == 'i') 
                    {
                        printf(
                            "[sysprobe] Executing custom ARM64 instruction (may crash!)\n");
                        sysprobe_exec_custom(test_code, 
                            sizeof(test_code), stdout);
                    } 
                    else if (sysopt[0] == 'y') 
                    {
                        printf(
                            "[sysprobe] Syscall probing is not supported on macOS ARM (Ventura).\n");
                    } 
                    else if (sysopt[0] == 'd') 
                    {
                        printf("[sysprobe] Probing /dev/mem, /dev/kmem, /dev/ttm, /dev/zero, /dev/random\n");
                        sysprobe_devfile("/dev/mem", stdout);
                        sysprobe_devfile("/dev/kmem", stdout);
                        sysprobe_devfile("/dev/ttm", stdout);
                        sysprobe_devfile("/dev/zero", stdout);
                        sysprobe_devfile("/dev/random", stdout);
                    } 
                    else if (sysopt[0] == 'e') 
                    {
                        printf(
                            "[sysprobe] Enumerating and probing all /dev device files (first 1000)...\n");
                        sysprobe_dev_enumerate(stdout);
                    } 
                    else if (sysopt[0] == 'f') 
                    {
                        char dir[256];
                        printf(
                            "Enter directory to probe (e.g. /, /etc, /bin, /usr/bin, /System, /dev): ");
                        fflush(stdout);
                        int dn = read(STDIN_FILENO, 
                            dir, sizeof(dir) - 1);
                        if (dn > 0) 
                        {
                            dir[dn] = '\0';
                            char *nl = strchr(dir, '\n');
                            if (nl) *nl = '\0';
                            printf(
                                "[sysprobe] Enumerating and probing all files in %s (first 1000)...\n", 
                                dir);
                            sysprobe_dir_enumerate(dir, stdout);
                        }
                    } 
                    else 
                    {
                        printf("[sysprobe] Unknown sysprobe option\n");
                    }
                }
            }
            else if (buffer[0] == 'm')
            {
                printf("Mach-O Menu:\n");
                printf("h - Print Mach-O header info for a file\n");
                printf("p - Patch first byte of a Mach-O binary (dangerous, research only!)\n");
                printf("c - Compare Mach-O headers in two directories\n");
                printf("a - Automated extraction of Mach-O info from a directory\n");
                printf("d - Deeper probing: scan for Mach-O binaries and analyze\n");
                printf("l - List load commands for a Mach-O file\n");
                printf("s - Show code signature presence for a Mach-O file\n");
                printf("b - List linked libraries for a Mach-O file\n");
                printf("Enter Mach-O option: ");
                char machopt[128];
                int mn = read(STDIN_FILENO,
                    machopt, sizeof(machopt) - 1);
                if (mn > 0) 
                {
                    machopt[mn] = '\0';
                    if (machopt[0] == 'h') 
                    {
                        char path[256];
                        printf("Enter path to Mach-O file: ");
                        fflush(stdout);
                        int pn = read(STDIN_FILENO, 
                            path, sizeof(path) - 1);
                        if (pn > 0) 
                        {
                            path[pn] = '\0';
                            char *nl = strchr(path, '\n');
                            if (nl) *nl = '\0';
                            macho_print_info(path, stdout);
                        }
                    } 
                    else if (machopt[0] == 'p') 
                    {
                        char path[256];
                        printf("Enter path to Mach-O file to patch: ");
                        fflush(stdout);
                        int pn = read(STDIN_FILENO, 
                            path, sizeof(path) - 1);
                        if (pn > 0) 
                        {
                            path[pn] = '\0';
                            char *nl = strchr(path, '\n');
                            if (nl) *nl = '\0';
                            printf(
                                "Enter new byte value (hex, e.g. ca): ");
                            char hex[8];
                            printf(
                                "Enter new first byte in hex (e.g. 90): ");
                            fflush(stdout);
                            int hn = read(STDIN_FILENO, 
                                hex, sizeof(hex) - 1);
                            if (hn > 0) 
                            { 
                                hex[hn] = '\0'; 
                                char *nl = strchr(hex, '\n'); 
                                if (nl) *nl = '\0'; 
                            }
                            uint8_t val = (uint8_t)strtol(hex, NULL, 16);
                            macho_patch_first_byte(path, val, stdout);
                        }
                    } 
                    else if (machopt[0] == 'c') 
                    {
                        char dir1[256], dir2[256];
                        printf("Enter first directory: ");
                        fflush(stdout);
                        int d1n = read(STDIN_FILENO, dir1, sizeof(dir1) - 1);
                        if (d1n > 0) 
                        { 
                            dir1[d1n] = '\0'; 
                            char *nl = strchr(dir1, '\n'); 
                            if (nl) *nl = '\0'; 
                        }
                        printf("Enter second directory: ");
                        fflush(stdout);
                        int d2n = read(STDIN_FILENO, 
                            dir2, sizeof(dir2) - 1);
                        if (d2n > 0) 
                        { 
                            dir2[d2n] = '\0'; 
                            char *nl = strchr(dir2, '\n'); 
                            if (nl) *nl = '\0'; 
                        }
                        macho_compare_dirs(dir1, dir2, stdout);
                    } 
                    else if (machopt[0] == 'a') 
                    {
                        char dir[256];
                        printf("Enter directory to extract Mach-O info from: ");
                        fflush(stdout);
                        int dn = read(STDIN_FILENO, 
                            dir, sizeof(dir) - 1);
                        if (dn > 0) 
                        { 
                            dir[dn] = '\0'; 
                            char *nl = strchr(dir, '\n'); 
                            if (nl) *nl = '\0'; 
                        }
                        macho_extract_dir(dir, stdout);
                    } 
                    else if (machopt[0] == 'd') 
                    {
                        char dir[256];
                        printf(
                            "Enter directory for deep Mach-O probing: ");
                        fflush(stdout);
                        int dn = read(STDIN_FILENO, 
                            dir, sizeof(dir) - 1);
                        if (dn > 0) 
                        { 
                            dir[dn] = '\0'; 
                            char *nl = strchr(dir, '\n'); 
                            if (nl) *nl = '\0'; 
                        }
                        macho_deep_probe(dir, stdout);
                    } 
                    else 
                    {
                        printf("[Mach-O] Unknown option\n");
                    }
                }
            }
            else if (buffer[0] == 'i')
            {
                printf("System/Hardware Info Menu:\n");
                printf("c - Print CPU info\n");
                printf("m - Print memory info\n");
                printf("g - Print GPU info\n");
                printf("k - List loaded kernel extensions (kexts)\n");
                printf("d - List IOKit devices (first 20)\n");
                printf("Enter info option: ");
                char infoopt[128];
                int in = read(STDIN_FILENO, infoopt, sizeof(infoopt) - 1);
                if (in > 0) 
                {
                    infoopt[in] = '\0';
                    if (infoopt[0] == 'c') 
                    {
                        sysprobe_print_cpu_info(stdout);
                    } 
                    else if (infoopt[0] == 'm') 
                    {
                        sysprobe_print_mem_info(stdout);
                    } 
                    else if (infoopt[0] == 'g') 
                    {
                        sysprobe_print_gpu_info(stdout);
                    } 
                    else if (infoopt[0] == 'k') 
                    {
                        sysprobe_list_kexts(stdout);
                    } 
                    else if (infoopt[0] == 'd') 
                    {
                        sysprobe_list_iokit_devices(stdout);
                    } 
                    else 
                    {
                        printf("[info] Unknown option\n");
                    }
                }
            }
            else if (buffer[0] == 'r') // 'r' for recursive and graphics stack probing
            {
                printf("Recursive/Graphics Probing Menu:\n");
                printf("m - Recursively scan for Mach-O binaries\n");
                printf("d - Recursively scan for .dylib files\n");
                printf("a - Recursively scan for AGX/graphics-related binaries\n");
                printf("Enter option: ");
                char recopt[128];
                int rn = read(STDIN_FILENO, 
                    recopt, sizeof(recopt) - 1);
                if (rn > 0) 
                {
                    recopt[rn] = '\0';
                    if (recopt[0] == 'm') 
                    {
                        char dir[256];
                        printf("Enter directory to scan for Mach-O: ");
                        fflush(stdout);
                        int dn = read(STDIN_FILENO, 
                            dir, sizeof(dir) - 1);
                        if (dn > 0) 
                        { 
                            dir[dn] = '\0'; 
                            char *nl = strchr(dir, '\n'); 
                            if (nl) *nl = '\0'; 
                        }
                        macho_recursive_scan(dir, stdout);
                    } 
                    else if (recopt[0] == 'd') 
                    {
                        char dir[256];
                        printf("Enter directory to scan for .dylib: ");
                        fflush(stdout);
                        int dn = read(STDIN_FILENO, 
                            dir, sizeof(dir) - 1);
                        if (dn > 0) 
                        { 
                            dir[dn] = '\0'; 
                            char *nl = strchr(dir, '\n'); 
                            if (nl) *nl = '\0'; 
                        }
                        macho_scan_dylibs(dir, stdout);
                    } 
                    else if (recopt[0] == 'a') 
                    {
                        char dir[256];
                        printf("Enter directory to scan for AGX/graphics: ");
                        fflush(stdout);
                        int dn = read(STDIN_FILENO, 
                            dir, sizeof(dir) - 1);
                        if (dn > 0) 
                        { 
                            dir[dn] = '\0'; 
                            char *nl = strchr(dir, '\n'); 
                            if (nl) *nl = '\0'; 
                        }
                        macho_scan_agx(dir, stdout);
                    } 
                    else 
                    {
                        printf("[rec] Unknown option\n");
                    }
                }
            }
            else if (buffer[0] == 'f')
            {
                printf("Framebuffer/Graphics Device Discovery:\n");
                sysprobe_list_framebuffer_devices(stdout);
            }
            else if (buffer[0] == 'd')
            {
                printf("Display/Config Directory Discovery:\n");
                sysprobe_list_display_dirs(stdout);
            }
            else if (buffer[0] == 't') 
            {
                printf("Test/Documentation Menu:\n");
                printf("l - Log a finding\n");
                printf("r - Save a report\n");
                printf("g - Prepare for graphics device test\n");
                printf("a - Prepare for ARM64 assembly experiment\n");
                printf("Enter option: ");
                char testopt[128];
                int tn = read(STDIN_FILENO, testopt, sizeof(testopt) - 1);
                if (tn > 0) 
                {
                    testopt[tn] = '\0';
                    if (testopt[0] == 'l') 
                    {
                        char msg[256];
                        printf("Enter finding to log: ");
                        fflush(stdout);
                        int mn = read(STDIN_FILENO, msg, sizeof(msg) - 1);
                        if (mn > 0) 
                        { 
                            msg[mn] = '\0'; 
                            char *nl = strchr(msg, '\n'); 
                            if (nl) *nl = '\0'; 
                        }
                        sysprobe_log_findings(msg, stdout);
                    } 
                    else if (testopt[0] == 'r') 
                    {
                        char fname[128], content[512];
                        printf("Enter report filename: ");
                        fflush(stdout);
                        int fn = read(STDIN_FILENO, 
                            fname, sizeof(fname) - 1);
                        if (fn > 0) 
                        { 
                            fname[fn] = '\0'; 
                            char *nl = strchr(fname, '\n'); 
                            if (nl) *nl = '\0'; 
                        }
                        printf("Enter report content: ");
                        fflush(stdout);
                        int cn = read(STDIN_FILENO, content, sizeof(content) - 1);
                        if (cn > 0) 
                        { content[cn] = '\0'; 
                            char *nl = strchr(content, '\n'); 
                            if (nl) *nl = '\0'; 
                        }
                        sysprobe_save_report(fname, content);
                    } 
                    else if (testopt[0] == 'g') 
                    {
                        char dev[256];
                        printf("Enter graphics device path: ");
                        fflush(stdout);
                        int dn = read(STDIN_FILENO, 
                            dev, sizeof(dev) - 1);
                        if (dn > 0) 
                        { 
                            dev[dn] = '\0'; 
                            char *nl = strchr(dev, '\n'); 
                            if (nl) *nl = '\0'; 
                        }
                        sysprobe_prepare_graphics_test(dev, stdout);
                    } 
                    else if (testopt[0] == 'a') 
                    {
                        sysprobe_prepare_arm64_asm(stdout);
                    } 
                    else 
                    {
                        printf("[test] Unknown option\n");
                    }
                }
            }
            else if (buffer[0] == 'w') 
            {
                printf(
                    "Bruteforce window probing started. See window_probing_results/ for results.\n");
                char candidates[128][256];
                int num_candidates = enumerate_dev_candidates(candidates, 128);
                if (num_candidates == 0) 
                {
                    printf("No candidate devices found in /dev.\n");
                    return 0;
                }
                int resolutions[][2] = 
                {
                    {640, 480}, {800, 600}, {1280, 720}, {1920, 1080}
                };
                for (int d = 0; d < num_candidates; ++d) 
                {
                    const char *dev = candidates[d];
                    char dev_clean[64];
                    int j = 0;
                    for (int i = 0; dev[i] && j < 63; ++i) 
                    {
                        if ((dev[i] >= 'a' && dev[i] <= 'z') || 
                            (dev[i] >= 'A' && dev[i] <= 'Z') || 
                            (dev[i] >= '0' && dev[i] <= '9'))
                            dev_clean[j++] = dev[i];
                        else
                            dev_clean[j++] = '_';
                    }
                    dev_clean[j] = '\0';
                    for (int r = 0; 
                            r < (int)(sizeof(resolutions) /
                            sizeof(resolutions[0])); 
                            ++r) 
                    {
                        char logname[128];
                        snprintf(logname, sizeof(logname), 
                            "window_probing_results/window_probe_log_%d_%s_%dx%d.txt", 
                            d, dev_clean, resolutions[r][0], resolutions[r][1]);
                        window_probe_bruteforce(&dev, 1, 
                            resolutions[r][0], resolutions[r][1], 
                            logname);
                    }
                }
                printf(
                    "Window probing complete. Check window_probing_results/ for details.\n");
            }
            else if (buffer[0] == 'g')
            {
                printf("Graphics Device Path Search:\n");
                sysprobe_search_graphics_devices(stdout);
            }
            else if (buffer[0] == 'B')
            {
                printf("Brute-force Graphics/Windowing: Running all methods...\n");
                graphics_bruteforce_all(stdout);
                printf("[brute] All methods complete. Check output for details.\n");
            }
            else if (buffer[0] == 'x') 
            {
                printf("Export framebuffer to output.txt as text or ASCII-art? (0=RGB, 1=ASCII): ");
                char modebuf[8];
                int mn = read(STDIN_FILENO, modebuf, sizeof(modebuf) - 1);
                int ascii = 0;
                if (mn > 0) { modebuf[mn] = '\0'; ascii = atoi(modebuf); }
                fb_save_txt(fb, "output.txt", ascii);
                printf("Framebuffer exported to output.txt\n");
            }
            else if (buffer[0] == 'c') 
            {
                printf("Export framebuffer to output.csv as CSV\n");
                fb_save_csv(fb, "output.csv");
                printf("Framebuffer exported to output.csv\n");
            }
            else if (buffer[0] == 'w') 
            {
                printf(
                    "Bruteforce window probing started. See window_probing_results/ for results.\n");
                char candidates[128][256];
                int num_candidates = 
                    enumerate_dev_candidates(candidates, 128);
                if (num_candidates == 0) 
                {
                    printf("No candidate devices found in /dev.\n");
                    return 0;
                }
                int resolutions[][2] = 
                {
                    {640, 480}, 
                    {800, 600}, 
                    {1280, 720}, 
                    {1920, 1080}
                };
                for (int d = 0; d < num_candidates; ++d) 
                {
                    const char *dev = candidates[d];
                    char dev_clean[64];
                    int j = 0;
                    for (int i = 0; dev[i] && j < 63; ++i) 
                    {
                        if ((dev[i] >= 'a' && dev[i] <= 'z') || 
                            (dev[i] >= 'A' && dev[i] <= 'Z') || 
                            (dev[i] >= '0' && dev[i] <= '9'))
                                dev_clean[j++] = dev[i];
                        else
                            dev_clean[j++] = '_';
                    }
                    dev_clean[j] = '\0';
                    for (int r = 0; r < 
                        (int)(sizeof(resolutions)/
                        sizeof(resolutions[0])); 
                        ++r) 
                    {
                        char logname[128];
                        snprintf(
                            logname, sizeof(logname), 
                            "window_probing_results/window_probe_log_%d_%s_%dx%d.txt", 
                            d, dev_clean, resolutions[r][0], resolutions[r][1]);
                        window_probe_bruteforce(&dev, 1, 
                            resolutions[r][0], resolutions[r][1], 
                            logname);
                    }
                }
                printf(
                    "Window probing complete. Check window_probing_results/ for details.\n");
            }
        }
    }
    
    return 0;
}

void cleanup_system(void)
{
    if (fb) 
    {
        fb_destroy(fb);
        fb = NULL;
    }
    
    if (kq != -1) 
    {
        close(kq);
        kq = -1;
    }
}

static void dummy_fn(void) 
{ 
    volatile int x = 0; 
    for (int i = 0; i < 1000000; ++i) 
        x += i; 
}

//Main function that uses the modular components
int main(void)
{
    if (init_system() != 0) 
    {
        return 1;
    }
    
    int quit = 0;
    while (!quit) 
    {
        quit = process_input();
        if (quit < 0) 
        {
            break;
        }
    }
    
    cleanup_system();
    return 0;
}
