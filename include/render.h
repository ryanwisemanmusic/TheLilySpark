#pragma once
#include "framebuffer.h"

void render_clear(Framebuffer* fb, Pixel color);
void render_rect(Framebuffer* fb, int x, int y, int width, int height, Pixel color);