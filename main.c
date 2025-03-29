#include "c_lib.h"
#include "mach_lib.h"
#include "net_lib.h"
#include "syscall.h"
#include "framebuffer.h"
#include "render.h"

int main()
{
    Framebuffer* fb = fb_create(640, 480);
    Pixel red = {255, 0, 0};
    Pixel black = {0, 0, 0};
    int kq = kqueue();
    if (kq == -1)
    {
        perror("kqueue");
        exit(1);
    }

    struct kevent change;
    EV_SET(&change, STDIN_FILENO, EVFILT_READ, EV_ADD, 0, 0, NULL);

    struct kevent event;
    while(1)
    {
        int nev = kevent(kq, &change, 1, &event, 1, NULL);
        if (nev == -1)
        {
            perror("kevent");
            exit(1);
        }

        if (event.filter == EVFILT_READ)
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
                else if (buffer[0] == 'q')
                {
                    printf("Exiting program...\n");
                    break;
                }
            }
        }
    }
    fb_destroy(fb);
    close(kq);
    return 0;
}
