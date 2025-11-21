#include "../include/window.h"
#include "../include/graphics.h"
#include "../include/ui.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_WINDOWS 32
#define TITLE_BAR_HEIGHT 24
#define RESIZE_HANDLE_SIZE 8
#define BORDER_SIZE 1

/* Window manager structure */
struct window_manager {
    window_t* windows[MAX_WINDOWS];
    i32 window_count;
    i32 focused_window_id;
    i32 next_window_id;
};

/* Window manager creation/destruction */
window_manager_t* window_manager_create(void) {
    window_manager_t* wm = (window_manager_t*)calloc(1, sizeof(window_manager_t));
    if (!wm) return NULL;
    
    wm->next_window_id = 1;
    wm->focused_window_id = -1;
    return wm;
}

void window_manager_destroy(window_manager_t* wm) {
    if (!wm) return;
    
    for (i32 i = 0; i < wm->window_count; i++) {
        if (wm->windows[i]) {
            free(wm->windows[i]);
        }
    }
    
    free(wm);
}

/* Window creation/destruction */
window_t* window_create(window_manager_t* wm, const char* title, i32 x, i32 y, i32 width, i32 height) {
    if (!wm || wm->window_count >= MAX_WINDOWS) return NULL;
    
    window_t* window = (window_t*)calloc(1, sizeof(window_t));
    if (!window) return NULL;
    
    window->id = wm->next_window_id++;
    strncpy(window->title, title ? title : "Window", sizeof(window->title) - 1);
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->visible = true;
    window->focused = false;
    window->state = WINDOW_STATE_NORMAL;
    window->flags = WINDOW_FLAG_RESIZABLE | WINDOW_FLAG_CLOSABLE | WINDOW_FLAG_MINIMIZABLE;
    window->min_width = 200;
    window->min_height = 100;
    
    wm->windows[wm->window_count++] = window;
    window_focus(wm, window);
    
    return window;
}

void window_destroy(window_manager_t* wm, window_t* window) {
    if (!wm || !window) return;
    
    /* Find and remove window from array */
    for (i32 i = 0; i < wm->window_count; i++) {
        if (wm->windows[i] == window) {
            /* Shift remaining windows */
            for (i32 j = i; j < wm->window_count - 1; j++) {
                wm->windows[j] = wm->windows[j + 1];
            }
            wm->window_count--;
            
            /* Update focus */
            if (wm->focused_window_id == window->id) {
                wm->focused_window_id = (wm->window_count > 0) ? wm->windows[wm->window_count - 1]->id : -1;
            }
            
            free(window);
            return;
        }
    }
}

void window_close(window_manager_t* wm, window_t* window) {
    window_destroy(wm, window);
}

/* Window properties */
void window_set_title(window_t* window, const char* title) {
    if (!window || !title) return;
    strncpy(window->title, title, sizeof(window->title) - 1);
}

void window_set_position(window_t* window, i32 x, i32 y) {
    if (!window) return;
    window->x = x;
    window->y = y;
}

void window_set_size(window_t* window, i32 width, i32 height) {
    if (!window) return;
    if (width < window->min_width) width = window->min_width;
    if (height < window->min_height) height = window->min_height;
    window->width = width;
    window->height = height;
}

void window_set_visible(window_t* window, bool visible) {
    if (!window) return;
    window->visible = visible;
}

void window_focus(window_manager_t* wm, window_t* window) {
    if (!wm || !window) return;
    
    /* Unfocus all windows */
    for (i32 i = 0; i < wm->window_count; i++) {
        wm->windows[i]->focused = false;
    }
    
    /* Focus this window */
    window->focused = true;
    wm->focused_window_id = window->id;
    
    /* Bring to front (move to end of array) */
    for (i32 i = 0; i < wm->window_count; i++) {
        if (wm->windows[i] == window) {
            /* Shift windows after this one back */
            window_t* temp = window;
            for (i32 j = i; j < wm->window_count - 1; j++) {
                wm->windows[j] = wm->windows[j + 1];
            }
            wm->windows[wm->window_count - 1] = temp;
            break;
        }
    }
}

/* Helper: Check if point is in title bar */
static bool point_in_title_bar(window_t* window, i32 x, i32 y) {
    return x >= window->x && x < window->x + window->width &&
           y >= window->y && y < window->y + TITLE_BAR_HEIGHT;
}

/* Helper: Check if point is in close button */
static bool point_in_close_button(window_t* window, i32 x, i32 y) {
    i32 btn_x = window->x + window->width - TITLE_BAR_HEIGHT;
    i32 btn_y = window->y;
    return x >= btn_x && x < btn_x + TITLE_BAR_HEIGHT &&
           y >= btn_y && y < btn_y + TITLE_BAR_HEIGHT;
}

/* Helper: Check if point is in resize handle */
static bool point_in_resize_handle(window_t* window, i32 x, i32 y) {
    i32 handle_x = window->x + window->width - RESIZE_HANDLE_SIZE;
    i32 handle_y = window->y + window->height - RESIZE_HANDLE_SIZE;
    return x >= handle_x && x < handle_x + RESIZE_HANDLE_SIZE &&
           y >= handle_y && y < handle_y + RESIZE_HANDLE_SIZE;
}

/* Helper: Check if point is in window body */
static bool point_in_window(window_t* window, i32 x, i32 y) {
    return x >= window->x && x < window->x + window->width &&
           y >= window->y && y < window->y + window->height;
}

/* Window manager update (handles input) */
void window_manager_update(window_manager_t* wm, i32 mouse_x, i32 mouse_y, bool mouse_down, bool mouse_was_down) {
    if (!wm) return;
    
    /* Process windows from front to back */
    for (i32 i = wm->window_count - 1; i >= 0; i--) {
        window_t* window = wm->windows[i];
        if (!window->visible || window->state == WINDOW_STATE_MINIMIZED) continue;
        
        /* Handle dragging */
        if (window->is_dragging) {
            if (mouse_down) {
                window->x = mouse_x - window->drag_offset_x;
                window->y = mouse_y - window->drag_offset_y;
            } else {
                window->is_dragging = false;
            }
            return; /* Only handle one window at a time */
        }
        
        /* Handle resizing */
        if (window->is_resizing) {
            if (mouse_down) {
                i32 new_width = mouse_x - window->x;
                i32 new_height = mouse_y - window->y;
                window_set_size(window, new_width, new_height);
            } else {
                window->is_resizing = false;
            }
            return;
        }
        
        /* Check for new interactions */
        if (mouse_down && !mouse_was_down) {
            /* Close button */
            if ((window->flags & WINDOW_FLAG_CLOSABLE) && point_in_close_button(window, mouse_x, mouse_y)) {
                window_close(wm, window);
                return;
            }
            
            /* Start dragging title bar */
            if (point_in_title_bar(window, mouse_x, mouse_y) && !point_in_close_button(window, mouse_x, mouse_y)) {
                window_focus(wm, window);
                window->is_dragging = true;
                window->drag_offset_x = mouse_x - window->x;
                window->drag_offset_y = mouse_y - window->y;
                return;
            }
            
            /* Start resizing */
            if ((window->flags & WINDOW_FLAG_RESIZABLE) && point_in_resize_handle(window, mouse_x, mouse_y)) {
                window_focus(wm, window);
                window->is_resizing = true;
                return;
            }
            
            /* Focus window */
            if (point_in_window(window, mouse_x, mouse_y)) {
                window_focus(wm, window);
                return;
            }
        }
    }
}

/* Window manager render (draws all windows) */
void window_manager_render(window_manager_t* wm, graphics_context_t* gfx, graphics_font_t* font) {
    if (!wm || !gfx) return;
    
    /* Render windows from back to front */
    for (i32 i = 0; i < wm->window_count; i++) {
        window_t* window = wm->windows[i];
        if (!window->visible || window->state == WINDOW_STATE_MINIMIZED) continue;
        
        /* Window background */
        graphics_rect_t bg_rect = graphics_rect(window->x, window->y, window->width, window->height);
        graphics_fill_rect(gfx, &bg_rect, graphics_rgb(45, 45, 48));
        
        /* Window border */
        graphics_draw_rect(gfx, &bg_rect, window->focused ? graphics_rgb(0, 122, 204) : graphics_rgb(100, 100, 105));
        
        /* Title bar */
        graphics_rect_t title_rect = graphics_rect(window->x, window->y, window->width, TITLE_BAR_HEIGHT);
        graphics_fill_rect(gfx, &title_rect, window->focused ? graphics_rgb(0, 122, 204) : graphics_rgb(60, 60, 65));
        
        /* Title text */
        if (font) {
            graphics_draw_text(gfx, window->title, window->x + 8, window->y + 4, graphics_rgb(255, 255, 255), font);
        }
        
        /* Close button */
        if (window->flags & WINDOW_FLAG_CLOSABLE) {
            i32 btn_x = window->x + window->width - TITLE_BAR_HEIGHT;
            i32 btn_y = window->y;
            graphics_rect_t close_btn = graphics_rect(btn_x + 4, btn_y + 4, TITLE_BAR_HEIGHT - 8, TITLE_BAR_HEIGHT - 8);
            graphics_fill_rect(gfx, &close_btn, graphics_rgb(200, 80, 80));
            
            /* X symbol */
            graphics_draw_line(gfx, btn_x + 8, btn_y + 8, btn_x + TITLE_BAR_HEIGHT - 8, btn_y + TITLE_BAR_HEIGHT - 8, graphics_rgb(255, 255, 255));
            graphics_draw_line(gfx, btn_x + TITLE_BAR_HEIGHT - 8, btn_y + 8, btn_x + 8, btn_y + TITLE_BAR_HEIGHT - 8, graphics_rgb(255, 255, 255));
        }
        
        /* Resize handle */
        if (window->flags & WINDOW_FLAG_RESIZABLE) {
            i32 handle_x = window->x + window->width - RESIZE_HANDLE_SIZE;
            i32 handle_y = window->y + window->height - RESIZE_HANDLE_SIZE;
            graphics_rect_t handle = graphics_rect(handle_x, handle_y, RESIZE_HANDLE_SIZE, RESIZE_HANDLE_SIZE);
            graphics_fill_rect(gfx, &handle, graphics_rgb(100, 100, 105));
        }
    }
}

/* Window content area begin/end */
bool window_begin(window_manager_t* wm, window_t* window) {
    if (!wm || !window || !window->visible || window->state == WINDOW_STATE_MINIMIZED) {
        return false;
    }
    
    /* Content rendering happens via ui_begin_window/ui_end_window in user code */
    /* This function just checks if the window is ready for content */
    return true;
}

void window_end(window_manager_t* wm, window_t* window) {
    /* Nothing to do - window rendering is handled by window_manager_render */
    (void)wm;
    (void)window;
}
