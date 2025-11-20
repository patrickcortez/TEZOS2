#include "../include/engine.h"
#include "../include/graphics.h"
#include "../include/input.h"
#include <stdio.h>
#include <math.h>

/* Particle structure */
typedef struct {
    i32 x, y;
    f32 lifetime;
    graphics_color_t color;
} particle_t;

/* Demo state */
typedef struct {
    graphics_context_t* gfx;
    
    /* Player square (WASD movement) */
    i32 player_x, player_y;
    i32 player_size;
    graphics_color_t player_color;
    
    /* Clicks create particles */
    particle_t particles[32];
    i32 particle_count;
    
    /* Visual feedback */
    bool space_pressed;
    i32 space_timer;
} demo_state_t;

void on_event(const engine_event_t* event, void* user_data) {
    demo_state_t* state = (demo_state_t*)user_data;
    
    /* Process input events */
    input_process_event(event);
    
    if (event->type == ENGINE_EVENT_WINDOW_RESIZE) {
        if (state->gfx) {
            graphics_resize(state->gfx, 
                          event->data.resize.width,
                          event->data.resize.height);
        }
    }
}

int main(void) {
    printf("=== Input Demo ===\n");
    printf("Controls:\n");
    printf("  WASD - Move square\n");
    printf("  Arrow Keys - Also move square\n");
    printf("  SPACE - Flash effect\n");
    printf("  Left Click - Spawn particle\n");
    printf("  ESC - Exit\n\n");
    
    /* Initialize engine */
    engine_config_t engine_config = {
        .app_name = "Input Demo",
        .enable_logging = true,
    };
    
    if (engine_init(&engine_config) != ENGINE_SUCCESS) {
        fprintf(stderr, "Failed to initialize engine\n");
        return 1;
    }
    
    /* Initialize input system */
    input_init();
    
    /* Create window */
    demo_state_t state = {0};
    state.player_x = 400;
    state.player_y = 300;
    state.player_size = 40;
    state.player_color = graphics_rgb(0, 200, 255);
    
    engine_window_config_t window_config = {
        .title = "Input Demo - 2D Engine",
        .width = 800,
        .height = 600,
        .resizable = true,
        .event_callback = on_event,
        .user_data = &state,
    };
    
    engine_window_t* window = NULL;
    if (engine_window_create(&window_config, &window) != ENGINE_SUCCESS) {
        fprintf(stderr, "Failed to create window\n");
        input_shutdown();
        engine_shutdown();
        return 1;
    }
    
    /* Create graphics context */
    state.gfx = graphics_create_context(800, 600);
    if (!state.gfx) {
        fprintf(stderr, "Failed to create graphics context\n");
        engine_window_destroy(window);
        input_shutdown();
        engine_shutdown();
        return 1;
    }
    
    printf("Input demo running!\n\n");
    
    f64 last_time = engine_get_time();
    i32 frame = 0;
    
    /* Main loop */
    while (!engine_window_should_close(window)) {
        /* Update input state at start of frame */
        input_update();
        
        /* Process events */
        engine_poll_events();
        
        /* Calculate delta time */
        f64 current_time = engine_get_time();
        f32 dt = (f32)(current_time - last_time);
        last_time = current_time;
        frame++;
        
        /* Exit on ESC */
        if (input_was_key_pressed(ENGINE_KEY_ESCAPE)) {
            break;
        }
        
        /* WASD / Arrow key movement */
        i32 speed = 300;  /* pixels per second */
        i32 move_amount = (i32)(speed * dt);
        
        if (input_is_key_down(ENGINE_KEY_W) || input_is_key_down(ENGINE_KEY_UP)) {
            state.player_y -= move_amount;
        }
        if (input_is_key_down(ENGINE_KEY_S) || input_is_key_down(ENGINE_KEY_DOWN)) {
            state.player_y += move_amount;
        }
        if (input_is_key_down(ENGINE_KEY_A) || input_is_key_down(ENGINE_KEY_LEFT)) {
            state.player_x -= move_amount;
        }
        if (input_is_key_down(ENGINE_KEY_D) || input_is_key_down(ENGINE_KEY_RIGHT)) {
            state.player_x += move_amount;
        }
        
        /* Space bar flash */
        if (input_was_key_pressed(ENGINE_KEY_SPACE)) {
            state.space_pressed = true;
            state.space_timer = 30;  /* 30 frames */
            printf("SPACE pressed!\n");
        }
        if (state.space_timer > 0) {
            state.space_timer--;
        } else {
            state.space_pressed = false;
        }
        
        /* Mouse click to spawn particles */
        if (input_was_mouse_button_pressed(ENGINE_MOUSE_BUTTON_LEFT)) {
            i32 mx, my;
            input_get_mouse_position(&mx, &my);
            
            if (state.particle_count < 32) {
                state.particles[state.particle_count].x = mx;
                state.particles[state.particle_count].y = my;
                state.particles[state.particle_count].lifetime = 60.0f;
                state.particles[state.particle_count].color = graphics_rgb(
                    255, 200, (u8)(frame % 256)
                );
                state.particle_count++;
                printf("Click at (%d, %d)\n", mx, my);
            }
        }
        
        /* Update particles */
        for (i32 i = 0; i < state.particle_count; i++) {
            state.particles[i].lifetime -= 60.0f * dt;
            if (state.particles[i].lifetime <= 0) {
                /* Remove particle */
                state.particles[i] = state.particles[state.particle_count - 1];
                state.particle_count--;
                i--;
            }
        }
        
        /* Get screen dimensions */
        i32 width = graphics_get_width(state.gfx);
        i32 height = graphics_get_height(state.gfx);
        
        /* Clamp player to screen */
        if (state.player_x < 0) state.player_x = 0;
        if (state.player_y < 0) state.player_y = 0;
        if (state.player_x + state.player_size > width) state.player_x = width - state.player_size;
        if (state.player_y + state.player_size > height) state.player_y = height - state.player_size;
        
        /* === RENDERING === */
        
        /* Clear with flash effect */
        graphics_color_t bg = state.space_pressed ? 
            graphics_rgb(80, 80, 100) : graphics_rgb(30, 30, 40);
        graphics_clear(state.gfx, bg);
        
        /* Draw grid */
        for (i32 x = 0; x < width; x += 50) {
            graphics_draw_line(state.gfx, x, 0, x, height, graphics_rgba(50, 50, 60, 100));
        }
        for (i32 y = 0; y < height; y += 50) {
            graphics_draw_line(state.gfx, 0, y, width, y, graphics_rgba(50, 50, 60, 100));
        }
        
        /* Draw particles */
        for (i32 i = 0; i < state.particle_count; i++) {
            i32 size = (i32)(state.particles[i].lifetime / 2);
            if (size > 20) size = 20;
            graphics_fill_circle(state.gfx, 
                state.particles[i].x, state.particles[i].y, 
                size, state.particles[i].color);
        }
        
        /* Draw mouse cursor indicator */
        i32 mx = input_get_mouse_x();
        i32 my = input_get_mouse_y();
        graphics_draw_circle(state.gfx, mx, my, 10, COLOR_WHITE);
        graphics_draw_line(state.gfx, mx - 15, my, mx + 15, my, COLOR_WHITE);
        graphics_draw_line(state.gfx, mx, my - 15, mx, my + 15, COLOR_WHITE);
        
        /* Draw player square */
        graphics_rect_t player_rect = graphics_rect(
            state.player_x, state.player_y, 
            state.player_size, state.player_size
        );
        graphics_fill_rect(state.gfx, &player_rect, state.player_color);
        graphics_draw_rect(state.gfx, &player_rect, COLOR_WHITE);
        
        /* Draw UI text */
        char text[256];
        snprintf(text, sizeof(text), "Input Demo - Use WASD or Arrows to move");
        graphics_draw_text(state.gfx, text, 10, 10, COLOR_WHITE, NULL);
        
        snprintf(text, sizeof(text), "Player: (%d, %d)", state.player_x, state.player_y);
        graphics_draw_text(state.gfx, text, 10, 30, COLOR_CYAN, NULL);
        
        snprintf(text, sizeof(text), "Mouse: (%d, %d)", mx, my);
        graphics_draw_text(state.gfx, text, 10, 50, COLOR_YELLOW, NULL);
        
        /* Mouse delta */
        i32 mdx, mdy;
        input_get_mouse_delta(&mdx, &mdy);
        snprintf(text, sizeof(text), "Mouse Delta: (%d, %d)", mdx, mdy);
        graphics_draw_text(state.gfx, text, 10, 70, COLOR_GRAY, NULL);
        
        /* Key states */
        bool w_down = input_is_key_down(ENGINE_KEY_W);
        bool a_down = input_is_key_down(ENGINE_KEY_A);
        bool s_down = input_is_key_down(ENGINE_KEY_S);
        bool d_down = input_is_key_down(ENGINE_KEY_D);
        bool space_down = input_is_key_down(ENGINE_KEY_SPACE);
        
        snprintf(text, sizeof(text), "Keys: W=%d A=%d S=%d D=%d SPACE=%d", 
                w_down, a_down, s_down, d_down, space_down);
        graphics_draw_text(state.gfx, text, 10, 90, COLOR_GREEN, NULL);
        
        snprintf(text, sizeof(text), "Particles: %d  Frame: %d", state.particle_count, frame);
        graphics_draw_text(state.gfx, text, 10, 110, COLOR_MAGENTA, NULL);
        
        /* Instructions at bottom */
        graphics_draw_text(state.gfx, "Press SPACE for flash, Click to spawn particles, ESC to exit", 
                         10, height - 20, COLOR_GRAY, NULL);
        
        /* Present to window */
        extern struct engine_window {
            void* platform_window;
        };
        struct engine_window* win_internal = (struct engine_window*)window;
        
        platform_window_present_buffer(
            (platform_window_t*)win_internal->platform_window,
            graphics_get_pixels(state.gfx),
            width, height
        );
        
        platform_sleep(16);  /* ~60 FPS */
    }
    
    /* Cleanup */
    printf("\nCleaning up...\n");
    graphics_destroy_context(state.gfx);
    engine_window_destroy(window);
    input_shutdown();
    engine_shutdown();
    
    printf("Input demo complete!\n");
    return 0;
}
