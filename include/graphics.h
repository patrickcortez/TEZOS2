#ifndef ENGINE_GRAPHICS_H
#define ENGINE_GRAPHICS_H

#include "types.h"

/* Forward declarations */
typedef struct graphics_context graphics_context_t;
typedef struct graphics_image graphics_image_t;
typedef struct graphics_font graphics_font_t;

/* Color structure (RGBA) */
typedef struct {
    u8 r, g, b, a;
} graphics_color_t;

/* Rectangle structure */
typedef struct {
    i32 x, y;
    i32 width, height;
} graphics_rect_t;

/* Point structure */
typedef struct {
    i32 x, y;
} graphics_point_t;

/* Predefined colors */
#define COLOR_BLACK       ((graphics_color_t){0, 0, 0, 255})
#define COLOR_WHITE       ((graphics_color_t){255, 255, 255, 255})
#define COLOR_RED         ((graphics_color_t){255, 0, 0, 255})
#define COLOR_GREEN       ((graphics_color_t){0, 255, 0, 255})
#define COLOR_BLUE        ((graphics_color_t){0, 0, 255, 255})
#define COLOR_YELLOW      ((graphics_color_t){255, 255, 0, 255})
#define COLOR_CYAN        ((graphics_color_t){0, 255, 255, 255})
#define COLOR_MAGENTA     ((graphics_color_t){255, 0, 255, 255})
#define COLOR_GRAY        ((graphics_color_t){128, 128, 128, 255})
#define COLOR_TRANSPARENT ((graphics_color_t){0, 0, 0, 0})

/* Color creation helpers */
ENGINE_API graphics_color_t graphics_rgb(u8 r, u8 g, u8 b);
ENGINE_API graphics_color_t graphics_rgba(u8 r, u8 g, u8 b, u8 a);
ENGINE_API graphics_color_t graphics_hex(u32 hex);

/* Context management */
ENGINE_API graphics_context_t* graphics_create_context(i32 width, i32 height);
ENGINE_API void graphics_destroy_context(graphics_context_t* ctx);
ENGINE_API i32 graphics_get_width(const graphics_context_t* ctx);
ENGINE_API i32 graphics_get_height(const graphics_context_t* ctx);
ENGINE_API void graphics_resize(graphics_context_t* ctx, i32 width, i32 height);

/* Rendering control */
ENGINE_API void graphics_clear(graphics_context_t* ctx, graphics_color_t color);
ENGINE_API void graphics_set_clip_rect(graphics_context_t* ctx, const graphics_rect_t* rect);
ENGINE_API void graphics_clear_clip_rect(graphics_context_t* ctx);

/* Drawing primitives */
ENGINE_API void graphics_draw_pixel(graphics_context_t* ctx, i32 x, i32 y, graphics_color_t color);
ENGINE_API void graphics_draw_line(graphics_context_t* ctx, i32 x1, i32 y1, i32 x2, i32 y2, graphics_color_t color);
ENGINE_API void graphics_draw_rect(graphics_context_t* ctx, const graphics_rect_t* rect, graphics_color_t color);
ENGINE_API void graphics_fill_rect(graphics_context_t* ctx, const graphics_rect_t* rect, graphics_color_t color);
ENGINE_API void graphics_draw_circle(graphics_context_t* ctx, i32 cx, i32 cy, i32 radius, graphics_color_t color);
ENGINE_API void graphics_fill_circle(graphics_context_t* ctx, i32 cx, i32 cy, i32 radius, graphics_color_t color);
ENGINE_API void graphics_draw_triangle(graphics_context_t* ctx, i32 x1, i32 y1, i32 x2, i32 y2, i32 x3, i32 y3, graphics_color_t color);
ENGINE_API void graphics_fill_triangle(graphics_context_t* ctx, i32 x1, i32 y1, i32 x2, i32 y2, i32 x3, i32 y3, graphics_color_t color);

/* Text rendering */
ENGINE_API graphics_font_t* graphics_get_default_font(void);
ENGINE_API void graphics_draw_text(graphics_context_t* ctx, const char* text, i32 x, i32 y, graphics_color_t color, graphics_font_t* font);
ENGINE_API void graphics_measure_text(const char* text, graphics_font_t* font, i32* out_width, i32* out_height);

/* Image operations */
ENGINE_API graphics_image_t* graphics_load_image(const char* filename);
ENGINE_API graphics_image_t* graphics_create_image(i32 width, i32 height);
ENGINE_API void graphics_destroy_image(graphics_image_t* image);
ENGINE_API void graphics_draw_image(graphics_context_t* ctx, const graphics_image_t* image, i32 x, i32 y);
ENGINE_API void graphics_draw_image_scaled(graphics_context_t* ctx, const graphics_image_t* image, const graphics_rect_t* dest);
ENGINE_API i32 graphics_image_get_width(const graphics_image_t* image);
ENGINE_API i32 graphics_image_get_height(const graphics_image_t* image);

/* Direct pixel buffer access (for advanced usage) */
ENGINE_API u32* graphics_get_pixels(graphics_context_t* ctx);
ENGINE_API void graphics_set_pixels(graphics_context_t* ctx, const u32* pixels);

/* Helper functions */
ENGINE_API graphics_rect_t graphics_rect(i32 x, i32 y, i32 width, i32 height);
ENGINE_API graphics_point_t graphics_point(i32 x, i32 y);
ENGINE_API bool graphics_rect_contains_point(const graphics_rect_t* rect, i32 x, i32 y);
ENGINE_API bool graphics_rect_intersects(const graphics_rect_t* a, const graphics_rect_t* b);

#endif /* ENGINE_GRAPHICS_H */
