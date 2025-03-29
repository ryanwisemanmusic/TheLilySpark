#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "framebuffer.h"

Framebuffer *fb_create(int width, int height)
{
    Framebuffer* fb = (Framebuffer*)malloc(sizeof(Framebuffer));
    fb->width = width;
    fb->height = height;
    fb->data = (Pixel*)calloc(width * height, sizeof(Pixel));
    return fb;
}

void fb_destroy(Framebuffer *fb)
{
    if (fb != NULL)
    {
        free(fb->data);
        free(fb);
    }
}

void fb_save_ppm(const Framebuffer *fb, const char *filename)
{
    FILE* file = fopen(filename, "wb");
    if (file == NULL)
    {
        perror("Cannot open file");
        return;
    }
    fprintf(file, "P6\n%d %d\n255\n", fb->width, fb->height);
    fwrite(fb->data, sizeof(Pixel), fb->width * fb->height, file);
    fclose(file);
}
