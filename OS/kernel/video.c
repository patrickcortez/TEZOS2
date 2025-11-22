#include "video.h"
#include "font.h"

static u64 fb_addr;
static u32 fb_width;
static u32 fb_height;
static u32 fb_pitch;
static u8 fb_bpp;
static u8 fb_type;

static int cursor_x = 0;
static int cursor_y = 0;

#define FONT_WIDTH 8
#define FONT_HEIGHT 8
#define FG_COLOR 0xFFFFFFFF // White
#define BG_COLOR 0xFF0000FF // Blue

/* Forward declarations */
void draw_char_graphics(int x, int y, char c, u32 fg, u32 bg);
void video_scroll();

void init_video(u64 addr, u32 width, u32 height, u32 pitch, u8 bpp, u8 type) {
    fb_addr = addr;
    fb_width = width;
    fb_height = height;
    fb_pitch = pitch;
    fb_bpp = bpp;
    fb_type = type;
    
    cursor_x = 0;
    cursor_y = 0;
    
    clear_screen();
}

void put_pixel(int x, int y, u32 color) {
    if (fb_type != 1) return; /* Only supported in RGB mode */
    
    if (x < 0 || x >= fb_width || y < 0 || y >= fb_height) return;
    
    u64 offset = y * fb_pitch + x * (fb_bpp / 8);
    *(u32*)(fb_addr + offset) = color;
}

void clear_screen() {
    if (fb_type == 2) {
        /* Text Mode Clear */
        u16* vga_buffer = (u16*)fb_addr;
        for (int i = 0; i < fb_width * fb_height; i++) {
            vga_buffer[i] = 0x0F20; /* White on Black Space */
        }
    } else {
        /* Graphics Mode Clear */
        for (int y = 0; y < fb_height; y++) {
            for (int x = 0; x < fb_width; x++) {
                put_pixel(x, y, BG_COLOR);
            }
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

void video_scroll() {
    if (fb_type == 2) {
        /* Text Mode Scroll */
        u16* vga_buffer = (u16*)fb_addr;
        /* Move lines up */
        for (int i = 0; i < (fb_height - 1) * fb_width; i++) {
            vga_buffer[i] = vga_buffer[i + fb_width];
        }
        /* Clear last line */
        for (int i = (fb_height - 1) * fb_width; i < fb_height * fb_width; i++) {
            vga_buffer[i] = 0x0F20;
        }
    } else {
        /* Graphics Mode Scroll - Simple Clear for now */
        clear_screen();
        cursor_x = 0;
        cursor_y = 0;
    }
}

void draw_char_graphics(int x, int y, char c, u32 fg, u32 bg) {
    if (c < 0 || c > 127) return;
    
    const u8* glyph = font8x8[(int)c];
    
    for (int row = 0; row < FONT_HEIGHT; row++) {
        for (int col = 0; col < FONT_WIDTH; col++) {
            if ((glyph[row] >> (7 - col)) & 1) {
                put_pixel(x + col, y + row, fg);
            } else {
                put_pixel(x + col, y + row, bg);
            }
        }
    }
}

void print_char(char c) {
    if (fb_type == 2) {
        /* Text Mode */
        if (c == '\n') {
            cursor_x = 0;
            cursor_y++;
        } else if (c == '\b') {
             /* Handled in video_backspace, but if printed directly: */
             /* Do nothing or print a weird char? usually backspace is handled by shell calling backspace() */
        } else {
            u16* vga_buffer = (u16*)fb_addr;
            u16 entry = (u16)c | 0x0F00; /* White on Black */
            vga_buffer[cursor_y * fb_width + cursor_x] = entry;
            cursor_x++;
            if (cursor_x >= fb_width) {
                cursor_x = 0;
                cursor_y++;
            }
        }
        
        if (cursor_y >= fb_height) {
            video_scroll();
            cursor_y--;
        }
    } else {
        /* Graphics Mode */
        if (c == '\n') {
            cursor_x = 0;
            cursor_y += FONT_HEIGHT;
        } else {
            draw_char_graphics(cursor_x, cursor_y, c, FG_COLOR, BG_COLOR);
            cursor_x += FONT_WIDTH;
            if (cursor_x >= fb_width) {
                cursor_x = 0;
                cursor_y += FONT_HEIGHT;
            }
        }
        
        if (cursor_y + FONT_HEIGHT > fb_height) {
            video_scroll();
            /* Scroll resets cursor in my simple impl, but if it didn't: */
            /* cursor_y -= FONT_HEIGHT; */
        }
    }
}

void print_str(const char* str) {
    while (*str) {
        print_char(*str++);
    }
}

void video_backspace() {
    if (fb_type == 2) {
        /* Text Mode */
        if (cursor_x > 0) {
            cursor_x--;
            u16* vga_buffer = (u16*)fb_addr;
            vga_buffer[cursor_y * fb_width + cursor_x] = 0x0F20;
        } else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = fb_width - 1;
            u16* vga_buffer = (u16*)fb_addr;
            vga_buffer[cursor_y * fb_width + cursor_x] = 0x0F20;
        }
    } else {
        /* Graphics Mode */
        if (cursor_x >= FONT_WIDTH) {
            cursor_x -= FONT_WIDTH;
            draw_char_graphics(cursor_x, cursor_y, ' ', FG_COLOR, BG_COLOR);
        } else if (cursor_y >= FONT_HEIGHT) {
            cursor_y -= FONT_HEIGHT;
            cursor_x = (fb_width / FONT_WIDTH) * FONT_WIDTH - FONT_WIDTH;
            draw_char_graphics(cursor_x, cursor_y, ' ', FG_COLOR, BG_COLOR);
        }
    }
}

/* Legacy wrapper if needed */
void draw_char(int x, int y, char c, u32 color) {
    draw_char_graphics(x, y, c, color, BG_COLOR);
}
