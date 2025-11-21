#include "../include/ui.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define UI_MAX_LAYOUT_STACK 32
#define UI_MAX_INPUT_BUFFER 256

/* Layout state */
typedef struct {
    graphics_rect_t bounds;
    ui_layout_direction_t direction;
    i32 current_x, current_y;
    i32 row_height;
    i32 item_index;
    i32 item_count;
    const i32* item_widths;
} ui_layout_t;

/* Popup command type */
typedef enum {
    UI_POPUP_CMD_RECT,
    UI_POPUP_CMD_TEXT,
    UI_POPUP_CMD_ICON
} ui_popup_cmd_type_t;

/* Popup command */
typedef struct {
    ui_popup_cmd_type_t type;
    graphics_rect_t rect;
    graphics_color_t color;
    char text[64];
    graphics_font_t* font;
} ui_popup_cmd_t;

#define UI_MAX_POPUP_COMMANDS 128

/* UI context */
struct ui_context {
    graphics_context_t* gfx;
    ui_style_t style;
    
    /* Input state */
    i32 mouse_x, mouse_y;
    bool mouse_down;
    bool mouse_was_down;
    i32 last_key;
    bool key_down;
    char input_char;
    i32 mouse_wheel_delta;
    
    /* Widget state */
    ui_id_t hot_id;      /* Widget under mouse */
    ui_id_t active_id;   /* Widget being interacted with */
    ui_id_t focus_id;    /* Widget with keyboard focus */
    
    /* Popup state */
    ui_id_t open_popup_id;
    graphics_rect_t popup_rect;
    i32 popup_cursor_x, popup_cursor_y;
    ui_popup_cmd_t popup_commands[UI_MAX_POPUP_COMMANDS];
    i32 popup_command_count;
    
    /* Scroll state */
    i32 scroll_offset_x, scroll_offset_y;
    i32 content_width, content_height;
    i32 viewport_width, viewport_height;
    graphics_rect_t window_bounds;
    i32 max_content_y;  /* Track maximum Y position for content */
    bool in_scroll_region;
    
    /* Layout stack */
    ui_layout_t layout_stack[UI_MAX_LAYOUT_STACK];
    i32 layout_stack_size;
    
    /* Current layout */
    i32 cursor_x, cursor_y;
    i32 row_height;
    bool same_line;
    
    /* Text input state */
    char* text_input_buffer;
    i32 text_input_buffer_size;
    i32 text_input_cursor;
    
    /* Frame counter for animations */
    i32 frame_count;
};

/* Helper: Hash string to ID */
ui_id_t ui_hash_string(const char* str) {
    ui_id_t hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) + (*str++);
    }
    return hash;
}

/* Helper: Check if point is in rect */
static bool point_in_rect(i32 x, i32 y, const graphics_rect_t* rect) {
    return x >= rect->x && x < rect->x + rect->width &&
           y >= rect->y && y < rect->y + rect->height;
}

/* Helper: Clamp value */
static i32 clamp_int(i32 val, i32 min, i32 max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

static f32 clamp_float(f32 val, f32 min, f32 max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

/* Default style */
ui_style_t ui_get_default_style(void) {
    ui_style_t style;
    style.background = graphics_rgb(45, 45, 48);
    style.foreground = graphics_rgb(60, 60, 65);
    style.border = graphics_rgb(100, 100, 105);
    style.text = graphics_rgb(220, 220, 220);
    style.accent = graphics_rgb(0, 122, 204);
    style.hover = graphics_rgb(70, 70, 75);
    style.active_color = graphics_rgb(0, 102, 184);
    
    style.padding = 8;
    style.spacing = 4;
    style.border_width = 1;
    style.scroll_bar_width = 12;
    
    style.text_size = 8;
    style.font = graphics_get_default_font();
    
    return style;
}

/* Context management */
ui_context_t* ui_create_context(graphics_context_t* gfx) {
    if (!gfx) return NULL;
    
    ui_context_t* ctx = (ui_context_t*)calloc(1, sizeof(ui_context_t));
    if (!ctx) return NULL;
    
    ctx->gfx = gfx;
    ctx->style = ui_get_default_style();
    ctx->layout_stack_size = 0;
    ctx->cursor_x = 0;
    ctx->cursor_y = 0;
    ctx->row_height = 24;
    ctx->same_line = false;
    
    return ctx;
}

void ui_destroy_context(ui_context_t* ctx) {
    if (!ctx) return;
    free(ctx);
}

void ui_begin_frame(ui_context_t* ctx) {
    if (!ctx) return;
    
    /* Reset per-frame state */
    ctx->hot_id = 0;
    /* DON'T clear input_char here - widgets need to read it! */
    ctx->cursor_x = ctx->style.spacing;
    ctx->cursor_y = ctx->style.spacing;
    ctx->same_line = false;
    ctx->frame_count++;
}

void ui_end_frame(ui_context_t* ctx) {
    if (!ctx) return;
    
    /* Render popups on top */
    for (i32 i = 0; i < ctx->popup_command_count; i++) {
        ui_popup_cmd_t* cmd = &ctx->popup_commands[i];
        switch (cmd->type) {
            case UI_POPUP_CMD_RECT:
                graphics_fill_rect(ctx->gfx, &cmd->rect, cmd->color);
                /* Optional border for popups */
                graphics_draw_rect(ctx->gfx, &cmd->rect, ctx->style.border);
                break;
            case UI_POPUP_CMD_TEXT:
                graphics_draw_text(ctx->gfx, cmd->text, cmd->rect.x, cmd->rect.y, cmd->color, cmd->font);
                break;
            default:
                break;
        }
    }
    
    /* Reset popup commands for next frame */
    ctx->popup_command_count = 0;
    
    /* Clear active if mouse released */
    if (!ctx->mouse_down && ctx->mouse_was_down) {
        ctx->active_id = 0;
    }

    /* Reset mouse state */
    ctx->mouse_was_down = ctx->mouse_down;
    
    /* Clear input char AFTER widgets have processed it */
    ctx->input_char = 0;
}

/* Input */
void ui_input_mouse_move(ui_context_t* ctx, i32 x, i32 y) {
    if (!ctx) return;
    ctx->mouse_x = x;
    ctx->mouse_y = y;
}

void ui_input_mouse_button(ui_context_t* ctx, bool down) {
    if (!ctx) return;
    ctx->mouse_down = down;
}

void ui_input_mouse_wheel(ui_context_t* ctx, i32 delta) {
    if (!ctx) return;
    ctx->mouse_wheel_delta = delta;
}

void ui_input_key(ui_context_t* ctx, i32 key, bool down) {
    if (!ctx) return;
    ctx->last_key = key;
    ctx->key_down = down;
}

void ui_input_char(ui_context_t* ctx, char c) {
    if (!ctx) return;
    ctx->input_char = c;
}

/* Style */
void ui_set_style(ui_context_t* ctx, const ui_style_t* style) {
    if (!ctx || !style) return;
    ctx->style = *style;
}

ui_style_t* ui_get_style(ui_context_t* ctx) {
    return ctx ? &ctx->style : NULL;
}

/* Layout */
void ui_layout_row(ui_context_t* ctx, i32 height, i32 item_count, const i32* widths) {
    if (!ctx) return;
    ctx->row_height = height > 0 ? height : 24;
    /* Simple layout - items will be placed sequentially */
    ENGINE_UNUSED(item_count);
    ENGINE_UNUSED(widths);
}

void ui_layout_column(ui_context_t* ctx, i32 width) {
    if (!ctx) return;
    ENGINE_UNUSED(width);
}

void ui_spacing(ui_context_t* ctx, i32 amount) {
    if (!ctx) return;
    ctx->cursor_y += amount;
}

void ui_same_line(ui_context_t* ctx) {
    if (!ctx) return;
    ctx->same_line = true;
}

/* Container widgets */
bool ui_begin_window(ui_context_t* ctx, const char* title, i32 x, i32 y, i32 width, i32 height) {
    if (!ctx) return false;
    
    /* Setup scroll region */
    ctx->in_scroll_region = true;
    ctx->window_bounds = graphics_rect(x, y, width, height);
    ctx->viewport_width = width - ctx->style.scroll_bar_width;
    ctx->viewport_height = height - 24 - ctx->style.padding; /* Title + padding */
    
    /* Handle mouse wheel scrolling */
    if (ctx->mouse_wheel_delta != 0) {
        ctx->scroll_offset_y -= ctx->mouse_wheel_delta * 20; /* 20 pixels per wheel notch */
        ctx->mouse_wheel_delta = 0; /* Consume the event */
    }
    
    /* Draw window background */
    graphics_rect_t rect = graphics_rect(x, y, width, height);
    graphics_fill_rect(ctx->gfx, &rect, ctx->style.foreground);
    graphics_draw_rect(ctx->gfx, &rect, ctx->style.border);
    
    /* Draw title */
    if (title) {
        graphics_draw_text(ctx->gfx, title, x + ctx->style.padding, y + ctx->style.padding,
                         ctx->style.text, ctx->style.font);
    }
    
    /* Set up clipping for content area */
    graphics_rect_t clip_rect = graphics_rect(
        x + ctx->style.padding,
        y + 24,
        ctx->viewport_width - ctx->style.padding,
        ctx->viewport_height
    );
    graphics_set_clip_rect(ctx->gfx, &clip_rect);
    
    /* Set cursor inside window with scroll offset */
    ctx->cursor_x = x + ctx->style.padding;
    ctx->cursor_y = y + ctx->style.padding + 24 - ctx->scroll_offset_y;
    ctx->max_content_y = ctx->cursor_y; /* Track start */
    
    return true;
}

void ui_end_window(ui_context_t* ctx) {
    if (!ctx) return;
    
    /* Calculate actual content height (how far down the content went) */
    i32 content_start_y = ctx->window_bounds.y + 24;
    ctx->content_height = (ctx->max_content_y + ctx->scroll_offset_y) - content_start_y;
    
    /* Clamp scroll offset to valid range */
    i32 max_scroll = ctx->content_height - ctx->viewport_height;
    if (max_scroll < 0) max_scroll = 0;
    
    /* Clamp the scroll offset */
    if (ctx->scroll_offset_y < 0) ctx->scroll_offset_y = 0;
    if (ctx->scroll_offset_y > max_scroll) ctx->scroll_offset_y = max_scroll;
    
    /* Clear clipping BEFORE drawing scrollbar */
    graphics_clear_clip_rect(ctx->gfx);
    
    /* Draw vertical scrollbar if needed (AFTER clearing clip) */
    if (ctx->content_height > ctx->viewport_height && max_scroll > 0) {
        i32 scrollbar_x = ctx->window_bounds.x + ctx->window_bounds.width - ctx->style.scroll_bar_width;
        i32 scrollbar_y = ctx->window_bounds.y + 24;
        i32 scrollbar_height = ctx->viewport_height;
        
        /* Scrollbar track */
        graphics_rect_t track = graphics_rect(scrollbar_x, scrollbar_y, ctx->style.scroll_bar_width, scrollbar_height);
        graphics_fill_rect(ctx->gfx, &track, ctx->style.background);
        
        /* Scrollbar thumb */
        f32 thumb_ratio = (f32)ctx->viewport_height / (f32)ctx->content_height;
        i32 thumb_height = (i32)(scrollbar_height * thumb_ratio);
        if (thumb_height < 20) thumb_height = 20;
        if (thumb_height > scrollbar_height) thumb_height = scrollbar_height;
        
        f32 scroll_ratio = 0.0f;
        if (max_scroll > 0) {
            scroll_ratio = (f32)ctx->scroll_offset_y / (f32)max_scroll;
        }
        i32 thumb_y = scrollbar_y + (i32)((scrollbar_height - thumb_height) * scroll_ratio);
        
        graphics_rect_t thumb = graphics_rect(scrollbar_x, thumb_y, ctx->style.scroll_bar_width, thumb_height);
        graphics_fill_rect(ctx->gfx, &thumb, ctx->style.accent);
        
        /* Handle scrollbar dragging */
        ui_id_t scrollbar_id = ui_hash_string("__scrollbar_v");
        bool thumb_hovered = point_in_rect(ctx->mouse_x, ctx->mouse_y, &thumb);
        
        if (thumb_hovered && ctx->mouse_down && ctx->active_id == 0) {
            ctx->active_id = scrollbar_id;
        }
        
        if (ctx->active_id == scrollbar_id && ctx->mouse_down) {
            /* Calculate new scroll offset from mouse position */
            i32 relative_y = ctx->mouse_y - scrollbar_y - (thumb_height / 2);
            f32 new_scroll_ratio = (f32)relative_y / (f32)(scrollbar_height - thumb_height);
            ctx->scroll_offset_y = (i32)(new_scroll_ratio * max_scroll);
            
            /* Clamp again */
            if (ctx->scroll_offset_y < 0) ctx->scroll_offset_y = 0;
            if (ctx->scroll_offset_y > max_scroll) ctx->scroll_offset_y = max_scroll;
        }
    }
    
    /* Reset state */
    ctx->in_scroll_region = false;
    ctx->cursor_x = ctx->style.spacing;
    ctx->cursor_y = ctx->style.spacing;
}

bool ui_begin_panel(ui_context_t* ctx, const char* id, i32 height) {
    if (!ctx) return false;
    
    i32 width = graphics_get_width(ctx->gfx) - ctx->cursor_x - ctx->style.spacing;
    graphics_rect_t rect = graphics_rect(ctx->cursor_x, ctx->cursor_y, width, height);
    
    graphics_fill_rect(ctx->gfx, &rect, ctx->style.foreground);
    graphics_draw_rect(ctx->gfx, &rect, ctx->style.border);
    
    ctx->cursor_x += ctx->style.padding;
    ctx->cursor_y += ctx->style.padding;
    
    ENGINE_UNUSED(id);
    return true;
}

void ui_end_panel(ui_context_t* ctx) {
    if (!ctx) return;
    ctx->cursor_x = ctx->style.spacing;
    ctx->cursor_y += ctx->style.padding + ctx->style.spacing;
}

/* Basic widgets */
void ui_label(ui_context_t* ctx, const char* text) {
    ui_label_ex(ctx, text, UI_ALIGN_LEFT);
}

void ui_label_ex(ui_context_t* ctx, const char* text, ui_align_t align) {
    if (!ctx || !text) return;
    
    i32 x = ctx->cursor_x;
    i32 y = ctx->cursor_y;
    
    /* Apply alignment */
    if (align == UI_ALIGN_CENTER) {
        i32 text_width, text_height;
        graphics_measure_text(text, ctx->style.font, &text_width, &text_height);
        x += (graphics_get_width(ctx->gfx) - ctx->cursor_x * 2 - text_width) / 2;
    } else if (align == UI_ALIGN_RIGHT) {
        i32 text_width, text_height;
        graphics_measure_text(text, ctx->style.font, &text_width, &text_height);
        x = graphics_get_width(ctx->gfx) - ctx->cursor_x - text_width;
    }
    
    graphics_draw_text(ctx->gfx, text, x, y, ctx->style.text, ctx->style.font);
    
    if (!ctx->same_line) {
        ctx->cursor_y += ctx->row_height;
    } else {
        i32 text_width, text_height;
        graphics_measure_text(text, ctx->style.font, &text_width, &text_height);
        ctx->cursor_x += text_width + ctx->style.spacing;
        ctx->same_line = false;
    }
}

bool ui_button(ui_context_t* ctx, const char* label) {
    return ui_button_ex(ctx, label, 120, ctx->row_height);
}

bool ui_button_ex(ui_context_t* ctx, const char* label, i32 width, i32 height) {
    if (!ctx || !label) return false;
    
    ui_id_t id = ui_hash_string(label);
    graphics_rect_t rect = graphics_rect(ctx->cursor_x, ctx->cursor_y, width, height);
    
    /* Check interaction */
    bool hovered = point_in_rect(ctx->mouse_x, ctx->mouse_y, &rect);
    bool clicked = false;
    
    if (hovered) {
        ctx->hot_id = id;
        
        if (ctx->mouse_down && !ctx->mouse_was_down) {
            ctx->active_id = id;
            // printf("[DEBUG] Button '%s' active (mouse_down=%d, was_down=%d)\n", label, ctx->mouse_down, ctx->mouse_was_down);
        }
    }
    
    if (ctx->active_id == id) {
        if (!ctx->mouse_down && ctx->mouse_was_down && hovered) {
            clicked = true;
        }
    }
    
    /* Render */
    graphics_color_t bg_color = ctx->style.foreground;
    if (ctx->active_id == id) {
        bg_color = ctx->style.active_color;
    } else if (ctx->hot_id == id) {
        bg_color = ctx->style.hover;
    }
    
    graphics_fill_rect(ctx->gfx, &rect, bg_color);
    graphics_draw_rect(ctx->gfx, &rect, ctx->style.border);
    
    /* Draw label centered */
    i32 text_width, text_height;
    graphics_measure_text(label, ctx->style.font, &text_width, &text_height);
    i32 text_x = rect.x + (width - text_width) / 2;
    i32 text_y = rect.y + (height - text_height) / 2;
    graphics_draw_text(ctx->gfx, label, text_x, text_y, ctx->style.text, ctx->style.font);
    
    /* Advance cursor */
    if (!ctx->same_line) {
        ctx->cursor_y += height + ctx->style.spacing;
    } else {
        ctx->cursor_x += width + ctx->style.spacing;
        ctx->same_line = false;
    }
    
    return clicked;
}

bool ui_checkbox(ui_context_t* ctx, const char* label, bool* checked) {
    if (!ctx || !label || !checked) return false;
    
    ui_id_t id = ui_hash_string(label);
    i32 box_size = 16;
    graphics_rect_t rect = graphics_rect(ctx->cursor_x, ctx->cursor_y, box_size, box_size);
    
    bool hovered = point_in_rect(ctx->mouse_x, ctx->mouse_y, &rect);
    bool changed = false;
    
    if (hovered) {
        ctx->hot_id = id;
        if (ctx->mouse_down && !ctx->mouse_was_down) {
            *checked = !(*checked);
            changed = true;
        }
    }
    
    /* Render box */
    graphics_color_t bg_color = ctx->hot_id == id ? ctx->style.hover : ctx->style.foreground;
    graphics_fill_rect(ctx->gfx, &rect, bg_color);
    graphics_draw_rect(ctx->gfx, &rect, ctx->style.border);
    
    /* Draw checkmark if checked */
    if (*checked) {
        graphics_rect_t check = graphics_rect(
            rect.x + 4, rect.y + 4,
            box_size - 8, box_size - 8
        );
        graphics_fill_rect(ctx->gfx, &check, ctx->style.accent);
    }
    
    /* Draw label */
    graphics_draw_text(ctx->gfx, label, ctx->cursor_x + box_size + ctx->style.spacing,
                      ctx->cursor_y, ctx->style.text, ctx->style.font);
    
    ctx->cursor_y += box_size + ctx->style.spacing;
    
    return changed;
}

bool ui_radio(ui_context_t* ctx, const char* label, i32* value, i32 option) {
    if (!ctx || !label || !value) return false;
    
    char id_str[256];
    snprintf(id_str, sizeof(id_str), "%s_%d", label, option);
    ui_id_t id = ui_hash_string(id_str);
    
    i32 circle_size = 16;
    i32 cx = ctx->cursor_x + circle_size / 2;
    i32 cy = ctx->cursor_y + circle_size / 2;
    
    graphics_rect_t bounds = graphics_rect(ctx->cursor_x, ctx->cursor_y, circle_size, circle_size);
    bool hovered = point_in_rect(ctx->mouse_x, ctx->mouse_y, &bounds);
    bool changed = false;
    
    if (hovered) {
        ctx->hot_id = id;
        if (ctx->mouse_down && !ctx->mouse_was_down) {
            *value = option;
            changed = true;
        }
    }
    
    /* Render circle */
    graphics_color_t bg_color = ctx->hot_id == id ? ctx->style.hover : ctx->style.foreground;
    graphics_fill_circle(ctx->gfx, cx, cy, circle_size / 2, bg_color);
    graphics_draw_circle(ctx->gfx, cx, cy, circle_size / 2, ctx->style.border);
    
    /* Draw dot if selected */
    if (*value == option) {
        graphics_fill_circle(ctx->gfx, cx, cy, circle_size / 4, ctx->style.accent);
    }
    
    /* Draw label */
    graphics_draw_text(ctx->gfx, label, ctx->cursor_x + circle_size + ctx->style.spacing,
                      ctx->cursor_y, ctx->style.text, ctx->style.font);
    
    ctx->cursor_y += circle_size + ctx->style.spacing;
    
    return changed;
}

/* Text input */
bool ui_text_input(ui_context_t* ctx, const char* label, char* buffer, i32 buffer_size) {
    if (!ctx || !label || !buffer || buffer_size <= 0) return false;
    
    ui_id_t id = ui_hash_string(label);
    i32 input_width = 200;
    i32 input_height = ctx->row_height;
    
    /* Draw label */
    graphics_draw_text(ctx->gfx, label, ctx->cursor_x, ctx->cursor_y, ctx->style.text, ctx->style.font);
    
    i32 label_width, label_height;
    graphics_measure_text(label, ctx->style.font, &label_width, &label_height);
    
    graphics_rect_t rect = graphics_rect(
        ctx->cursor_x + label_width + ctx->style.spacing,
        ctx->cursor_y, input_width, input_height
    );
    
    bool hovered = point_in_rect(ctx->mouse_x, ctx->mouse_y, &rect);
    bool changed = false;
    
    if (hovered && ctx->mouse_down && !ctx->mouse_was_down) {
        ctx->focus_id = id;
    }
    
    bool focused = (ctx->focus_id == id);
    
    /* Handle input */
    if (focused && ctx->input_char != 0) {
        i32 len = strlen(buffer);
        if (ctx->input_char == '\b' || ctx->input_char == 127) {
            /* Backspace */
            if (len > 0) {
                buffer[len - 1] = '\0';
                changed = true;
            }
        } else if (ctx->input_char >= 32 && ctx->input_char < 127) {
            /* Printable character */
            if (len < buffer_size - 1) {
                buffer[len] = ctx->input_char;
                buffer[len + 1] = '\0';
                changed = true;
            }
        }
    }
    
    /* Render */
    graphics_color_t bg_color = focused ? ctx->style.hover : ctx->style.foreground;
    graphics_fill_rect(ctx->gfx, &rect, bg_color);
    graphics_draw_rect(ctx->gfx, &rect, focused ? ctx->style.accent : ctx->style.border);
    
    /* Draw text */
    graphics_draw_text(ctx->gfx, buffer, rect.x + ctx->style.padding, rect.y + ctx->style.padding,
                      ctx->style.text, ctx->style.font);
    
    /* Draw blinking cursor */
    if (focused && (ctx->frame_count / 30) % 2 == 0) {  /* Blink every 30 frames */
        i32 text_width, text_height;
        graphics_measure_text(buffer, ctx->style.font, &text_width, &text_height);
        graphics_draw_line(ctx->gfx,
            rect.x + ctx->style.padding + text_width, rect.y + 4,
            rect.x + ctx->style.padding + text_width, rect.y + input_height - 4,
            ctx->style.text);
    }
    
    ctx->cursor_y += input_height + ctx->style.spacing;
    
    return changed;
}

/* Sliders */
bool ui_slider_int(ui_context_t* ctx, const char* label, i32* value, i32 min, i32 max) {
    if (!ctx || !label || !value) return false;
    
    ui_id_t id = ui_hash_string(label);
    i32 slider_width = 200;
    i32 slider_height = ctx->row_height;
    
    /* Draw label */
    char label_text[256];
    snprintf(label_text, sizeof(label_text), "%s: %d", label, *value);
    graphics_draw_text(ctx->gfx, label_text, ctx->cursor_x, ctx->cursor_y, ctx->style.text, ctx->style.font);
    
    ctx->cursor_y += ctx->row_height;
    
    graphics_rect_t rect = graphics_rect(ctx->cursor_x, ctx->cursor_y, slider_width, slider_height / 2);
    
    bool hovered = point_in_rect(ctx->mouse_x, ctx->mouse_y, &rect);
    bool changed = false;
    
    if (hovered && ctx->mouse_down) {
        ctx->active_id = id;
    }
    
    if (ctx->active_id == id && ctx->mouse_down) {
        /* Calculate value from mouse position */
        f32 t = (f32)(ctx->mouse_x - rect.x) / (f32)rect.width;
        t = clamp_float(t, 0.0f, 1.0f);
        i32 new_value = min + (i32)(t * (max - min));
        if (new_value != *value) {
            *value = new_value;
            changed = true;
        }
    }
    
    /* Clamp value */
    *value = clamp_int(*value, min, max);
    
    /* Render track */
    graphics_fill_rect(ctx->gfx, &rect, ctx->style.foreground);
    graphics_draw_rect(ctx->gfx, &rect, ctx->style.border);
    
    /* Render filled portion */
    f32 fill_t = (f32)(*value - min) / (f32)(max - min);
    graphics_rect_t fill_rect = graphics_rect(rect.x, rect.y, (i32)(rect.width * fill_t), rect.height);
    graphics_fill_rect(ctx->gfx, &fill_rect, ctx->style.accent);
    
    /* Render thumb */
    i32 thumb_x = rect.x + (i32)(fill_t * rect.width);
    i32 thumb_y = rect.y + rect.height / 2;
    graphics_fill_circle(ctx->gfx, thumb_x, thumb_y, slider_height / 4, 
                        ctx->active_id == id ? ctx->style.active_color : ctx->style.border);
    
    ctx->cursor_y += slider_height + ctx->style.spacing;
    
    return changed;
}

bool ui_slider_float(ui_context_t* ctx, const char* label, f32* value, f32 min, f32 max) {
    if (!ctx || !label || !value) return false;
    
    /* Convert to int, use int slider, convert back */
    i32 int_value = (i32)((*value - min) / (max - min) * 1000.0f);
    char label_text[256];
    snprintf(label_text, sizeof(label_text), "%s: %.2f", label, *value);
    
    bool changed = ui_slider_int(ctx, label_text, &int_value, 0, 1000);
    
    if (changed) {
        *value = min + (int_value / 1000.0f) * (max - min);
        *value = clamp_float(*value, min, max);
    }
    
    return changed;
}

/* Other widgets */
void ui_separator(ui_context_t* ctx) {
    if (!ctx) return;
    
    i32 width = graphics_get_width(ctx->gfx) - ctx->cursor_x * 2;
    graphics_draw_line(ctx->gfx, ctx->cursor_x, ctx->cursor_y, 
                      ctx->cursor_x + width, ctx->cursor_y, ctx->style.border);
    ctx->cursor_y += ctx->style.spacing * 2;
}

/* Helper to add popup rect */
static void ui_popup_add_rect(ui_context_t* ctx, graphics_rect_t rect, graphics_color_t color) {
    if (ctx->popup_command_count < UI_MAX_POPUP_COMMANDS) {
        ui_popup_cmd_t* cmd = &ctx->popup_commands[ctx->popup_command_count++];
        cmd->type = UI_POPUP_CMD_RECT;
        cmd->rect = rect;
        cmd->color = color;
    }
}

/* Helper to add popup text */
static void ui_popup_add_text(ui_context_t* ctx, const char* text, i32 x, i32 y, graphics_color_t color, graphics_font_t* font) {
    if (ctx->popup_command_count < UI_MAX_POPUP_COMMANDS) {
        ui_popup_cmd_t* cmd = &ctx->popup_commands[ctx->popup_command_count++];
        cmd->type = UI_POPUP_CMD_TEXT;
        cmd->rect.x = x;
        cmd->rect.y = y;
        cmd->color = color;
        cmd->font = font;
        strncpy(cmd->text, text, sizeof(cmd->text) - 1);
    }
}

bool ui_dropdown(ui_context_t* ctx, const char* label, const char** options, i32 option_count, i32* selected_index) {
    ui_id_t id = ui_hash_string(label);
    
    /* Layout */
    graphics_rect_t rect;
    rect.x = ctx->cursor_x;
    rect.y = ctx->cursor_y;
    rect.width = 200; /* Default width */
    
    /* Use available width if in layout */
    i32 screen_width = graphics_get_width(ctx->gfx);
    if (screen_width > 0) {
        i32 avail = screen_width - ctx->cursor_x - ctx->style.padding * 2;
        if (avail > 50 && avail < 400) rect.width = avail;
    }
    
    rect.height = 24;
    
    ctx->cursor_y += rect.height + ctx->style.spacing;
    
    /* Interaction for main button */
    bool hovered = point_in_rect(ctx->mouse_x, ctx->mouse_y, &rect);
    bool clicked = false;
    
    if (hovered) {
        ctx->hot_id = id;
        if (ctx->mouse_down && ctx->active_id == 0) {
            ctx->active_id = id;
        }
    }
    
    if (ctx->active_id == id) {
        if (!ctx->mouse_down && ctx->mouse_was_down && hovered) {
            clicked = true;
            /* Toggle popup */
            if (ctx->open_popup_id == id) {
                ctx->open_popup_id = 0;
            } else {
                ctx->open_popup_id = id;
                /* Set popup position */
                ctx->popup_rect.x = rect.x;
                ctx->popup_rect.y = rect.y + rect.height;
                ctx->popup_rect.width = rect.width;
                ctx->popup_rect.height = option_count * 24; /* Estimate height */
                ctx->popup_cursor_y = ctx->popup_rect.y;
            }
        }
    }
    
    /* Render main button */
    graphics_fill_rect(ctx->gfx, &rect, ctx->style.background);
    graphics_draw_rect(ctx->gfx, &rect, ctx->style.border);
    
    /* Draw current selection */
    const char* current_text = (selected_index && *selected_index >= 0 && *selected_index < option_count) 
                             ? options[*selected_index] : "Select...";
    graphics_draw_text(ctx->gfx, current_text, rect.x + 5, rect.y + 4, ctx->style.text, ctx->style.font);
    
    /* Draw arrow */
    graphics_draw_triangle(ctx->gfx, 
        rect.x + rect.width - 15, rect.y + 8,
        rect.x + rect.width - 5, rect.y + 8,
        rect.x + rect.width - 10, rect.y + 16,
        ctx->style.text);
        
    /* Handle Popup Logic */
    if (ctx->open_popup_id == id) {
        /* Check if clicked outside to close */
        if (ctx->mouse_down && !ctx->mouse_was_down) {
            if (!point_in_rect(ctx->mouse_x, ctx->mouse_y, &ctx->popup_rect) && 
                !point_in_rect(ctx->mouse_x, ctx->mouse_y, &rect)) {
                ctx->open_popup_id = 0;
            }
        }
        
        /* Queue popup drawing */
        /* Background */
        ui_popup_add_rect(ctx, ctx->popup_rect, ctx->style.background);
        
        /* Options */
        for (i32 i = 0; i < option_count; i++) {
            graphics_rect_t item_rect = {
                .x = ctx->popup_rect.x,
                .y = ctx->popup_rect.y + (i * 24),
                .width = ctx->popup_rect.width,
                .height = 24
            };
            
            bool item_hovered = point_in_rect(ctx->mouse_x, ctx->mouse_y, &item_rect);
            
            if (item_hovered) {
                ui_popup_add_rect(ctx, item_rect, ctx->style.hover);
                
                if (ctx->mouse_down && !ctx->mouse_was_down) {
                    if (selected_index) *selected_index = i;
                    ctx->open_popup_id = 0; /* Close on select */
                    return true; /* Changed */
                }
            }
            
            ui_popup_add_text(ctx, options[i], item_rect.x + 5, item_rect.y + 4, ctx->style.text, ctx->style.font);
        }
    }
    
    return false;
}

/* Menu Bar */
bool ui_begin_menu_bar(ui_context_t* ctx) {
    if (!ctx) return false;
    
    /* Draw menu bar background across full width */
    i32 width = graphics_get_width(ctx->gfx);
    graphics_rect_t bar_rect = {0, 0, width, 24};
    graphics_fill_rect(ctx->gfx, &bar_rect, ctx->style.background);
    graphics_draw_line(ctx->gfx, 0, 24, width, 24, ctx->style.border);
    
    /* Reset cursor to start of menu bar */
    ctx->cursor_x = 5;
    ctx->cursor_y = 0;
    ctx->row_height = 24;
    
    return true;
}

void ui_end_menu_bar(ui_context_t* ctx) {
    if (!ctx) return;
    
    /* Reset cursor for content below menu bar */
    ctx->cursor_x = ctx->style.padding;
    ctx->cursor_y = 24 + ctx->style.spacing;
}

bool ui_begin_menu(ui_context_t* ctx, const char* label) {
    ui_id_t id = ui_hash_string(label);
    
    /* Calculate text width */
    i32 text_w, text_h;
    graphics_measure_text(label, ctx->style.font, &text_w, &text_h);
    
    graphics_rect_t rect = {
        .x = ctx->cursor_x,
        .y = ctx->cursor_y,
        .width = text_w + 20,
        .height = 24
    };
    
    /* Advance cursor */
    ctx->cursor_x += rect.width;
    
    /* Interaction */
    bool hovered = point_in_rect(ctx->mouse_x, ctx->mouse_y, &rect);
    bool open = (ctx->open_popup_id == id);
    
    if (hovered || open) {
        graphics_fill_rect(ctx->gfx, &rect, ctx->style.hover);
    }
    
    graphics_draw_text(ctx->gfx, label, rect.x + 10, rect.y + 4, ctx->style.text, ctx->style.font);
    
    /* Click to toggle */
    if (hovered && ctx->mouse_down && !ctx->mouse_was_down) {
        if (open) {
            ctx->open_popup_id = 0;
        } else {
            ctx->open_popup_id = id;
            /* Initialize popup state */
            ctx->popup_rect.x = rect.x;
            ctx->popup_rect.y = rect.y + rect.height;
            ctx->popup_rect.width = 150; /* Default menu width */
            ctx->popup_rect.height = 0; /* Will grow with items */
            ctx->popup_cursor_y = ctx->popup_rect.y;
        }
    }
    
    return (ctx->open_popup_id == id);
}

void ui_end_menu(ui_context_t* ctx) {
    (void)ctx;
}

bool ui_menu_item(ui_context_t* ctx, const char* label) {
    /* Calculate rect based on popup state */
    graphics_rect_t rect = {
        .x = ctx->popup_rect.x,
        .y = ctx->popup_cursor_y,
        .width = ctx->popup_rect.width,
        .height = 24
    };
    
    ctx->popup_cursor_y += 24;
    
    /* Background for this item (menu background) */
    ui_popup_add_rect(ctx, rect, ctx->style.background);
    
    /* Interaction */
    bool hovered = point_in_rect(ctx->mouse_x, ctx->mouse_y, &rect);
    bool clicked = false;
    
    if (hovered) {
        ui_popup_add_rect(ctx, rect, ctx->style.hover);
        if (ctx->mouse_down && !ctx->mouse_was_down) {
            clicked = true;
            ctx->open_popup_id = 0; /* Close menu on click */
        }
    }
    
    ui_popup_add_text(ctx, label, rect.x + 10, rect.y + 4, ctx->style.text, ctx->style.font);
    
    return clicked;
}

void ui_progress_bar(ui_context_t* ctx, f32 fraction) {
    if (!ctx) return;
    
    fraction = clamp_float(fraction, 0.0f, 1.0f);
    i32 width = 200;
    i32 height = 20;
    
    graphics_rect_t rect = graphics_rect(ctx->cursor_x, ctx->cursor_y, width, height);
    
    /* Background */
    graphics_fill_rect(ctx->gfx, &rect, ctx->style.foreground);
    graphics_draw_rect(ctx->gfx, &rect, ctx->style.border);
    
    /* Fill */
    graphics_rect_t fill = graphics_rect(rect.x, rect.y, (i32)(width * fraction), height);
    graphics_fill_rect(ctx->gfx, &fill, ctx->style.accent);
    
    /* Percentage text */
    char text[32];
    snprintf(text, sizeof(text), "%.0f%%", fraction * 100.0f);
    i32 text_width, text_height;
    graphics_measure_text(text, ctx->style.font, &text_width, &text_height);
    graphics_draw_text(ctx->gfx, text, rect.x + (width - text_width) / 2,
                      rect.y + (height - text_height) / 2, ctx->style.text, ctx->style.font);
    
    ctx->cursor_y += rect.height + ctx->style.spacing;
    if (ctx->cursor_y > ctx->max_content_y) ctx->max_content_y = ctx->cursor_y;
    
    if (!ctx->same_line) {
        ctx->cursor_x = ctx->style.padding;
    }
}

void ui_image(ui_context_t* ctx, graphics_image_t* image, i32 width, i32 height) {
    if (!ctx || !image) return;
    
    graphics_rect_t dest = graphics_rect(ctx->cursor_x, ctx->cursor_y, width, height);
    graphics_draw_image_scaled(ctx->gfx, image, &dest);
    
    ctx->cursor_y += height + ctx->style.spacing;
}
/* Enhanced text input with flags and placeholder */
bool ui_text_input_ex(
    ui_context_t* ctx,
    const char* label,
    char* buffer,
    i32 buffer_size,
    u32 flags,
    const char* placeholder
) {
    if (!ctx || !label || !buffer || buffer_size <= 0) return false;
    
    ui_id_t id = ui_hash_string(label);
    i32 input_width = 300;
    i32 input_height = ctx->row_height;
    
    /* Don't draw label here - it's drawn externally with ui_label() */
    /* Just create the input box at current cursor position */
    graphics_rect_t rect = graphics_rect(ctx->cursor_x, ctx->cursor_y, input_width, input_height);
    
    bool hovered = point_in_rect(ctx->mouse_x, ctx->mouse_y, &rect);
    bool changed = false;
    bool readonly = (flags & UI_TEXT_INPUT_READONLY) != 0;
    
    if (hovered && ctx->mouse_down && !ctx->mouse_was_down && !readonly) {
        ctx->focus_id = id;
    }
    
    bool focused = (ctx->focus_id == id);
    
    /* Handle input */
    if (focused && !readonly && ctx->input_char != 0) {
        i32 len = strlen(buffer);
        
        if (ctx->input_char == '\b' || ctx->input_char == 127) {
            /* Backspace */
            if (len > 0) {
                buffer[len - 1] = '\0';
                changed = true;
            }
        } else if (ctx->input_char >= 32 && ctx->input_char < 127) {
            /* Validate based on flags */
            bool allow_char = true;
            
            if (flags & UI_TEXT_INPUT_NUMERIC) {
                /* Only allow numbers and decimal point */
                if (!(ctx->input_char >= '0' && ctx->input_char <= '9') &&
                    ctx->input_char != '.' && ctx->input_char != '-') {
                    allow_char = false;
                }
            }
            
            /* Printable character */
            if (allow_char && len < buffer_size - 1) {
                buffer[len] = ctx->input_char;
                buffer[len + 1] = '\0';
                changed = true;
            }
        }
    }
    
    /* Render */
    graphics_color_t bg_color = focused ? ctx->style.hover : ctx->style.foreground;
    graphics_fill_rect(ctx->gfx, &rect, bg_color);
    graphics_color_t border_color = focused ? ctx->style.accent : ctx->style.border;
    graphics_draw_rect(ctx->gfx, &rect, border_color);
    
    /* Draw text or placeholder */
    i32 text_x = rect.x + ctx->style.padding;
    i32 text_y = rect.y + ctx->style.padding;
    
    if (strlen(buffer) > 0) {
        /* Display password as asterisks if flag is set */
        if (flags & UI_TEXT_INPUT_PASSWORD) {
            char display[256] = "";
            i32 len = strlen(buffer);
            for (i32 i = 0; i < len && i < 255; i++) {
                display[i] = '*';
            }
            display[len] = '\0';
            graphics_draw_text(ctx->gfx, display, text_x, text_y, ctx->style.text, ctx->style.font);
        } else {
            graphics_draw_text(ctx->gfx, buffer, text_x, text_y, ctx->style.text, ctx->style.font);
        }
    } else if (placeholder && !focused) {
        /* Show placeholder in gray */
        graphics_draw_text(ctx->gfx, placeholder, text_x, text_y, ctx->style.border, ctx->style.font);
    }
    
    /* Draw blinking cursor */
    if (focused && (ctx->frame_count / 30) % 2 == 0) {
        i32 text_width = 0, text_height;
        if (strlen(buffer) > 0) {
            if (flags & UI_TEXT_INPUT_PASSWORD) {
                i32 len = strlen(buffer);
                text_width = len * 8;  /* Approximate width for asterisks */
            } else {
                graphics_measure_text(buffer, ctx->style.font, &text_width, &text_height);
            }
        }
        graphics_draw_line(ctx->gfx,
            text_x + text_width, rect.y + 4,
            text_x + text_width, rect.y + input_height - 4,
            ctx->style.text);
    }
    
    ctx->cursor_y += input_height + ctx->style.spacing;
    
    return changed;
}

/* Multiline text area */
bool ui_text_area(
    ui_context_t* ctx,
    const char* label,
    char* buffer,
    i32 buffer_size,
    i32 height_lines
) {
    if (!ctx || !label || !buffer || buffer_size <= 0) return false;
    
    ui_id_t id = ui_hash_string(label);
    i32 area_width = 400;
    i32 area_height = ctx->row_height * height_lines;
    
    /* Draw label above */
    graphics_draw_text(ctx->gfx, label, ctx->cursor_x, ctx->cursor_y, 
                      ctx->style.text, ctx->style.font);
    ctx->cursor_y += ctx->row_height;
    
    graphics_rect_t rect = graphics_rect(ctx->cursor_x, ctx->cursor_y, area_width, area_height);
    
    bool hovered = point_in_rect(ctx->mouse_x, ctx->mouse_y, &rect);
    bool changed = false;
    
    if (hovered && ctx->mouse_down && !ctx->mouse_was_down) {
        ctx->focus_id = id;
    }
    
    bool focused = (ctx->focus_id == id);
    
    /* Handle input */
    if (focused && ctx->input_char != 0) {
        i32 len = strlen(buffer);
        
        if (ctx->input_char == '\b' || ctx->input_char == 127) {
            /* Backspace */
            if (len > 0) {
                buffer[len - 1] = '\0';
                changed = true;
            }
        } else if (ctx->input_char == '\n' || ctx->input_char == '\r') {
            /* Newline */
            if (len < buffer_size - 1) {
                buffer[len] = '\n';
                buffer[len + 1] = '\0';
                changed = true;
            }
        } else if (ctx->input_char >= 32 && ctx->input_char < 127) {
            /* Printable character */
            if (len < buffer_size - 1) {
                buffer[len] = ctx->input_char;
                buffer[len + 1] = '\0';
                changed = true;
            }
        }
    }
    
    /* Render */
    graphics_color_t bg_color = focused ? ctx->style.hover : ctx->style.foreground;
    graphics_fill_rect(ctx->gfx, &rect, bg_color);
    graphics_color_t border_color = focused ? ctx->style.accent : ctx->style.border;
    graphics_draw_rect(ctx->gfx, &rect, border_color);
    
    /* Draw text (simple, no word wrap for now) */
    graphics_draw_text(ctx->gfx, buffer, rect.x + ctx->style.padding,
                     rect.y + ctx->style.padding, ctx->style.text, ctx->style.font);
    
    ctx->cursor_y += area_height + ctx->style.spacing;
    
    return changed;
}

/* Check if last text input was submitted (Enter pressed) */
bool ui_text_input_submitted(ui_context_t* ctx) {
    if (!ctx) return false;
    /* Check if Enter key was pressed with focus on a text input */
    return (ctx->focus_id != 0 && ctx->last_key == ENGINE_KEY_ENTER && ctx->key_down);
}
