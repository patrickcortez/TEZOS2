#include "../include/engine.h"
#include <stdio.h>

/* Event callback function */
void on_event(const engine_event_t* event, void* user_data) {
    (void)user_data;  /* Unused */
    
    switch (event->type) {
        case ENGINE_EVENT_WINDOW_CLOSE:
            printf("Window close requested\n");
            break;
            
        case ENGINE_EVENT_WINDOW_RESIZE:
            printf("Window resized to %dx%d\n",
                   event->data.resize.width,
                   event->data.resize.height);
            break;
            
        case ENGINE_EVENT_KEY_PRESS:
            printf("Key pressed: %d\n", event->data.key.key);
            if (event->data.key.key == ENGINE_KEY_ESCAPE) {
                printf("Escape key pressed, closing...\n");
            }
            break;
            
        case ENGINE_EVENT_MOUSE_MOVE:
            /* Uncomment to see mouse movement (verbose) */
            /* printf("Mouse moved to (%d, %d)\n",
                   event->data.mouse_move.x,
                   event->data.mouse_move.y); */
            break;
            
        default:
            break;
    }
}

int main(void) {
    printf("=== 2D Engine - Basic Window Example ===\n");
    printf("Platform: %s\n\n", engine_get_platform());
    
    /* Initialize engine */
    engine_config_t engine_config = {
        .app_name = "Basic Window Example",
        .enable_logging = true,
    };
    
    engine_result_t result = engine_init(&engine_config);
    if (result != ENGINE_SUCCESS) {
        fprintf(stderr, "Failed to initialize engine: %d\n", result);
        return 1;
    }
    
    printf("Engine version: %s\n\n", engine_get_version());
    
    /* Create window */
    engine_window_config_t window_config = {
        .title = "Basic Window",
        .width = 800,
        .height = 600,
        .resizable = true,
        .event_callback = on_event,
        .user_data = NULL,
    };
    
    engine_window_t* window = NULL;
    result = engine_window_create(&window_config, &window);
    if (result != ENGINE_SUCCESS) {
        fprintf(stderr, "Failed to create window: %d\n", result);
        engine_shutdown();
        return 1;
    }
    
    printf("Window created successfully!\n");
    printf("Press ESC or close the window to exit.\n\n");
    
    /* Main loop */
    double last_time = engine_get_time();
    int frame_count = 0;
    
    while (!engine_window_should_close(window)) {
        /* Poll events */
        engine_poll_events();
        
        /* Simple FPS counter */
        frame_count++;
        double current_time = engine_get_time();
        if (current_time - last_time >= 1.0) {
            printf("FPS: %d\n", frame_count);
            frame_count = 0;
            last_time = current_time;
        }
        
        /* In a real application, you would:
         * 1. Update game logic
         * 2. Render graphics
         * 3. Swap buffers
         */
    }
    
    /* Cleanup */
    printf("\nCleaning up...\n");
    engine_window_destroy(window);
    engine_shutdown();
    
    printf("Goodbye!\n");
    return 0;
}
