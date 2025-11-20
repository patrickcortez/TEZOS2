#include "../include/input.h"
#include <string.h>

#define MAX_KEYS 256
#define MAX_MOUSE_BUTTONS 8

/* Input state structure */
typedef struct {
    /* Keyboard state */
    bool keys_down[MAX_KEYS];
    bool keys_down_prev[MAX_KEYS];
    
    /* Mouse state */
    i32 mouse_x, mouse_y;
    i32 mouse_prev_x, mouse_prev_y;
    bool mouse_buttons_down[MAX_MOUSE_BUTTONS];
    bool mouse_buttons_down_prev[MAX_MOUSE_BUTTONS];
    
    /* Initialization flag */
    bool initialized;
} input_state_t;

/* Global input state */
static input_state_t g_input = {0};

/* Initialization */
void input_init(void) {
    memset(&g_input, 0, sizeof(input_state_t));
    g_input.initialized = true;
    ENGINE_LOG_INFO("Input system initialized");
}

void input_shutdown(void) {
    g_input.initialized = false;
    ENGINE_LOG_INFO("Input system shut down");
}

/* Frame update - copy current state to previous */
void input_update(void) {
    if (!g_input.initialized) return;
    
    /* Copy keyboard state */
    memcpy(g_input.keys_down_prev, g_input.keys_down, sizeof(g_input.keys_down));
    
    /* Copy mouse button state */
    memcpy(g_input.mouse_buttons_down_prev, g_input.mouse_buttons_down, 
           sizeof(g_input.mouse_buttons_down));
    
    /* Copy mouse position */
    g_input.mouse_prev_x = g_input.mouse_x;
    g_input.mouse_prev_y = g_input.mouse_y;
}

/* Process engine events */
void input_process_event(const engine_event_t* event) {
    if (!g_input.initialized || !event) return;
    
    switch (event->type) {
        case ENGINE_EVENT_KEY_PRESS:
            if (event->data.key.key < MAX_KEYS) {
                g_input.keys_down[event->data.key.key] = true;
            }
            break;
            
        case ENGINE_EVENT_KEY_RELEASE:
            if (event->data.key.key < MAX_KEYS) {
                g_input.keys_down[event->data.key.key] = false;
            }
            break;
            
        case ENGINE_EVENT_MOUSE_MOVE:
            g_input.mouse_x = event->data.mouse_move.x;
            g_input.mouse_y = event->data.mouse_move.y;
            break;
            
        case ENGINE_EVENT_MOUSE_BUTTON_PRESS:
            if (event->data.mouse_button.button < MAX_MOUSE_BUTTONS) {
                g_input.mouse_buttons_down[event->data.mouse_button.button] = true;
            }
            break;
            
        case ENGINE_EVENT_MOUSE_BUTTON_RELEASE:
            if (event->data.mouse_button.button < MAX_MOUSE_BUTTONS) {
                g_input.mouse_buttons_down[event->data.mouse_button.button] = false;
            }
            break;
            
        default:
            break;
    }
}

/* Keyboard queries */
bool input_is_key_down(engine_key_t key) {
    if (!g_input.initialized || key >= MAX_KEYS) return false;
    return g_input.keys_down[key];
}

bool input_was_key_pressed(engine_key_t key) {
    if (!g_input.initialized || key >= MAX_KEYS) return false;
    return g_input.keys_down[key] && !g_input.keys_down_prev[key];
}

bool input_was_key_released(engine_key_t key) {
    if (!g_input.initialized || key >= MAX_KEYS) return false;
    return !g_input.keys_down[key] && g_input.keys_down_prev[key];
}

/* Mouse button queries */
bool input_is_mouse_button_down(engine_mouse_button_t button) {
    if (!g_input.initialized || button >= MAX_MOUSE_BUTTONS) return false;
    return g_input.mouse_buttons_down[button];
}

bool input_was_mouse_button_pressed(engine_mouse_button_t button) {
    if (!g_input.initialized || button >= MAX_MOUSE_BUTTONS) return false;
    return g_input.mouse_buttons_down[button] && !g_input.mouse_buttons_down_prev[button];
}

bool input_was_mouse_button_released(engine_mouse_button_t button) {
    if (!g_input.initialized || button >= MAX_MOUSE_BUTTONS) return false;
    return !g_input.mouse_buttons_down[button] && g_input.mouse_buttons_down_prev[button];
}

/* Mouse position */
void input_get_mouse_position(i32* out_x, i32* out_y) {
    if (out_x) *out_x = g_input.mouse_x;
    if (out_y) *out_y = g_input.mouse_y;
}

void input_get_mouse_delta(i32* out_dx, i32* out_dy) {
    if (out_dx) *out_dx = g_input.mouse_x - g_input.mouse_prev_x;
    if (out_dy) *out_dy = g_input.mouse_y - g_input.mouse_prev_y;
}

i32 input_get_mouse_x(void) {
    return g_input.mouse_x;
}

i32 input_get_mouse_y(void) {
    return g_input.mouse_y;
}

/* Convenience helpers */
bool input_is_key_down_any(engine_key_t* keys, i32 count) {
    if (!g_input.initialized || !keys) return false;
    
    for (i32 i = 0; i < count; i++) {
        if (input_is_key_down(keys[i])) {
            return true;
        }
    }
    return false;
}

void input_reset(void) {
    if (!g_input.initialized) return;
    
    memset(g_input.keys_down, 0, sizeof(g_input.keys_down));
    memset(g_input.keys_down_prev, 0, sizeof(g_input.keys_down_prev));
    memset(g_input.mouse_buttons_down, 0, sizeof(g_input.mouse_buttons_down));
    memset(g_input.mouse_buttons_down_prev, 0, sizeof(g_input.mouse_buttons_down_prev));
    
    g_input.mouse_x = 0;
    g_input.mouse_y = 0;
    g_input.mouse_prev_x = 0;
    g_input.mouse_prev_y = 0;
}
