#include "../include/engine.h"
#include "../include/input.h"
#include <stdio.h>

void on_event(const engine_event_t* event, void* user_data) {
    input_process_event(event);
    
    if (event->type == ENGINE_EVENT_KEY_PRESS) {
        printf("KEY PRESS: key=%d\n", event->data.key.key);
        
        if (event->data.key.key >= ENGINE_KEY_A && event->data.key.key <= ENGINE_KEY_Z) {
            char ch = 'a' + (event->data.key.key - ENGINE_KEY_A);
            printf("  -> Letter: %c\n", ch);
        } else if (event->data.key.key >= ENGINE_KEY_0 && event->data.key.key <= ENGINE_KEY_9) {
            char ch = '0' + (event->data.key.key - ENGINE_KEY_0);
            printf("  -> Number: %c\n", ch);
        } else if (event->data.key.key == ENGINE_KEY_SPACE) {
            printf("  -> SPACE\n");
        } else if (event->data.key.key == ENGINE_KEY_BACKSPACE) {
            printf("  -> BACKSPACE\n");
        } else if (event->data.key.key == ENGINE_KEY_ENTER) {
            printf("  -> ENTER\n");
        } else {
            printf("  -> Unknown/Special key\n");
        }
    }
}

int main(void) {
    printf("=== Keyboard Test ===\n");
    printf("Type keys to see if they're detected...\n");
    printf("Press ESC to exit\n\n");
    
    engine_config_t engine_config = {
        .app_name = "Keyboard Test",
        .enable_logging = false,
    };
    
    if (engine_init(&engine_config) != ENGINE_SUCCESS) {
        return 1;
    }
    
    input_init();
    
    engine_window_config_t window_config = {
        .title = "Keyboard Test - Press keys",
        .width = 400,
        .height = 200,
        .resizable = false,
        .event_callback = on_event,
        .user_data = NULL,
    };
    
    engine_window_t* window = NULL;
    if (engine_window_create(&window_config, &window) != ENGINE_SUCCESS) {
        input_shutdown();
        engine_shutdown();
        return 1;
    }
    
    while (!engine_window_should_close(window)) {
        input_update();
        engine_poll_events();
        
        if (input_was_key_pressed(ENGINE_KEY_ESCAPE)) {
            break;
        }
        
        platform_sleep(16);
    }
    
    engine_window_destroy(window);
    input_shutdown();
    engine_shutdown();
    
    printf("\nKeyboard test complete!\n");
    return 0;
}
