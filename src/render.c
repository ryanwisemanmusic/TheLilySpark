#include "render.h"
void render_clear(Framebuffer* fb, Pixel color)
{
    for (int i = 0; i < fb->width * fb->height; i++)
    {
        fb->data[i] = color;
    }
}

void render_rect(Framebuffer* fb, int x, int y, int w, int h, Pixel color) 
{
    for (int dy = 0; dy < h; dy++) 
    {
        for (int dx = 0; dx < w; dx++) 
        {
            int px = x + dx;
            int py = y + dy;
            if (px >= 0 && px < fb->width && py >= 0 && py < fb->height) 
            {
                fb->data[py * fb->width + px] = color;
            }
        }
    }
}