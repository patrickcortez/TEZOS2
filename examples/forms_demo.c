#include "../include/engine.h"
#include "../include/graphics.h"
#include "../include/ui.h"
#include "../include/input.h"
#include <stdio.h>
#include <string.h>

/* Demo state */
typedef struct {
    graphics_context_t* gfx;
    ui_context_t* ui;
    
    /* Login form */
    char username[64];
    char password[64];
    bool remember_me;
    bool logged_in;
    char status_message[256];
    
    /* Registration form */
    char reg_email[128];
    char reg_username[64];
    char reg_password[64];
    char reg_confirm[64];
    bool agree_terms;
    
    /* Search bar */
    char search_query[256];
    
    /* Chat/comments */
    char message[1024];
    char chat_history[2048];
    
    /* Shift key state */
    bool shift_down;
} demo_state_t;

void on_event(const engine_event_t* event, void* user_data) {
    demo_state_t* state = (demo_state_t*)user_data;
    
    input_process_event(event);
    
    /* Pass events to UI */
    if (event->type == ENGINE_EVENT_MOUSE_MOVE) {
        ui_input_mouse_move(state->ui, event->data.mouse_move.x, event->data.mouse_move.y);
    } else if (event->type == ENGINE_EVENT_MOUSE_BUTTON_PRESS) {
        ui_input_mouse_button(state->ui, true);
    } else if (event->type == ENGINE_EVENT_MOUSE_BUTTON_RELEASE) {
        ui_input_mouse_button(state->ui, false);
    } else if (event->type == ENGINE_EVENT_KEY_PRESS) {
        engine_key_t key = event->data.key.key;
        ui_input_key(state->ui, key, true);
        
        /* Track shift key */
        if (key == ENGINE_KEY_LEFT_SHIFT || key == ENGINE_KEY_RIGHT_SHIFT) {
            state->shift_down = true;
        }
        
        /* Convert ENGINE_KEY to character */
        char ch = 0;
        
        /* Letters A-Z */
        if (key >= ENGINE_KEY_A && key <= ENGINE_KEY_Z) {
            ch = 'a' + (key - ENGINE_KEY_A);
            /* Convert to uppercase if shift is down */
            if (state->shift_down) {
                ch = ch - 32;  /* 'a' - 32 = 'A' */
            }
        }
        /* Numbers 0-9 with shift symbols */
        else if (key >= ENGINE_KEY_0 && key <= ENGINE_KEY_9) {
            if (state->shift_down) {
                /* Shifted number symbols */
                const char shifted[] = ")!@#$%^&*(";
                ch = shifted[key - ENGINE_KEY_0];
            } else {
                ch = '0' + (key - ENGINE_KEY_0);
            }
        }
        /* Special keys */
        else if (key == ENGINE_KEY_SPACE) ch = ' ';
        else if (key == ENGINE_KEY_MINUS) ch = state->shift_down ? '_' : '-';
        else if (key == ENGINE_KEY_EQUALS) ch = state->shift_down ? '+' : '=';
        else if (key == ENGINE_KEY_LEFT_BRACKET) ch = state->shift_down ? '{' : '[';
        else if (key == ENGINE_KEY_RIGHT_BRACKET) ch = state->shift_down ? '}' : ']';
        else if (key == ENGINE_KEY_BACKSLASH) ch = state->shift_down ? '|' : '\\';
        else if (key == ENGINE_KEY_SEMICOLON) ch = state->shift_down ? ':' : ';';
        else if (key == ENGINE_KEY_APOSTROPHE) ch = state->shift_down ? '"' : '\'';
        else if (key == ENGINE_KEY_COMMA) ch = state->shift_down ? '<' : ',';
        else if (key == ENGINE_KEY_PERIOD) ch = state->shift_down ? '>' : '.';
        else if (key == ENGINE_KEY_SLASH) ch = state->shift_down ? '?' : '/';
        else if (key == ENGINE_KEY_BACKSPACE) ch = '\b';
        else if (key == ENGINE_KEY_ENTER) ch = '\n';
        
        if (ch != 0) {
            ui_input_char(state->ui, ch);
        }
    } else if (event->type == ENGINE_EVENT_KEY_RELEASE) {
        engine_key_t key = event->data.key.key;
        ui_input_key(state->ui, key, false);
        
        /* Track shift key release */
        if (key == ENGINE_KEY_LEFT_SHIFT || key == ENGINE_KEY_RIGHT_SHIFT) {
            state->shift_down = false;
        }
    } else if (event->type == ENGINE_EVENT_WINDOW_RESIZE) {
        if (state->gfx) {
            graphics_resize(state->gfx, event->data.resize.width, event->data.resize.height);
        }
    }
}

int main(void) {
    printf("=== Forms Demo - Login, Registration, Search, Chat ===\n");
    
    engine_config_t engine_config = {
        .app_name = "Forms Demo",
        .enable_logging = true,
    };
    
    if (engine_init(&engine_config) != ENGINE_SUCCESS) {
        fprintf(stderr, "Failed to initialize engine\n");
        return 1;
    }
    
    input_init();
    
    demo_state_t state = {0};
    strcpy(state.username, "admin");
    strcpy(state.status_message, "Enter credentials");
    strcpy(state.chat_history, "Welcome to the chat!\n");
    
    engine_window_config_t window_config = {
        .title = "Forms Demo - Login, Registration, Search & Chat",
        .width = 900,
        .height = 700,
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
    
    state.gfx = graphics_create_context(900, 700);
    state.ui = ui_create_context(state.gfx);
    
    if (!state.gfx || !state.ui) {
        fprintf(stderr, "Failed to create contexts\n");
        engine_window_destroy(window);
        input_shutdown();
        engine_shutdown();
        return 1;
    }
    
    printf("Forms demo running!\n\n");
    
    while (!engine_window_should_close(window)) {
        input_update();
        engine_poll_events();
        
        if (input_was_key_pressed(ENGINE_KEY_ESCAPE)) {break;
        }
        
        i32 width = graphics_get_width(state.gfx);
        i32 height = graphics_get_height(state.gfx);
        
        graphics_clear(state.gfx, graphics_rgb(25, 25, 30));
        ui_begin_frame(state.ui);
        
        /* === LOGIN FORM === */
        if (!state.logged_in) {
            if (ui_begin_window(state.ui, "Login", 50, 50, 350, 300)) {
                ui_label(state.ui, "Username:");
                ui_text_input_ex(state.ui, "login_username", state.username, 64, 0, "Enter username");
                
                ui_label(state.ui, "Password:");
                ui_text_input_ex(state.ui, "login_password", state.password, 64, UI_TEXT_INPUT_PASSWORD, "••••••••");
                
                ui_checkbox(state.ui, "Remember me", &state.remember_me);
                
                if (ui_button(state.ui, "Login") || ui_text_input_submitted(state.ui)) {
                    if (strcmp(state.username, "admin") == 0 && strcmp(state.password, "1234") == 0) {
                        state.logged_in = true;
                        strcpy(state.status_message, "Login successful!");
                        printf("LOGIN SUCCESS\n");
                    } else {
                        strcpy(state.status_message, "Invalid credentials!");
                    }
                }
                
                ui_label(state.ui, state.status_message);
                ui_end_window(state.ui);
            }
        }
        
        /* === REGISTRATION FORM === */
        if (ui_begin_window(state.ui, "Registration", 450, 50, 400, 400)) {
            ui_label(state.ui, "Email:");
            ui_text_input_ex(state.ui, "reg_email", state.reg_email, 128, 0, "user@example.com");
            
            ui_label(state.ui, "Username:");
            ui_text_input_ex(state.ui, "reg_username", state.reg_username, 64, 0, "Choose username");
            
            ui_label(state.ui, "Password:");
            ui_text_input_ex(state.ui, "reg_password", state.reg_password, 64, UI_TEXT_INPUT_PASSWORD, "••••••••");
            
            ui_label(state.ui, "Confirm:");
            ui_text_input_ex(state.ui, "reg_confirm", state.reg_confirm, 64, UI_TEXT_INPUT_PASSWORD, "••••••••");
            
            ui_checkbox(state.ui, "I agree to terms", &state.agree_terms);
            
            if (ui_button(state.ui, "Register") && state.agree_terms) {
                if (strcmp(state.reg_password, state.reg_confirm) == 0) {
                    printf("REGISTRATION: %s (%s)\n", state.reg_username, state.reg_email);
                }
            }
            
            ui_end_window(state.ui);
        }
        
        /* === SEARCH BAR === */
        if (ui_begin_window(state.ui, "Search", 50, 380, 350, 150)) {
            ui_label(state.ui, "Search:");
            ui_text_input_ex(state.ui, "search_query", state.search_query, 256, 0, "Type to search...");
            
            if (ui_button(state.ui, "Search") || ui_text_input_submitted(state.ui)) {
                if (strlen(state.search_query) > 0) {
                    printf("SEARCH: %s\n", state.search_query);
                }
            }
            
            ui_end_window(state.ui);
        }
        
        /* === CHAT/COMMENT BOX === */
        if (ui_begin_window(state.ui, "Chat", 450, 470, 400, 200)) {
            ui_text_area(state.ui, "chat_message", state.message, 1024, 3);
            
            if (ui_button(state.ui, "Send")) {
                if (strlen(state.message) > 0) {
                    printf("SENT: %s\n", state.message);
                    strcat(state.chat_history, state.message);
                    strcat(state.chat_history, "\n");
                    state.message[0] = '\0';
                }
            }
            
            ui_end_window(state.ui);
        }
        
        ui_end_frame(state.ui);
        
        /* Present */
        extern struct engine_window { void* platform_window; };
        struct engine_window* win_internal = (struct engine_window*)window;
        platform_window_present_buffer((platform_window_t*)win_internal->platform_window,
                                     graphics_get_pixels(state.gfx), width, height);
        
        platform_sleep(16);
    }
    
    printf("\nCleaning up...\n");
    ui_destroy_context(state.ui);
    graphics_destroy_context(state.gfx);
    engine_window_destroy(window);
    input_shutdown();
    engine_shutdown();
    
    printf("Forms demo complete!\n");
    return 0;
}
