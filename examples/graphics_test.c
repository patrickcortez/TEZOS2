#include "../include/engine.h"
#include "../include/graphics.h"
#include <stdio.h>
#include <math.h>

/* Simple demo state */
typedef struct {
    graphics_context_t* gfx;
    float time;
    int frame;
} demo_state_t;

void on_event(const engine_event_t* event, void* user_data) {
    demo_state_t* state = (demo_state_t*)user_data;
    
    if (event->type == ENGINE_EVENT_WINDOW_RESIZE) {
        printf("Window resized to %dx%d\n",
               event->data.resize.width,
               event->data.resize.height);
        
        /* Resize graphics context */
        if (state->gfx) {
            graphics_resize(state->gfx, 
                          event->data.resize.width,
                          event->data.resize.height);
        }
    }
}

int main(void) {
    printf("=== Graphics Test ===\n");
    
    /* Initialize engine */
    engine_config_t engine_config = {
        .app_name = "Graphics Test",
        .enable_logging = true,
    };
    
    if (engine_init(&engine_config) != ENGINE_SUCCESS) {
        fprintf(stderr, "Failed to initialize engine\n");
        return 1;
    }
    
    /* Create demo state */
    demo_state_t state = {0};
    
    /* Create window */
    engine_window_config_t window_config = {
        .title = "Graphics Test - 2D Engine",
        .width = 800,
        .height = 600,
        .resizable = true,
        .event_callback = on_event,
        .user_data = &state,
    };
    
    engine_window_t* window = NULL;
    if (engine_window_create(&window_config, &window) != ENGINE_SUCCESS) {
        fprintf(stderr, "Failed to create window\n");
        engine_shutdown();
        return 1;
    }
    
    /* Create graphics context */
    state.gfx = graphics_create_context(800, 600);
    if (!state.gfx) {
        fprintf(stderr, "Failed to create graphics context\n");
        engine_window_destroy(window);
        engine_shutdown();
        return 1;
    }
    
    printf("Graphics test started!\n");
    printf("Watch the window for animated graphics.\n\n");
    
    /* Main loop */
    while (!engine_window_should_close(window)) {
        engine_poll_events();
        
        state.time = (float)engine_get_time();
        state.frame++;
        
        i32 width = graphics_get_width(state.gfx);
        i32 height = graphics_get_height(state.gfx);
        
        /* Clear to dark blue */
        graphics_clear(state.gfx, graphics_rgb(20, 30, 48));
        
        /* Draw animated shapes */
        
        /* 1. Bouncing circle */
        i32 circle_x = width / 2 + (i32)(cosf(state.time) * 200);
        i32 circle_y = height / 2 + (i32)(sinf(state.time * 1.5f) * 150);
        graphics_fill_circle(state.gfx, circle_x, circle_y, 30, COLOR_YELLOW);
        graphics_draw_circle(state.gfx, circle_x, circle_y, 30, COLOR_WHITE);
        
        /* 2. Rotating rectangles */
        for (int i = 0; i < 4; i++) {
            float angle = state.time + i * 3.14159f / 2;
            i32 rx = width / 2 + (i32)(cosf(angle) * 150);
            i32 ry = height / 2 + (i32)(sinf(angle) * 150);
            
            graphics_color_t color = graphics_rgb(
                (u8)(128 + 127 * sinf(state.time + i)),
                (u8)(128 + 127 * cosf(state.time + i)),
                (u8)(128 + 127 * sinf(state.time + i + 1))
            );
            
            graphics_rect_t rect = graphics_rect(rx - 20, ry - 20, 40, 40);
            graphics_fill_rect(state.gfx, &rect, color);
        }
        
        /* 3. Grid pattern */
        graphics_color_t grid_color = graphics_rgba(100, 150, 200, 128);
        for (i32 x = 0; x < width; x += 50) {
            graphics_draw_line(state.gfx, x, 0, x,  height, grid_color);
        }
        for (i32 y = 0; y < height; y += 50) {
            graphics_draw_line(state.gfx, 0, y, width, y, grid_color);
        }
        
        /* 4. Corner triangles */
        graphics_fill_triangle(state.gfx, 
            0, 0, 
            100, 0, 
            0, 100, 
            COLOR_RED);
        graphics_fill_triangle(state.gfx,
            width, 0,
            width - 100, 0,
            width, 100,
            COLOR_GREEN);
        graphics_fill_triangle(state.gfx,
            0, height,
            100, height,
            0, height - 100,
            COLOR_BLUE);
        graphics_fill_triangle(state.gfx,
            width, height,
            width - 100, height,
            width, height - 100,
            COLOR_MAGENTA);
        
        /* 5. Text */
        char text[256];
        snprintf(text, sizeof(text), "2D Engine - Graphics Test");
        graphics_draw_text(state.gfx, text, 10, 10, COLOR_WHITE, NULL);
        
        snprintf(text, sizeof(text), "Frame: %d  Time: %.2fs", state.frame, state.time);
        graphics_draw_text(state.gfx, text, 10, 25, COLOR_CYAN, NULL);
        
        snprintf(text, sizeof(text), "Resolution: %dx%d", width, height);
        graphics_draw_text(state.gfx, text, 10, 40, COLOR_GRAY, NULL);
        
        /* Center text */
        const char* center_text = "Software Renderer";
        i32 text_width, text_height;
        graphics_measure_text(center_text, NULL, &text_width, &text_height);
        graphics_draw_text(state.gfx, center_text, 
                         width / 2 - text_width / 2,
                         height - 30,
                         COLOR_YELLOW, NULL);
        
        /* Draw some lines radiating from center */
        for (int i = 0; i < 12; i++) {
            float angle = (state.time * 2 + i * 3.14159f / 6);
            i32 x1 = width / 2;
            i32 y1 = height / 2;
            i32 x2 = x1 + (i32)(cosf(angle) * 100);
            i32 y2 = y1 + (i32)(sinf(angle) * 100);
            
            graphics_color_t line_color = graphics_rgb(
                (u8)(255 * (i % 3 == 0)),
                (u8)(255 * (i % 3 == 1)),
                (u8)(255 * (i % 3 == 2))
            );
            
            graphics_draw_line(state.gfx, x1, y1, x2, y2, line_color);
        }
        
        /* Present to window */
        /* Access internal platform window from engine window */
        /* Note: This is a workaround - in a real implementation, we'd add
         * a proper graphics_present() function to the engine API */
        extern struct engine_window {
            void* platform_window;
        };
        struct engine_window* win_internal = (struct engine_window*)window;
        
        platform_window_present_buffer(
            (platform_window_t*)win_internal->platform_window,
            graphics_get_pixels(state.gfx),
            width,
            height
        );
        
        /* Cap frame rate to ~60 FPS */
        platform_sleep(16);
    }
    
    /* Cleanup */
    printf("\nCleaning up...\n");
    graphics_destroy_context(state.gfx);
    engine_window_destroy(window);
    engine_shutdown();
    
    printf("Graphics test complete!\n");
    return 0;
}
