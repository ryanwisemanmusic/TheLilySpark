#pragma once
#include <stdint.h>
#include "c_lib.h"
#include "mach_lib.h"
#include "net_lib.h"
#include "syscall.h"

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Pixel;

typedef struct
{
    Pixel* data;
    int width;
    int height;
} Framebuffer;

Framebuffer* fb_create(int width, int height);
void fb_destroy (Framebuffer* fb);

void fb_save_ppm(const Framebuffer* fb, const char* filename);