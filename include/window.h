#ifndef WINDOW_H
#define WINDOW_H

#include "types.h"
#include "graphics.h"
#include "ui.h"

/* Forward declarations */
typedef struct window window_t;
typedef struct window_manager window_manager_t;

/* Window flags */
typedef enum {
    WINDOW_FLAG_RESIZABLE = 1 << 0,
    WINDOW_FLAG_CLOSABLE = 1 << 1,
    WINDOW_FLAG_MINIMIZABLE = 1 << 2,
} window_flags_t;

/* Window state */
typedef enum {
    WINDOW_STATE_NORMAL,
    WINDOW_STATE_MINIMIZED,
    WINDOW_STATE_MAXIMIZED,
} window_state_t;

/* Window structure */
struct window {
    i32 id;
    char title[256];
    i32 x, y, width, height;
    bool visible;
    bool focused;
    window_state_t state;
    u32 flags;
    void* user_data;
    
    /* Internal state */
    bool is_dragging;
    bool is_resizing;
    i32 drag_offset_x, drag_offset_y;
    i32 min_width, min_height;
};

/* Window manager API */
ENGINE_API window_manager_t* window_manager_create(void);
ENGINE_API void window_manager_destroy(window_manager_t* wm);

ENGINE_API window_t* window_create(window_manager_t* wm, const char* title, i32 x, i32 y, i32 width, i32 height);
ENGINE_API void window_destroy(window_manager_t* wm, window_t* window);
ENGINE_API void window_close(window_manager_t* wm, window_t* window);

ENGINE_API void window_set_title(window_t* window, const char* title);
ENGINE_API void window_set_position(window_t* window, i32 x, i32 y);
ENGINE_API void window_set_size(window_t* window, i32 width, i32 height);
ENGINE_API void window_set_visible(window_t* window, bool visible);
ENGINE_API void window_focus(window_manager_t* wm, window_t* window);

ENGINE_API void window_manager_update(window_manager_t* wm, i32 mouse_x, i32 mouse_y, bool mouse_down, bool mouse_was_down);
ENGINE_API void window_manager_render(window_manager_t* wm, graphics_context_t* gfx, graphics_font_t* font);

ENGINE_API bool window_begin(window_manager_t* wm, window_t* window);
ENGINE_API void window_end(window_manager_t* wm, window_t* window);

#endif /* WINDOW_H */
