#include "../include/engine.h"
#include "../include/graphics.h"
#include "../include/ui.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Demo application state */
typedef struct {
    graphics_context_t* gfx;
    ui_context_t* ui;
    
    /* Widget states */
    bool checkbox1;
    bool checkbox2;
    bool checkbox3;
    int radio_option;
    int slider_int_value;
    f32 slider_float_value;
    f32 progress;
    char text_buffer[128];
    
    /* Animation */
    float time;
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
            ui_input_mouse_move(state->ui, event->data.mouse_move.x, event->data.mouse_move.y);
            break;
            
        case ENGINE_EVENT_MOUSE_BUTTON_PRESS:
            ui_input_mouse_button(state->ui, true);
            break;
            
        case ENGINE_EVENT_MOUSE_BUTTON_RELEASE:
            ui_input_mouse_button(state->ui, false);
            break;
            
        case ENGINE_EVENT_MOUSE_WHEEL:
            ui_input_mouse_wheel(state->ui, event->data.mouse_wheel.delta);
            break;
            
        case ENGINE_EVENT_KEY_PRESS:
            /* Handle text input */
            if (event->data.key.key >= ENGINE_KEY_A && event->data.key.key <= ENGINE_KEY_Z) {
                char c = 'a' + (event->data.key.key - ENGINE_KEY_A);
                ui_input_char(state->ui, c);
            } else if (event->data.key.key == ENGINE_KEY_SPACE) {
                ui_input_char(state->ui, ' ');
            } else if (event->data.key.key == ENGINE_KEY_BACKSPACE) {
                ui_input_char(state->ui, '\b');
            }
            break;
            
        default:
            break;
    }
}

int main(void) {
    printf("=== UI Demo Application ===\n");
    
    /* Initialize engine */
    engine_config_t engine_config = {
        .app_name = "UI Demo",
        .enable_logging = true,
    };
    
    if (engine_init(&engine_config) != ENGINE_SUCCESS) {
        fprintf(stderr, "Failed to initialize engine\n");
        return 1;
    }
    
    /* Initialize demo state */
    demo_state_t state = {0};
    state.checkbox1 = true;
    state.checkbox2 = false;
    state.checkbox3 = false;
    state.radio_option = 0;
    state.slider_int_value = 50;
    state.slider_float_value = 0.5f;
    state.progress = 0.0f;
    strcpy(state.text_buffer, "Hello UI!");
    
    /* Create window */
    engine_window_config_t window_config = {
        .title = "UI Demo - 2D Engine",
        .width = 1024,
        .height = 768,
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
    state.gfx = graphics_create_context(1024, 768);
    if (!state.gfx) {
        fprintf(stderr, "Failed to create graphics context\n");
        engine_window_destroy(window);
        engine_shutdown();
        return 1;
    }
    
    /* Create UI context */
    state.ui = ui_create_context(state.gfx);
    if (!state.ui) {
        fprintf(stderr, "Failed to create UI context\n");
        graphics_destroy_context(state.gfx);
        engine_window_destroy(window);
        engine_shutdown();
        return 1;
    }
    
    printf("UI Demo initialized successfully!\n");
    printf("Try interacting with the widgets in the window.\n\n");
    
    /* Main loop */
    while (!engine_window_should_close(window)) {
        engine_poll_events();
        
        state.time = (float)engine_get_time();
        
        /* Animate progress bar */
        state.progress = (sinf(state.time) + 1.0f) * 0.5f;
        
        /* Clear */
        graphics_clear(state.gfx, graphics_rgb(30, 30, 35));
        
        /* Begin UI frame */
        ui_begin_frame(state.ui);
        
        /* Menu Bar */
        if (ui_begin_menu_bar(state.ui)) {
            if (ui_begin_menu(state.ui, "File")) {
                if (ui_menu_item(state.ui, "New")) printf("Clicked New\n");
                if (ui_menu_item(state.ui, "Open")) printf("Clicked Open\n");
                if (ui_menu_item(state.ui, "Save")) printf("Clicked Save\n");
                ui_end_menu(state.ui);
            }
            
            if (ui_begin_menu(state.ui, "Edit")) {
                if (ui_menu_item(state.ui, "Cut")) printf("Clicked Cut\n");
                if (ui_menu_item(state.ui, "Copy")) printf("Clicked Copy\n");
                if (ui_menu_item(state.ui, "Paste")) printf("Clicked Paste\n");
                ui_end_menu(state.ui);
            }
            
            if (ui_begin_menu(state.ui, "View")) {
                if (ui_menu_item(state.ui, "Zoom In")) printf("Clicked Zoom In\n");
                if (ui_menu_item(state.ui, "Zoom Out")) printf("Clicked Zoom Out\n");
                ui_end_menu(state.ui);
            }
            
            ui_end_menu_bar(state.ui);
        }
        
        /* Main window */
        if (ui_begin_window(state.ui, "UI Demo Application", 20, 60, 980, 680)) {
            
            /* Title */
            ui_label_ex(state.ui, "2D Engine - UI Widget Showcase", UI_ALIGN_CENTER);
            ui_separator(state.ui);
            ui_spacing(state.ui, 10);
            
            /* Dropdown */
            ui_label(state.ui, "Dropdown:");
            ui_spacing(state.ui, 5);
            const char* options[] = { "Dark Theme", "Light Theme", "Blue Theme", "Red Theme" };
            static int selected_theme = 0;
            if (ui_dropdown(state.ui, "ThemeSelect", options, 4, &selected_theme)) {
                printf("Theme changed to: %s\n", options[selected_theme]);
            }
            
            ui_spacing(state.ui, 10);
            ui_separator(state.ui);
            
            /* Buttons section */
            ui_label(state.ui, "Buttons:");
            ui_spacing(state.ui, 5);
            
            if (ui_button(state.ui, "Click Me!")) {
                printf("Button clicked!\n");
            }
            ui_same_line(state.ui);
            
            if (ui_button(state.ui, "Another Button")) {
                printf("Another button clicked!\n");
            }
            ui_same_line(state.ui);
            
            if (ui_button_ex(state.ui, "Wide Button", 250, 24)) {
                printf("Wide button clicked!\n");
            }
            
            ui_spacing(state.ui, 10);
            ui_separator(state.ui);
            
            /* Checkboxes */
            ui_label(state.ui, "Checkboxes:");
            ui_spacing(state.ui, 5);
            
            if (ui_checkbox(state.ui, "Enable feature A", &state.checkbox1)) {
                printf("Checkbox 1: %s\n", state.checkbox1 ? "ON" : "OFF");
            }
            
            if (ui_checkbox(state.ui, "Enable feature B", &state.checkbox2)) {
                printf("Checkbox 2: %s\n", state.checkbox2 ? "ON" : "OFF");
            }
            
            if (ui_checkbox(state.ui, "Enable feature C", &state.checkbox3)) {
                printf("Checkbox 3: %s\n", state.checkbox3 ? "ON" : "OFF");
            }
            
            ui_spacing(state.ui, 10);
            ui_separator(state.ui);
            
            /* Radio buttons */
            ui_label(state.ui, "Radio Buttons (Select one):");
            ui_spacing(state.ui, 5);
            
            if (ui_radio(state.ui, "Option 1", &state.radio_option, 0)) {
                printf("Selected option: 0\n");
            }
            if (ui_radio(state.ui, "Option 2", &state.radio_option, 1)) {
                printf("Selected option: 1\n");
            }
            if (ui_radio(state.ui, "Option 3", &state.radio_option, 2)) {
                printf("Selected option: 2\n");
            }
            
            ui_spacing(state.ui, 10);
            ui_separator(state.ui);
            
            /* Sliders */
            ui_label(state.ui, "Sliders:");
            ui_spacing(state.ui, 5);
            
            if (ui_slider_int(state.ui, "Integer", &state.slider_int_value, 0, 100)) {
                printf("Int slider value: %d\n", state.slider_int_value);
            }
            
            if (ui_slider_float(state.ui, "Float", &state.slider_float_value, 0.0f, 1.0f)) {
                printf("Float slider value: %.2f\n", state.slider_float_value);
            }
            
            ui_spacing(state.ui, 10);
            ui_separator(state.ui);
            
            /* Text input */
            ui_label(state.ui, "Text Input:");
            ui_spacing(state.ui, 5);
            
            if (ui_text_input(state.ui, "Name", state.text_buffer, sizeof(state.text_buffer))) {
                printf("Text input changed: %s\n", state.text_buffer);
            }
            
            ui_spacing(state.ui, 10);
            ui_separator(state.ui);
            
            /* Progress bar */
            ui_label(state.ui, "Progress Bar (Animated):");
            ui_spacing(state.ui, 5);
            ui_progress_bar(state.ui, state.progress);
            
            ui_spacing(state.ui, 10);
            ui_separator(state.ui);
            
            /* Info */
            char info[256];
            snprintf(info, sizeof(info), "Time: %.2fs  |  FPS: ~60", state.time);
            ui_label_ex(state.ui, info, UI_ALIGN_CENTER);
            
            ui_end_window(state.ui);
        }
        
        /* End UI frame */
        ui_end_frame(state.ui);
        
        /* Present */
        extern struct engine_window {
            void* platform_window;
        };
        struct engine_window* win_internal = (struct engine_window*)window;
        
        platform_window_present_buffer(
            (platform_window_t*)win_internal->platform_window,
            graphics_get_pixels(state.gfx),
            graphics_get_width(state.gfx),
            graphics_get_height(state.gfx)
        );
        
        platform_sleep(16); /* ~60 FPS */
    }
    
    /* Cleanup */
    printf("\nCleaning up...\n");
    ui_destroy_context(state.ui);
    graphics_destroy_context(state.gfx);
    engine_window_destroy(window);
    engine_shutdown();
    
    printf("UI Demo complete!\n");
    return 0;
}
