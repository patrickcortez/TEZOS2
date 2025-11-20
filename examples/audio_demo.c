#include "../include/engine.h"
#include "../include/audio.h"
#include "../include/input.h"
#include "../include/graphics.h"
#include "../include/ui.h"
#include "../include/engine.h"
#include "../include/dialogs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Application state to pass to event handler */
typedef struct {
    ui_context_t* ui;
} app_state_t;

/* Event callback */
void on_event(const engine_event_t* event, void* user_data) {
    app_state_t* state = (app_state_t*)user_data;
    
    /* Update global input state */
    input_process_event(event);
    
    /* Pass events to UI */
    if (state && state->ui) {
        if (event->type == ENGINE_EVENT_MOUSE_MOVE) {
            ui_input_mouse_move(state->ui, event->data.mouse_move.x, event->data.mouse_move.y);
        } else if (event->type == ENGINE_EVENT_MOUSE_BUTTON_PRESS) {
            ui_input_mouse_button(state->ui, true);
        } else if (event->type == ENGINE_EVENT_MOUSE_BUTTON_RELEASE) {
            ui_input_mouse_button(state->ui, false);
        } else if (event->type == ENGINE_EVENT_KEY_PRESS) {
            ui_input_key(state->ui, event->data.key.key, true);
            
            /* Handle basic character input for UI (if needed for text fields later) */
            /* For now just simple mapping for demo purposes */
            char ch = 0;
            if (event->data.key.key >= ENGINE_KEY_A && event->data.key.key <= ENGINE_KEY_Z) {
                ch = 'a' + (event->data.key.key - ENGINE_KEY_A);
            } else if (event->data.key.key >= ENGINE_KEY_0 && event->data.key.key <= ENGINE_KEY_9) {
                ch = '0' + (event->data.key.key - ENGINE_KEY_0);
            } else if (event->data.key.key == ENGINE_KEY_SPACE) ch = ' ';
            else if (event->data.key.key == ENGINE_KEY_BACKSPACE) ch = '\b';
            else if (event->data.key.key == ENGINE_KEY_ENTER) ch = '\n';
            
            if (ch != 0) ui_input_char(state->ui, ch);
            
        } else if (event->type == ENGINE_EVENT_KEY_RELEASE) {
            ui_input_key(state->ui, event->data.key.key, false);
        }
    }
}

int main(void) {
    printf("=== Audio System Demo ===\n");
    printf("This demo tests the audio system with multiple sounds and music.\n\n");
    
    engine_config_t engine_config = {
        .app_name = "Audio Demo",
        .enable_logging = true,
    };
    
    if (engine_init(&engine_config) != ENGINE_SUCCESS) {
        fprintf(stderr, "Failed to initialize engine\n");
        return 1;
    }
    
    input_init();
    
    /* Initialize audio */
    printf("Initializing audio system...\n");
    if (audio_init() != ENGINE_SUCCESS) {
        fprintf(stderr, "Failed to initialize audio\n");
        input_shutdown();
        engine_shutdown();
        return 1;
    }
    
    /* Create app state */
    app_state_t app_state = {0};
    
    engine_window_config_t window_config = {
        .title = "Audio Demo - File Dialogs",
        .width = 800,
        .height = 500,
        .resizable = false,
        .event_callback = on_event,
        .user_data = &app_state,  /* Pass state to callback */
    };
    
    engine_window_t* window = NULL;
    if (engine_window_create(&window_config, &window) != ENGINE_SUCCESS) {
        fprintf(stderr, "Failed to create window\n");
        audio_shutdown();
        input_shutdown();
        engine_shutdown();
        return 1;
    }
    
    graphics_context_t* gfx = graphics_create_context(800, 500);
    ui_context_t* ui = ui_create_context(gfx);
    
    if (!gfx || !ui) {
        fprintf(stderr, "Failed to create contexts\n");
        engine_window_destroy(window);
        audio_shutdown();
        input_shutdown();
        engine_shutdown();
        return 1;
    }
    
    /* Update state with UI context */
    app_state.ui = ui;
    
    /* Audio state */
    audio_sound_t* sound_effect = NULL;
    audio_sound_t* music_track = NULL;
    char sfx_path[256] = "None";
    char music_path[256] = "None";
    bool music_playing = false;
    f32 volume = 1.0f;
    
    /* File filters */
    file_filter_t audio_filters[] = {
        { "Audio Files", "*.wav;*.mp3;*.ogg;*.flac" }
    };
    
    int frame_counter = 0;
    
    while (!engine_window_should_close(window)) {
        input_update();
        engine_poll_events();
        
        frame_counter++;
        if (frame_counter < 10) {
            /* Skip initial frames to avoid phantom clicks */
            graphics_clear(gfx, graphics_rgb(30, 30, 35));
            /* engine_window_swap_buffers(window); - Not exposed in API, just clear is enough or wait */
            continue;
        }
        
        if (input_was_key_pressed(ENGINE_KEY_ESCAPE)) {
            break;
        }
        
        /* Render UI */
        graphics_clear(gfx, graphics_rgb(30, 30, 35));
        ui_begin_frame(ui);
        
        if (ui_begin_window(ui, "Audio Control Panel", 50, 50, 700, 400)) {
            ui_label(ui, "Sound Effect Channel");
            
            char sfx_label[300];
            snprintf(sfx_label, sizeof(sfx_label), "Current File: %s", sfx_path);
            ui_label(ui, sfx_label);
            
            if (ui_button(ui, "Load SFX...")) {
                char* path = dialog_open_file("Load Sound Effect", NULL, audio_filters, 1);
                if (path) {
                    if (sound_effect) audio_destroy_sound(sound_effect);
                    sound_effect = audio_load_sound(path);
                    strncpy(sfx_path, path, 255);
                    free(path);
                }
            }
            
            if (ui_button(ui, "Play SFX")) {
                if (sound_effect) {
                    audio_play(sound_effect, false);
                } else {
                    dialog_message("Error", "No sound effect loaded!");
                }
            }
            
            ui_separator(ui);
            
            ui_label(ui, "Music Channel");
            
            char music_label[300];
            snprintf(music_label, sizeof(music_label), "Current File: %s", music_path);
            ui_label(ui, music_label);
            
            if (ui_button(ui, "Load Music...")) {
                char* path = dialog_open_file("Load Music Track", NULL, audio_filters, 1);
                if (path) {
                    if (music_track) {
                        audio_stop(music_track);
                        audio_destroy_sound(music_track);
                        music_playing = false;
                    }
                    
                    music_track = audio_load_sound(path);
                    
                    strncpy(music_path, path, 255);
                    free(path);
                }
            }
            
            if (ui_button(ui, music_playing ? "Stop Music" : "Play Music (Loop)")) {
                if (music_track) {
                    if (music_playing) {
                        audio_stop(music_track);
                        music_playing = false;
                    } else {
                        audio_play(music_track, true);
                        music_playing = true;
                    }
                } else {
                    dialog_message("Error", "No music track loaded!");
                }
            }
            
            ui_separator(ui);
            
            char vol_text[64];
            snprintf(vol_text, sizeof(vol_text), "Master Volume: %.0f%%", volume * 100.0f);
            ui_label(ui, vol_text);
            
            if (ui_button(ui, "Volume -")) {
                volume -= 0.1f;
                if (volume < 0.0f) volume = 0.0f;
                audio_set_master_volume(volume);
            }
            
            if (ui_button(ui, "Volume +")) {
                volume += 0.1f;
                if (volume > 1.0f) volume = 1.0f;
                audio_set_master_volume(volume);
            }
            
            ui_end_window(ui);
        }
        
        ui_end_frame(ui);
        
        /* Present */
        extern struct engine_window { void* platform_window; };
        struct engine_window* win_internal = (struct engine_window*)window;
        platform_window_present_buffer((platform_window_t*)win_internal->platform_window,
                                     graphics_get_pixels(gfx), 800, 500);
        
        platform_sleep(16);
    }
    
    printf("\nCleaning up...\n");
    
    if (sound_effect) audio_destroy_sound(sound_effect);
    if (music_track) audio_destroy_sound(music_track);
    
    ui_destroy_context(ui);
    graphics_destroy_context(gfx);
    engine_window_destroy(window);
    audio_shutdown();
    input_shutdown();
    engine_shutdown();
    
    printf("Audio demo complete!\n");
    return 0;
}
