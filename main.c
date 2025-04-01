#include "c_lib.h"
#include "mach_lib.h"
#include "net_lib.h"
#include "syscall.h"
#include "framebuffer.h"
#include "render.h"
#include "armOpCodes.h"

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
    if (!fb) {
        fprintf(stderr, "Failed to create framebuffer\n");
        return -1;
    }
    
    kq = kqueue();
    if (kq == -1) {
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
    if (nev == -1) {
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
            
            if (buffer[0] == 'd') {
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
                printf("Enter option: \n");

                char suboption[128];
                int m = read(STDIN_FILENO, suboption, sizeof(suboption) - 1);

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
                        agx_disassemble_instr(test_code, &stop, true, stdout);
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
                }
                
            }
            else if (buffer[0] == 'q') {
                printf("Exiting program...\n");
                return 1; 
            }
        }
    }
    
    return 0;
}

void cleanup_system(void)
{
    if (fb) {
        fb_destroy(fb);
        fb = NULL;
    }
    
    if (kq != -1) {
        close(kq);
        kq = -1;
    }
}

// Main function that uses the modular components
int main(void)
{
    if (init_system() != 0) {
        return 1;
    }
    
    int quit = 0;
    while (!quit) {
        quit = process_input();
        if (quit < 0) {
            break;
        }
    }
    
    cleanup_system();
    return 0;
}
