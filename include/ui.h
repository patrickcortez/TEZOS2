#ifndef ENGINE_UI_H
#define ENGINE_UI_H

#include "types.h"
#include "graphics.h"
#include "platform.h"

/* Forward declarations */
typedef struct ui_context ui_context_t;
typedef u64 ui_id_t;

/* UI result */
typedef struct {
    bool clicked;
    bool hovered;
    bool active;
    bool focused;
    i32 value_int;
    f32 value_float;
    bool value_bool;
} ui_result_t;

/* UI style */
typedef struct {
    /* Colors */
    graphics_color_t background;
    graphics_color_t foreground;
    graphics_color_t border;
    graphics_color_t text;
    graphics_color_t accent;
    graphics_color_t hover;
    graphics_color_t active_color;
    
    /* Sizes */
    i32 padding;
    i32 spacing;
    i32 border_width;
    i32 scroll_bar_width;
    
    /* Text */
    i32 text_size;
    graphics_font_t* font;
} ui_style_t;

/* Layout direction */
typedef enum {
    UI_LAYOUT_VERTICAL,
    UI_LAYOUT_HORIZONTAL
} ui_layout_direction_t;

/* Text alignment */
typedef enum {
    UI_ALIGN_LEFT,
    UI_ALIGN_CENTER,
    UI_ALIGN_RIGHT
} ui_align_t;

/* Context management */
ENGINE_API ui_context_t* ui_create_context(graphics_context_t* gfx);
ENGINE_API void ui_destroy_context(ui_context_t* ctx);
ENGINE_API void ui_begin_frame(ui_context_t* ctx);
ENGINE_API void ui_end_frame(ui_context_t* ctx);

/* Input */
ENGINE_API void ui_input_mouse_move(ui_context_t* ctx, i32 x, i32 y);
ENGINE_API void ui_input_mouse_button(ui_context_t* ctx, bool down);
ENGINE_API void ui_input_key(ui_context_t* ctx, i32 key, bool down);
ENGINE_API void ui_input_char(ui_context_t* ctx, char c);

/* Style */
ENGINE_API ui_style_t ui_get_default_style(void);
ENGINE_API void ui_set_style(ui_context_t* ctx, const ui_style_t* style);
ENGINE_API ui_style_t* ui_get_style(ui_context_t* ctx);

/* Layout */
ENGINE_API void ui_layout_row(ui_context_t* ctx, i32 height, i32 item_count, const i32* widths);
ENGINE_API void ui_layout_column(ui_context_t* ctx, i32 width);
ENGINE_API void ui_spacing(ui_context_t* ctx, i32 amount);
ENGINE_API void ui_same_line(ui_context_t* ctx);

/* Container widgets */
ENGINE_API bool ui_begin_window(ui_context_t* ctx, const char* title, i32 x, i32 y, i32 width, i32 height);
ENGINE_API void ui_end_window(ui_context_t* ctx);
ENGINE_API bool ui_begin_panel(ui_context_t* ctx, const char* id, i32 height);
ENGINE_API void ui_end_panel(ui_context_t* ctx);

/* Basic widgets */
ENGINE_API void ui_label(ui_context_t* ctx, const char* text);
ENGINE_API void ui_label_ex(ui_context_t* ctx, const char* text, ui_align_t align);
ENGINE_API bool ui_button(ui_context_t* ctx, const char* label);
ENGINE_API bool ui_button_ex(ui_context_t* ctx, const char* label, i32 width, i32 height);
ENGINE_API bool ui_checkbox(ui_context_t* ctx, const char* label, bool* checked);
ENGINE_API bool ui_radio(ui_context_t* ctx, const char* label, i32* value, i32 option);

/* Input widgets */
ENGINE_API bool ui_text_input(ui_context_t* ctx, const char* label, char* buffer, i32 buffer_size);

/* Text input flags */
typedef enum {
    UI_TEXT_INPUT_PASSWORD = 1 << 0,    /* Show * instead of characters */
    UI_TEXT_INPUT_NUMERIC  = 1 << 1,    /* Only allow numbers */
    UI_TEXT_INPUT_READONLY = 1 << 2,    /* Display only, no editing */
} ui_text_input_flags_t;

/* List item - returns true if clicked */
ENGINE_API bool ui_list_item(ui_context_t* ctx, const char* label, bool selected);

/* Dropdown menu */
ENGINE_API bool ui_dropdown(ui_context_t* ctx, const char* label, const char** options, i32 option_count, i32* selected_index);

/* Menu Bar */
ENGINE_API bool ui_begin_menu_bar(ui_context_t* ctx);
ENGINE_API void ui_end_menu_bar(ui_context_t* ctx);
ENGINE_API bool ui_begin_menu(ui_context_t* ctx, const char* label);
ENGINE_API void ui_end_menu(ui_context_t* ctx);
ENGINE_API bool ui_menu_item(ui_context_t* ctx, const char* label);

/* Enhanced text input with flags and placeholder */
ENGINE_API bool ui_text_input_ex(
    ui_context_t* ctx,
    const char* label,
    char* buffer,
    i32 buffer_size,
    u32 flags,
    const char* placeholder
);

/* Multiline text area */
ENGINE_API bool ui_text_area(
    ui_context_t* ctx,
    const char* label,
    char* buffer,
    i32 buffer_size,
    i32 height_lines
);

/* Check if last text input was submitted (Enter pressed) */
ENGINE_API bool ui_text_input_submitted(ui_context_t* ctx);

ENGINE_API bool ui_slider_int(ui_context_t* ctx, const char* label, i32* value, i32 min, i32 max);
ENGINE_API bool ui_slider_float(ui_context_t* ctx, const char* label, f32* value, f32 min, f32 max);

/* Other widgets */
ENGINE_API void ui_separator(ui_context_t* ctx);
ENGINE_API void ui_progress_bar(ui_context_t* ctx, f32 progress);
ENGINE_API void ui_image(ui_context_t* ctx, graphics_image_t* image, i32 width, i32 height);

/* Helper macros for ID generation */
#define UI_ID(str) ui_hash_string(str)

/* Hash function for string IDs */
ENGINE_API ui_id_t ui_hash_string(const char* str);

#endif /* ENGINE_UI_H */
