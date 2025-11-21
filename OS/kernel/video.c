#include "video.h"
#include "font.h"

static u32* framebuffer;
static u32 fb_width;
static u32 fb_height;
static u32 fb_pitch;
static u8 fb_bpp;

static int cursor_x = 0;
static int cursor_y = 0;

#define FONT_WIDTH 8
#define FONT_HEIGHT 8
#define FG_COLOR 0xFFFFFFFF // White
#define BG_COLOR 0xFF0000FF // Blue for debug

void init_video(u64 addr, u32 width, u32 height, u32 pitch, u8 bpp) {
    framebuffer = (u32*)addr;
    fb_width = width;
    fb_height = height;
    fb_pitch = pitch;
    fb_bpp = bpp;
    
    cursor_x = 0;
    cursor_y = 0;
    
    clear_screen();
}

void put_pixel(int x, int y, u32 color) {
    if (x < 0 || x >= fb_width || y < 0 || y >= fb_height) return;
    
    /* Assuming 32-bit color for simplicity as requested in header.asm */
    /* Pitch is in bytes, so divide by 4 for u32 pointer arithmetic if pitch is strictly row size */
    /* However, standard framebuffer access: */
    u32 pixel_offset = y * (fb_pitch / 4) + x;
    framebuffer[pixel_offset] = color;
}

void draw_char(int x, int y, char c, u32 color) {
    if (c < 0 || c > 127) return;
    
    const u8* glyph = font8x8[(int)c];
    
    for (int row = 0; row < FONT_HEIGHT; row++) {
        for (int col = 0; col < FONT_WIDTH; col++) {
            if ((glyph[row] >> (7 - col)) & 1) {
                put_pixel(x + col, y + row, color);
            } else {
                /* Draw background color to overwrite previous text */
                put_pixel(x + col, y + row, BG_COLOR);
            }
        }
    }
}

void clear_screen() {
    for (int y = 0; y < fb_height; y++) {
        for (int x = 0; x < fb_width; x++) {
            put_pixel(x, y, BG_COLOR);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

void video_scroll() {
    /* Simple scroll: copy lines up */
    /* This is slow in software, but functional */
    u32 row_size = fb_pitch / 4;
    
    for (int y = 0; y < fb_height - FONT_HEIGHT; y++) {
        for (int x = 0; x < fb_width; x++) {
            u32 src_idx = (y + FONT_HEIGHT) * row_size + x;
            u32 dst_idx = y * row_size + x;
            framebuffer[dst_idx] = framebuffer[src_idx];
        }
    }
    
    /* Clear last line */
    for (int y = fb_height - FONT_HEIGHT; y < fb_height; y++) {
        for (int x = 0; x < fb_width; x++) {
            put_pixel(x, y, BG_COLOR);
        }
    }
}

void print_str(const char* str) {
    while (*str) {
        print_char(*str++);
    }
}

void print_char(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y += FONT_HEIGHT;
    } else {
        draw_char(cursor_x, cursor_y, c, FG_COLOR);
        cursor_x += FONT_WIDTH;
        
        if (cursor_x >= fb_width) {
            cursor_x = 0;
            cursor_y += FONT_HEIGHT;
        }
    }
    
    if (cursor_y + FONT_HEIGHT >= fb_height) {
        video_scroll();
        cursor_y -= FONT_HEIGHT;
    }
}

void video_backspace() {
    if (cursor_x >= FONT_WIDTH) {
        cursor_x -= FONT_WIDTH;
        /* Draw space to erase */
        draw_char(cursor_x, cursor_y, ' ', FG_COLOR); 
        /* Actually draw_char draws background for 0 bits, so ' ' works if font[' '] is all 0s */
    } else if (cursor_y >= FONT_HEIGHT) {
        cursor_y -= FONT_HEIGHT;
        cursor_x = (fb_width / FONT_WIDTH) * FONT_WIDTH - FONT_WIDTH;
        draw_char(cursor_x, cursor_y, ' ', FG_COLOR);
    }
}
