#ifndef VIDEO_H
#define VIDEO_H

#include "types.h"

void init_video(u64 addr, u32 width, u32 height, u32 pitch, u8 bpp, u8 type);
void put_pixel(int x, int y, u32 color);
void draw_char(int x, int y, char c, u32 color);
void print_char(char c);
void print_str(const char* str);
void clear_screen();
void video_scroll();
void video_backspace();

#endif
