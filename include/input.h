#ifndef ENGINE_INPUT_H
#define ENGINE_INPUT_H

#include "types.h"
#include "platform.h"

/* Input system - Easy polling-based input queries */

/* Initialization and frame management */
ENGINE_API void input_init(void);
ENGINE_API void input_shutdown(void);
ENGINE_API void input_update(void);  /* Call at start of each frame */
ENGINE_API void input_process_event(const engine_event_t* event);

/* Keyboard queries */
ENGINE_API bool input_is_key_down(engine_key_t key);
ENGINE_API bool input_was_key_pressed(engine_key_t key);   /* Pressed THIS frame */
ENGINE_API bool input_was_key_released(engine_key_t key);  /* Released THIS frame */

/* Mouse button queries */
ENGINE_API bool input_is_mouse_button_down(engine_mouse_button_t button);
ENGINE_API bool input_was_mouse_button_pressed(engine_mouse_button_t button);
ENGINE_API bool input_was_mouse_button_released(engine_mouse_button_t button);

/* Mouse position and movement */
ENGINE_API void input_get_mouse_position(i32* out_x, i32* out_y);
ENGINE_API void input_get_mouse_delta(i32* out_dx, i32* out_dy);  /* Movement since last frame */
ENGINE_API i32 input_get_mouse_x(void);
ENGINE_API i32 input_get_mouse_y(void);

/* Convenience helpers */
ENGINE_API bool input_is_key_down_any(engine_key_t* keys, i32 count);  /* Any of the keys down */
ENGINE_API void input_reset(void);  /* Reset all state */

#endif /* ENGINE_INPUT_H */
