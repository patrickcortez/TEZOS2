#include "../include/engine.h"
#include "../include/ui.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_FILES 100
#define MAX_PATH_LEN 256

typedef struct {
    char name[MAX_PATH_LEN];
    bool is_dir;
} file_entry_t;

typedef struct {
    ui_context_t* ui;
    file_entry_t files[MAX_FILES];
    int file_count;
    int selected_index;
    char current_path[MAX_PATH_LEN];
} app_state_t;

/* Helper to scan directory */
void scan_directory(app_state_t* state, const char* path) {
    DIR* dir;
    struct dirent* ent;
    
    state->file_count = 0;
    state->selected_index = -1;
    strncpy(state->current_path, path, MAX_PATH_LEN - 1);
    
    dir = opendir(path);
    if (dir != NULL) {
        while ((ent = readdir(dir)) != NULL && state->file_count < MAX_FILES) {
            /* Skip . and .. for simplicity in this demo, or keep them for navigation */
            if (strcmp(ent->d_name, ".") == 0) continue;
            
            strncpy(state->files[state->file_count].name, ent->d_name, MAX_PATH_LEN - 1);
            
            /* Check if directory */
            struct stat statbuf;
            char full_path[MAX_PATH_LEN * 2];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, ent->d_name);
            
            if (stat(full_path, &statbuf) == 0) {
                state->files[state->file_count].is_dir = S_ISDIR(statbuf.st_mode);
            } else {
                state->files[state->file_count].is_dir = false;
            }
            
            state->file_count++;
        }
        closedir(dir);
    }
}

/* Event callback */
void on_event(const engine_event_t* event, void* user_data) {
    app_state_t* state = (app_state_t*)user_data;
    
    input_process_event(event);
    
    if (state && state->ui) {
        if (event->type == ENGINE_EVENT_MOUSE_MOVE) {
            ui_input_mouse_move(state->ui, event->data.mouse_move.x, event->data.mouse_move.y);
        } else if (event->type == ENGINE_EVENT_MOUSE_BUTTON_PRESS) {
            ui_input_mouse_button(state->ui, true);
        } else if (event->type == ENGINE_EVENT_MOUSE_BUTTON_RELEASE) {
            ui_input_mouse_button(state->ui, false);
        } else if (event->type == ENGINE_EVENT_KEY_PRESS) {
            /* Map key codes to characters for text input if needed */
        }
    }
}

int main(void) {
    /* Initialize engine */
    if (engine_init(NULL) != ENGINE_SUCCESS) {
        fprintf(stderr, "Failed to initialize engine\n");
        return 1;
    }
    
    /* Create window */
    engine_window_config_t window_config = {
        .title = "File List Demo",
        .width = 800,
        .height = 600,
        .resizable = false,
        .event_callback = on_event,
        .user_data = NULL
    };
    
    engine_window_t* window;
    if (engine_window_create(&window_config, &window) != ENGINE_SUCCESS) {
        fprintf(stderr, "Failed to create window\n");
        return 1;
    }
    
    /* Initialize graphics */
    graphics_context_t* gfx = graphics_create_context(800, 600);
    if (!gfx) {
        fprintf(stderr, "Failed to initialize graphics\n");
        return 1;
    }
    
    /* Initialize UI */
    ui_context_t* ui = ui_create_context(gfx);
    
    /* App state */
    app_state_t state;
    memset(&state, 0, sizeof(state));
    state.ui = ui;
    
    /* Set user data for callback */
    /* Note: In a real engine, we'd need a way to update the user_data pointer in the window */
    /* For now, we rely on the fact that we passed NULL initially and can't easily change it without an API */
    /* So we'll just use a global or assume the callback can access state if we passed it. */
    /* Wait, engine_window_create took user_data. We passed NULL. We need to pass &state. */
    /* But we created state AFTER window. Let's recreate window or move state up. */
    
    /* Re-create window with user data */
    engine_window_destroy(window);
    window_config.user_data = &state;
    if (engine_window_create(&window_config, &window) != ENGINE_SUCCESS) {
        return 1;
    }
    
    /* Scan current directory */
    scan_directory(&state, ".");
    
    while (!engine_window_should_close(window)) {
        input_update();
        engine_poll_events();
        
        if (input_was_key_pressed(ENGINE_KEY_ESCAPE)) {
            break;
        }
        
        /* Render */
        graphics_clear(gfx, graphics_rgb(40, 44, 52));
        
        ui_begin_frame(ui);
        
        if (ui_begin_window(ui, "File Explorer", 50, 50, 400, 500)) {
            ui_label(ui, "Current Directory:");
            ui_label(ui, state.current_path);
            ui_separator(ui);
            
            /* List files */
            for (int i = 0; i < state.file_count; i++) {
                char label[300];
                if (state.files[i].is_dir) {
                    snprintf(label, sizeof(label), "[DIR] %s", state.files[i].name);
                } else {
                    snprintf(label, sizeof(label), "      %s", state.files[i].name);
                }
                
                if (ui_list_item(ui, label, state.selected_index == i)) {
                    state.selected_index = i;
                    printf("Selected: %s\n", state.files[i].name);
                    
                    /* If directory, could navigate into it (not implemented for simplicity) */
                }
            }
        }
        
        /* Info Panel */
        if (ui_begin_window(ui, "Selected Item Info", 470, 50, 300, 200)) {
            if (state.selected_index >= 0 && state.selected_index < state.file_count) {
                ui_label(ui, "Name:");
                ui_label(ui, state.files[state.selected_index].name);
                
                ui_label(ui, "Type:");
                if (state.files[state.selected_index].is_dir) {
                    ui_label(ui, "Directory");
                } else {
                    ui_label(ui, "File");
                }
            } else {
                ui_label(ui, "No item selected");
            }
        }
        
        ui_end_frame(ui);
        
        /* Present buffer */
        /* We need to access the internal platform window to present */
        /* This is a hack because engine_window_t is opaque in the public API */
        struct {
            void* platform_window;
        }* win_internal = (void*)window;
        
        platform_window_present_buffer(
            (platform_window_t*)win_internal->platform_window,
            graphics_get_pixels(gfx),
            800,
            600
        );
        
        /* Small sleep to limit FPS */
        platform_sleep(16);
    }
    
    ui_destroy_context(ui);
    graphics_destroy_context(gfx);
    engine_window_destroy(window);
    engine_shutdown();
    
    return 0;
}
