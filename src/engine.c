#include "../include/engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Engine state */
typedef struct {
    bool initialized;
    bool logging_enabled;
    f64 start_time;
    char version_string[32];
} engine_state_t;

static engine_state_t g_engine_state = {0};

/* Window wrapper structure */
struct engine_window {
    platform_window_t* platform_window;
};

/* Helper function to get version string */
static const char* build_version_string(void) {
    static char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d.%d.%d",
             ENGINE_VERSION_MAJOR,
             ENGINE_VERSION_MINOR,
             ENGINE_VERSION_PATCH);
    return buffer;
}

engine_result_t engine_init(const engine_config_t* config) {
    if (g_engine_state.initialized) {
        ENGINE_LOG_WARN("Engine already initialized");
        return ENGINE_SUCCESS;
    }

    /* Set up engine state */
    memset(&g_engine_state, 0, sizeof(engine_state_t));
    
    if (config) {
        g_engine_state.logging_enabled = config->enable_logging;
    }

    /* Initialize platform layer */
    engine_result_t result = platform_init();
    if (result != ENGINE_SUCCESS) {
        ENGINE_LOG_ERROR("Failed to initialize platform layer");
        return result;
    }

    /* Record start time */
    g_engine_state.start_time = platform_get_time();
    g_engine_state.initialized = true;

    /* Build version string */
    strncpy(g_engine_state.version_string, build_version_string(),
            sizeof(g_engine_state.version_string) - 1);

    ENGINE_LOG_INFO("Engine initialized successfully");
    ENGINE_LOG_INFO("Version: %s", g_engine_state.version_string);
    ENGINE_LOG_INFO("Platform: %s", ENGINE_PLATFORM_NAME);

    return ENGINE_SUCCESS;
}

void engine_shutdown(void) {
    if (!g_engine_state.initialized) {
        ENGINE_LOG_WARN("Engine not initialized");
        return;
    }

    ENGINE_LOG_INFO("Shutting down engine");

    /* Shutdown platform layer */
    platform_shutdown();

    /* Clear engine state */
    memset(&g_engine_state, 0, sizeof(engine_state_t));
}

engine_result_t engine_window_create(
    const engine_window_config_t* config,
    engine_window_t** out_window
) {
    if (!g_engine_state.initialized) {
        ENGINE_LOG_ERROR("Engine not initialized");
        return ENGINE_ERROR_NOT_INITIALIZED;
    }

    if (!config || !out_window) {
        ENGINE_LOG_ERROR("Invalid parameters");
        return ENGINE_ERROR_INVALID_PARAM;
    }

    /* Allocate window wrapper */
    engine_window_t* window = (engine_window_t*)malloc(sizeof(engine_window_t));
    if (!window) {
        ENGINE_LOG_ERROR("Failed to allocate window");
        return ENGINE_ERROR_OUT_OF_MEMORY;
    }

    memset(window, 0, sizeof(engine_window_t));

    /* Create platform window configuration */
    platform_window_config_t platform_config = {
        .title = config->title ? config->title : "Engine Window",
        .width = config->width > 0 ? config->width : 800,
        .height = config->height > 0 ? config->height : 600,
        .x = -1,  /* Centered */
        .y = -1,  /* Centered */
        .resizable = config->resizable,
        .visible = true,
        .event_callback = config->event_callback,
        .user_data = config->user_data,
    };

    /* Create platform window */
    engine_result_t result = platform_window_create(&platform_config, &window->platform_window);
    if (result != ENGINE_SUCCESS) {
        ENGINE_LOG_ERROR("Failed to create platform window");
        free(window);
        return result;
    }

    *out_window = window;
    ENGINE_LOG_INFO("Window created: %s (%dx%d)",
                    platform_config.title,
                    platform_config.width,
                    platform_config.height);

    return ENGINE_SUCCESS;
}

void engine_window_destroy(engine_window_t* window) {
    if (!window) {
        ENGINE_LOG_WARN("Attempted to destroy NULL window");
        return;
    }

    if (window->platform_window) {
        platform_window_destroy(window->platform_window);
    }

    free(window);
    ENGINE_LOG_INFO("Window destroyed");
}

bool engine_window_should_close(const engine_window_t* window) {
    if (!window || !window->platform_window) {
        return true;
    }

    return platform_window_should_close(window->platform_window);
}

void engine_poll_events(void) {
    if (!g_engine_state.initialized) {
        ENGINE_LOG_WARN("Engine not initialized");
        return;
    }

    platform_poll_events();
}

i32 engine_window_get_width(const engine_window_t* window) {
    if (!window || !window->platform_window) {
        return 0;
    }

    return platform_window_get_width(window->platform_window);
}

i32 engine_window_get_height(const engine_window_t* window) {
    if (!window || !window->platform_window) {
        return 0;
    }

    return platform_window_get_height(window->platform_window);
}

void engine_window_set_title(engine_window_t* window, const char* title) {
    if (!window || !window->platform_window || !title) {
        ENGINE_LOG_WARN("Invalid parameters for set_title");
        return;
    }

    platform_window_set_title(window->platform_window, title);
}

void engine_window_set_visible(engine_window_t* window, bool visible) {
    if (!window || !window->platform_window) {
        ENGINE_LOG_WARN("Invalid window for set_visible");
        return;
    }

    platform_window_set_visible(window->platform_window, visible);
}

const char* engine_get_version(void) {
    return g_engine_state.version_string;
}

const char* engine_get_platform(void) {
    return ENGINE_PLATFORM_NAME;
}

f64 engine_get_time(void) {
    if (!g_engine_state.initialized) {
        return 0.0;
    }

    return platform_get_time() - g_engine_state.start_time;
}
