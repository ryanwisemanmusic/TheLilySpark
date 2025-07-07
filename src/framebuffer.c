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

void fb_save_txt(const Framebuffer *fb, const char *filename, int ascii_art)
{
    FILE *file = fopen(filename, "w");
    if (!file) 
    {
        perror("Cannot open file for text export");
        return;
    }
    for (int y = 0; y < fb->height; ++y) 
    {
        for (int x = 0; x < fb->width; ++x) 
        {
            Pixel p = fb->data[y * fb->width + x];
            if (!ascii_art) 
            {
                // Output as RGB values
                fprintf(file, "%3d,%3d,%3d ", p.r, p.g, p.b);
            } 
            else 
            {
                // Output as ASCII-art based on brightness
                int brightness = (p.r + p.g + p.b) / 3;
                char c;
                if (brightness > 220) c = ' ';
                else if (brightness > 180) c = '.';
                else if (brightness > 140) c = '*';
                else if (brightness > 100) c = 'o';
                else if (brightness > 60) c = 'O';
                else if (brightness > 30) c = '#';
                else c = '@';
                fputc(c, file);
            }
        }
        fputc('\n', file);
    }
    fclose(file);
}

void fb_save_csv(const Framebuffer *fb, const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (!file) 
    {
        perror("Cannot open file for CSV export");
        return;
    }
    // Write header
    fprintf(file, "x,y,r,g,b\n");
    for (int y = 0; y < fb->height; ++y) 
    {
        for (int x = 0; x < fb->width; ++x) 
        {
            Pixel p = fb->data[y * fb->width + x];
            fprintf(file, "%d,%d,%d,%d,%d\n", x, y, p.r, p.g, p.b);
        }
    }
    fclose(file);
}