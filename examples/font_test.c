#include "../include/engine.h"
#include "../include/graphics.h"
#include <stdio.h>

int main(void) {
    /* Test just the font rendering */
    graphics_context_t* gfx = graphics_create_context(400, 200);
    
    /* Clear */
    graphics_clear(gfx, COLOR_BLACK);
    
    /* Test individual characters */
    graphics_draw_text(gfx, "ABCDEFGH", 10, 10, COLOR_WHITE, NULL);
    graphics_draw_text(gfx, "abcdefgh", 10, 30, COLOR_WHITE, NULL);
    graphics_draw_text(gfx, "01234567", 10, 50, COLOR_WHITE, NULL);
    graphics_draw_text(gfx, "Hello World!", 10, 70, COLOR_WHITE, NULL);
    graphics_draw_text(gfx, "2D Engine Test", 10, 90, COLOR_WHITE, NULL);
    
    /* Save to BMP to debug */
    FILE* fp = fopen("font_test.bmp", "wb");
    if (fp) {
        /* BMP header */
        u32 width = 400;
        u32 height = 200;
        u32 row_size = width * 3;
        u32 padded_row = (row_size + 3) & ~3;
        u32 file_size = 54 + padded_row * height;
        
        u8 header[54] = {
            'B','M',  /* Signature */
            file_size&0xFF, (file_size>>8)&0xFF, (file_size>>16)&0xFF, (file_size>>24)&0xFF,
            0,0,0,0,  /* Reserved */
            54,0,0,0,  /* Data offset */
            40,0,0,0,  /* DIB header size */
            width&0xFF, (width>>8)&0xFF, (width>>16)&0xFF, (width>>24)&0xFF,
            height&0xFF, (height>>8)&0xFF, (height>>16)&0xFF, (height>>24)&0xFF,
            1,0,      /* Planes */
            24,0,     /* Bits per pixel */
            0,0,0,0,  /* Compression */
            0,0,0,0,  /* Image size */
            0,0,0,0, 0,0,0,0,  /* Resolution */
            0,0,0,0, 0,0,0,0   /* Colors */
        };
        fwrite(header, 1, 54, fp);
        
        /* Write pixels bottom-up */
        u32* pixels = graphics_get_pixels(gfx);
        u8 row_buf[padded_row];
        memset(row_buf, 0, padded_row);
        
        for (i32 y = height - 1; y >= 0; y--) {
            for (u32 x = 0; x < width; x++) {
                u32 pixel = pixels[y * width + x];
                row_buf[x*3 + 0] = (pixel >> 16) & 0xFF;  /* B */
                row_buf[x*3 + 1] = (pixel >> 8) & 0xFF;   /* G */
                row_buf[x*3 + 2] = pixel & 0xFF;          /* R */
            }
            fwrite(row_buf, 1, padded_row, fp);
        }
        fclose(fp);
        printf("Saved font_test.bmp\n");
    }
    
    graphics_destroy_context(gfx);
    return 0;
}
