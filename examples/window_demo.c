#include "../include/engine.h"
#include "../include/ui.h"
#include "../include/window.h"
#include <stdio.h>
#include <stdlib.h>

/* Demo state */
typedef struct {
    graphics_context_t* gfx;
    ui_context_t* ui;
    window_manager_t* wm;
    
    window_t* window1;
    window_t* window2;
    window_t* window3;
    
    /* Track mouse state for window manager */
    i32 mouse_x, mouse_y;
    bool mouse_down;
    bool mouse_was_down;
    
    f32 time;
} demo_state_t;

/* Event handler */
void on_event(const engine_event_t* event, void* user_data) {
    demo_state_t* state = (demo_state_t*)user_data;
    
    switch (event->type) {
        case ENGINE_EVENT_WINDOW_RESIZE:
            if (state->gfx) {
                graphics_resize(state->gfx,
                              event->data.resize.width,
                              event->data.resize.height);
            }
            break;
            
        case ENGINE_EVENT_MOUSE_MOVE:
            state->mouse_x = event->data.mouse_move.x;
            state->mouse_y = event->data.mouse_move.y;
            ui_input_mouse_move(state->ui, event->data.mouse_move.x, event->data.mouse_move.y);
            break;
            
        case ENGINE_EVENT_MOUSE_BUTTON_PRESS:
            state->mouse_down = true;
            ui_input_mouse_button(state->ui, true);
            break;
            
        case ENGINE_EVENT_MOUSE_BUTTON_RELEASE:
            state->mouse_down = false;
            ui_input_mouse_button(state->ui, false);
            break;
            
        case ENGINE_EVENT_MOUSE_WHEEL:
            ui_input_mouse_wheel(state->ui, event->data.mouse_wheel.delta);
            break;
            
        default:
            break;
    }
}

int main(void) {
    printf("=== Window Manager Demo ===\n");
    
    /* Initialize engine */
    engine_config_t engine_config = {
        .app_name = "Window Manager Demo",
        .enable_logging = true,
    };
    
    if (engine_init(&engine_config) != ENGINE_SUCCESS) {
        fprintf(stderr, "Failed to initialize engine\n");
        return 1;
    }
    
    demo_state_t state = {0};
    state.time = 0.0f;
    
    /* Create window */
    engine_window_config_t window_config = {
        .title = "Window Manager Demo",
        .width = 1024,
        .height = 768,
        .resizable = false,
        .event_callback = on_event,
        .user_data = &state,
    };
    
    engine_window_t* window = NULL;
    if (engine_window_create(&window_config, &window) != ENGINE_SUCCESS) {
        fprintf(stderr, "Failed to create window\n");
        engine_shutdown();
        return 1;
    }
    
    state.gfx = graphics_create_context(1024, 768);
    state.ui = ui_create_context(state.gfx);
    state.wm = window_manager_create();
    
    if (!state.gfx || !state.ui || !state.wm) {
        fprintf(stderr, "Failed to create contexts\n");
        return 1;
    }
    
    printf("Window Manager Demo initialized!\n");
    printf("Try dragging, resizing, and closing windows.\n\n");
    
    /* Create three demo windows */
    state.window1 = window_create(state.wm, "Welcome Window", 100, 100, 400, 300);
    state.window2 = window_create(state.wm, "Settings", 520, 150, 350, 250);
    state.window3 = window_create(state.wm, "Info Panel", 250, 420, 500, 200);
    
    /* Set window properties */
    if (state.window3) {
        state.window3->min_width = 300;
        state.window3->min_height = 150;
    }
    
    /* Main loop */
    while (!engine_window_should_close(window)) {
        engine_poll_events();
        
        state.time += 1.0f / 60.0f;
        
        /* Update mouse_was_down at start of frame */
        state.mouse_was_down = state.mouse_down;
        
        /* Clear background */
        graphics_clear(state.gfx, graphics_rgb(40, 40, 45));
        
        /* Begin UI frame */
        ui_begin_frame(state.ui);
        
        /* Update window manager (handles dragging, resizing, etc.) */
        window_manager_update(state.wm, 
                            state.mouse_x, state.mouse_y,
                            state.mouse_down, state.mouse_was_down);
        
        /* Render window backgrounds and decorations */
        graphics_font_t* font = NULL; /* Use default font */
        window_manager_render(state.wm, state.gfx, font);
        
        /* Render window 1 content */
        if (window_begin(state.wm, state.window1)) {
            ui_label(state.ui, "Welcome to the Window Manager Demo!");
            ui_spacing(state.ui, 10);
            ui_label(state.ui, "You can:");
            ui_label(state.ui, "- Drag windows by their title bars");
            ui_label(state.ui, "- Resize windows by dragging the corner");
            ui_label(state.ui, "- Close windows with the X button");
            ui_label(state.ui, "- Click windows to bring them to front");
            
            window_end(state.wm, state.window1);
        }
        
        /* Render window 2 content */
        if (window_begin(state.wm, state.window2)) {
            ui_label(state.ui, "Settings Panel");
            ui_separator(state.ui);
            ui_spacing(state.ui, 5);
            
            static bool option1 = true;
            static bool option2 = false;
            ui_checkbox(state.ui, "Enable Feature A", &option1);
            ui_checkbox(state.ui, "Enable Feature B", &option2);
            
            ui_spacing(state.ui, 10);
            if (ui_button(state.ui, "Apply Settings")) {
                printf("Settings applied!\n");
            }
            
            window_end(state.wm, state.window2);
        }
        
        /* Render window 3 content */
        if (window_begin(state.wm, state.window3)) {
            ui_label(state.ui, "System Information");
            ui_separator(state.ui);
            ui_spacing(state.ui, 5);
            
            char fps_text[64];
            snprintf(fps_text, sizeof(fps_text), "FPS: ~60");
            ui_label(state.ui, fps_text);
            
            snprintf(fps_text, sizeof(fps_text), "Time: %.1f seconds", state.time);
            ui_label(state.ui, fps_text);
            
            window_end(state.wm, state.window3);
        }
        
        /* End UI frame */
        ui_end_frame(state.ui);
        
        /* Render to screen */
        platform_window_t* plat_window = engine_window_get_platform_window(window);
        platform_window_present_buffer(plat_window, graphics_get_pixels(state.gfx), 
                                      graphics_get_width(state.gfx), graphics_get_height(state.gfx));
        
        platform_sleep(16);
    }
    
    /* Cleanup */
    window_manager_destroy(state.wm);
    ui_destroy_context(state.ui);
    graphics_destroy_context(state.gfx);
    engine_window_destroy(window);
    engine_shutdown();
    
    return 0;
}
