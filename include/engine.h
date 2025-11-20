#ifndef ENGINE_H
#define ENGINE_H

#include "platform.h"
#include "graphics.h"
#include "ui.h"
#include "input.h"
#include "audio.h"
#include "dialogs.h"

/* Engine configuration */
#define ENGINE_VERSION_MAJOR 0
#define ENGINE_VERSION_MINOR 1
#define ENGINE_VERSION_PATCH 0

/* Forward declarations */
typedef struct engine_window engine_window_t;

/* Window configuration */
typedef struct {
    const char* title;
    i32 width;
    i32 height;
    bool resizable;
    engine_event_callback_t event_callback;
    void* user_data;
} engine_window_config_t;

/* Engine initialization configuration */
typedef struct {
    const char* app_name;
    bool enable_logging;
} engine_config_t;

/**
 * Initialize the engine
 * @param config Engine configuration (can be NULL for defaults)
 * @return ENGINE_SUCCESS on success, error code otherwise
 */
ENGINE_API engine_result_t engine_init(const engine_config_t* config);

/**
 * Shutdown the engine
 */
ENGINE_API void engine_shutdown(void);

/**
 * Create a new window
 * @param config Window configuration
 * @param out_window Pointer to receive the created window handle
 * @return ENGINE_SUCCESS on success, error code otherwise
 */
ENGINE_API engine_result_t engine_window_create(
    const engine_window_config_t* config,
    engine_window_t** out_window
);

/**
 * Destroy a window
 * @param window Window to destroy
 */
ENGINE_API void engine_window_destroy(engine_window_t* window);

/**
 * Check if window should close
 * @param window Window to check
 * @return true if window should close, false otherwise
 */
ENGINE_API bool engine_window_should_close(const engine_window_t* window);

/**
 * Poll and process events for all windows
 */
ENGINE_API void engine_poll_events(void);

/**
 * Get window width
 * @param window Window to query
 * @return Window width in pixels
 */
ENGINE_API i32 engine_window_get_width(const engine_window_t* window);

/**
 * Get window height
 * @param window Window to query
 * @return Window height in pixels
 */
ENGINE_API i32 engine_window_get_height(const engine_window_t* window);

/**
 * Set window title
 * @param window Window to modify
 * @param title New window title
 */
ENGINE_API void engine_window_set_title(engine_window_t* window, const char* title);

/**
 * Show or hide window
 * @param window Window to modify
 * @param visible true to show, false to hide
 */
ENGINE_API void engine_window_set_visible(engine_window_t* window, bool visible);

/**
 * Get engine version string
 * @return Version string in format "major.minor.patch"
 */
ENGINE_API const char* engine_get_version(void);

/**
 * Get platform name
 * @return Platform name string
 */
ENGINE_API const char* engine_get_platform(void);

/**
 * Get high-resolution time in seconds
 * @return Time in seconds since engine initialization
 */
ENGINE_API f64 engine_get_time(void);

#endif /* ENGINE_H */
