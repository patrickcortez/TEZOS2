#ifndef ENGINE_AUDIO_H
#define ENGINE_AUDIO_H

#include "types.h"

/* Forward declaration */
typedef struct audio_sound audio_sound_t;

/* Audio system initialization */
ENGINE_API engine_result_t audio_init(void);
ENGINE_API void audio_shutdown(void);

/* Sound loading and management */
ENGINE_API audio_sound_t* audio_load_sound(const char* filepath);
ENGINE_API void audio_destroy_sound(audio_sound_t* sound);

/* Playback control */
ENGINE_API void audio_play(audio_sound_t* sound, bool loop);
ENGINE_API void audio_stop(audio_sound_t* sound);
ENGINE_API void audio_pause(audio_sound_t* sound);
ENGINE_API void audio_resume(audio_sound_t* sound);

/* Volume control (0.0f to 1.0f) */
ENGINE_API void audio_set_volume(audio_sound_t* sound, f32 volume);
ENGINE_API f32 audio_get_volume(audio_sound_t* sound);
ENGINE_API void audio_set_master_volume(f32 volume);
ENGINE_API f32 audio_get_master_volume(void);

/* State queries */
ENGINE_API bool audio_is_playing(const audio_sound_t* sound);
ENGINE_API bool audio_is_looping(const audio_sound_t* sound);

#endif /* ENGINE_AUDIO_H */
