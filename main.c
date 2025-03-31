#include "c_lib.h"
#include "mach_lib.h"
#include "net_lib.h"
#include "syscall.h"
#include "framebuffer.h"
#include "render.h"
#include "armOpCodes.h"

// Global variables using your original naming
static Framebuffer* fb = NULL;
static int kq = -1;
static struct kevent change;
static struct kevent event;
static Pixel red = {255, 0, 0};
static Pixel black = {0, 0, 0};

//Testing our buffer into Apple Hardware
uint8_t test_code[16] = 
{
    0xAA, 0x00, 0x20, 0x30, 0x40, 0x50,
    0x96, 0x00, 0x10, 0x20, 0x30, 0x40, 
    0x00, 0x80, 0x00, 0x00 
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
                printf("a - Runs agx_disassemble\n");
                printf("Enter option: \n");

                char suboption[128];
                int m = read(STDIN_FILENO, suboption, sizeof(suboption) - 1);

                if (m > 0)
                {
                    suboption[m] = '\0';
                    

                    if (suboption[0] == 'a')
                    {
                        printf("Disassembly of test code: \n");
                        agx_disassemble(test_code, sizeof(test_code), stdout);
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
