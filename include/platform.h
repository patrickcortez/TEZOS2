#ifndef ENGINE_PLATFORM_H
#define ENGINE_PLATFORM_H

#include "types.h"

/* Forward declarations */
typedef struct platform_window platform_window_t;
typedef struct platform_state platform_state_t;

/* Event types */
typedef enum {
    ENGINE_EVENT_NONE = 0,
    ENGINE_EVENT_WINDOW_CLOSE,
    ENGINE_EVENT_WINDOW_RESIZE,
    ENGINE_EVENT_WINDOW_FOCUS,
    ENGINE_EVENT_WINDOW_UNFOCUS,
    ENGINE_EVENT_KEY_PRESS,
    ENGINE_EVENT_KEY_RELEASE,
    ENGINE_EVENT_MOUSE_MOVE,
    ENGINE_EVENT_MOUSE_BUTTON_PRESS,
    ENGINE_EVENT_MOUSE_BUTTON_RELEASE,
    ENGINE_EVENT_MOUSE_WHEEL,
} engine_event_type_t;

/* Key codes (platform-independent) */
typedef enum {
    ENGINE_KEY_UNKNOWN = 0,
    
    /* Printable keys */
    ENGINE_KEY_SPACE = 32,
    
    /* Numbers */
    ENGINE_KEY_0 = 48, ENGINE_KEY_1, ENGINE_KEY_2, ENGINE_KEY_3, ENGINE_KEY_4,
    ENGINE_KEY_5, ENGINE_KEY_6, ENGINE_KEY_7, ENGINE_KEY_8, ENGINE_KEY_9,
    
    /* Letters */
    ENGINE_KEY_A = 65, ENGINE_KEY_B, ENGINE_KEY_C, ENGINE_KEY_D, ENGINE_KEY_E,
    ENGINE_KEY_F, ENGINE_KEY_G, ENGINE_KEY_H, ENGINE_KEY_I, ENGINE_KEY_J,
    ENGINE_KEY_K, ENGINE_KEY_L, ENGINE_KEY_M, ENGINE_KEY_N, ENGINE_KEY_O,
    ENGINE_KEY_P, ENGINE_KEY_Q, ENGINE_KEY_R, ENGINE_KEY_S, ENGINE_KEY_T,
    ENGINE_KEY_U, ENGINE_KEY_V, ENGINE_KEY_W, ENGINE_KEY_X, ENGINE_KEY_Y,
    ENGINE_KEY_Z,
    
    /* Symbols */
    ENGINE_KEY_APOSTROPHE = 39,
    ENGINE_KEY_COMMA = 44,
    ENGINE_KEY_MINUS = 45,
    ENGINE_KEY_PERIOD = 46,
    ENGINE_KEY_SLASH = 47,
    ENGINE_KEY_SEMICOLON = 59,
    ENGINE_KEY_EQUALS = 61,
    ENGINE_KEY_LEFT_BRACKET = 91,
    ENGINE_KEY_BACKSLASH = 92,
    ENGINE_KEY_RIGHT_BRACKET = 93,
    
    /* Function keys */
    ENGINE_KEY_ESCAPE = 256,
    ENGINE_KEY_ENTER,
    ENGINE_KEY_TAB,
    ENGINE_KEY_BACKSPACE,
    ENGINE_KEY_INSERT,
    ENGINE_KEY_DELETE,
    ENGINE_KEY_RIGHT,
    ENGINE_KEY_LEFT,
    ENGINE_KEY_DOWN,
    ENGINE_KEY_UP,
    
    /* Modifier keys */
    ENGINE_KEY_LEFT_SHIFT = 340,
    ENGINE_KEY_LEFT_CONTROL,
    ENGINE_KEY_LEFT_ALT,
    ENGINE_KEY_RIGHT_SHIFT = 344,
    ENGINE_KEY_RIGHT_CONTROL,
    ENGINE_KEY_RIGHT_ALT,
    
    ENGINE_KEY_F1 = 290, ENGINE_KEY_F2, ENGINE_KEY_F3, ENGINE_KEY_F4,
    ENGINE_KEY_F5, ENGINE_KEY_F6, ENGINE_KEY_F7, ENGINE_KEY_F8,
    ENGINE_KEY_F9, ENGINE_KEY_F10, ENGINE_KEY_F11, ENGINE_KEY_F12,
} engine_key_t;

/* Mouse buttons */
typedef enum {
    ENGINE_MOUSE_BUTTON_LEFT = 0,
    ENGINE_MOUSE_BUTTON_RIGHT = 1,
    ENGINE_MOUSE_BUTTON_MIDDLE = 2,
} engine_mouse_button_t;

/* Event data structures */
typedef struct {
    i32 width;
    i32 height;
} engine_event_resize_data_t;

typedef struct {
    engine_key_t key;
    bool repeat;
} engine_event_key_data_t;

typedef struct {
    i32 x;
    i32 y;
} engine_event_mouse_move_data_t;

typedef struct {
    engine_mouse_button_t button;
} engine_event_mouse_button_data_t;

typedef struct {
    f32 delta;
} engine_event_mouse_wheel_data_t;

/* Event union */
typedef struct {
    engine_event_type_t type;
    union {
        engine_event_resize_data_t resize;
        engine_event_key_data_t key;
        engine_event_mouse_move_data_t mouse_move;
        engine_event_mouse_button_data_t mouse_button;
        engine_event_mouse_wheel_data_t mouse_wheel;
    } data;
} engine_event_t;

/* Event callback */
typedef void (*engine_event_callback_t)(const engine_event_t* event, void* user_data);

/* Window creation parameters */
typedef struct {
    const char* title;
    i32 width;
    i32 height;
    i32 x;  /* Window position X (-1 for centered) */
    i32 y;  /* Window position Y (-1 for centered) */
    bool resizable;
    bool visible;
    engine_event_callback_t event_callback;
    void* user_data;
} platform_window_config_t;

/* Platform API - Must be implemented by each platform */

/**
 * Initialize the platform layer
 * @return ENGINE_SUCCESS on success, error code otherwise
 */
ENGINE_API engine_result_t platform_init(void);

/**
 * Shutdown the platform layer
 */
ENGINE_API void platform_shutdown(void);

/**
 * Create a new window
 * @param config Window configuration
 * @param out_window Pointer to receive the created window handle
 * @return ENGINE_SUCCESS on success, error code otherwise
 */
ENGINE_API engine_result_t platform_window_create(
    const platform_window_config_t* config,
    platform_window_t** out_window
);

/**
 * Destroy a window
 * @param window Window to destroy
 */
ENGINE_API void platform_window_destroy(platform_window_t* window);

/**
 * Check if window should close
 * @param window Window to check
 * @return true if window should close, false otherwise
 */
ENGINE_API bool platform_window_should_close(const platform_window_t* window);

/**
 * Poll and process platform events
 * This will call registered event callbacks
 */
ENGINE_API void platform_poll_events(void);

/**
 * Get window width
 * @param window Window to query
 * @return Window width in pixels
 */
ENGINE_API i32 platform_window_get_width(const platform_window_t* window);

/**
 * Get window height
 * @param window Window to query
 * @return Window height in pixels
 */
ENGINE_API i32 platform_window_get_height(const platform_window_t* window);

/**
 * Set window title
 * @param window Window to modify
 * @param title New window title
 */
ENGINE_API void platform_window_set_title(platform_window_t* window, const char* title);

/**
 * Show or hide window
 * @param window Window to modify
 * @param visible true to show, false to hide
 */
ENGINE_API void platform_window_set_visible(platform_window_t* window, bool visible);

/**
 * Get high-resolution time in seconds
 * @return Time in seconds since some fixed point
 */
ENGINE_API f64 platform_get_time(void);

/**
 * Present a pixel buffer to the window
 * @param window Window to present to
 * @param pixels RGBA pixel buffer (width * height * 4 bytes)
 * @param width Buffer width
 * @param height Buffer height
 */
ENGINE_API void platform_window_present_buffer(
    platform_window_t* window,
    const u32* pixels,
    i32 width,
    i32 height
);

/**
 * Sleep for specified milliseconds
 * @param milliseconds Time to sleep
 */
ENGINE_API void platform_sleep(u32 milliseconds);

#endif /* ENGINE_PLATFORM_H */
