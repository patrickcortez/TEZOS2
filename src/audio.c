#include "../include/audio.h"
#include "../include/types.h"
#include <stdio.h>
#include <stdlib.h>

#define MINIAUDIO_IMPLEMENTATION
#include "../include/miniaudio.h"

/* Audio sound structure */
struct audio_sound {
    ma_sound sound;
    bool loaded;
    f32 volume;
};

/* Global audio state */
static struct {
    ma_engine engine;
    bool initialized;
    f32 master_volume;
} g_audio = {0};

/* Initialize audio system */
engine_result_t audio_init(void) {
    if (g_audio.initialized) {
        ENGINE_LOG_WARN("Audio already initialized");
        return ENGINE_SUCCESS;
    }
    
    ma_result result = ma_engine_init(NULL, &g_audio.engine);
    if (result != MA_SUCCESS) {
        ENGINE_LOG_ERROR("Failed to initialize audio engine");
        return ENGINE_ERROR;
    }
    
    g_audio.initialized = true;
    g_audio.master_volume = 1.0f;
    
    ENGINE_LOG_INFO("Audio system initialized");
    return ENGINE_SUCCESS;
}

/* Shutdown audio system */
void audio_shutdown(void) {
    if (!g_audio.initialized) {
        return;
    }
    
    ma_engine_uninit(&g_audio.engine);
    g_audio.initialized = false;
    
    ENGINE_LOG_INFO("Audio system shut down");
}

/* Load sound from file */
audio_sound_t* audio_load_sound(const char* filename) {
    if (!g_audio.initialized) {
        ENGINE_LOG_ERROR("Audio not initialized");
        return NULL;
    }
    
    if (!filename) {
        ENGINE_LOG_ERROR("Invalid filename");
        return NULL;
    }
    
    printf("[DEBUG] audio_load_sound: Attempting to load '%s'\n", filename);
    
    audio_sound_t* sound = (audio_sound_t*)malloc(sizeof(audio_sound_t));
    if (!sound) {
        ENGINE_LOG_ERROR("Failed to allocate sound");
        printf("[DEBUG] audio_load_sound: Failed to allocate memory for sound\n");
        return NULL;
    }
    
    /* Load sound from file */
    ma_result result = ma_sound_init_from_file(&g_audio.engine, filename, 0, NULL, NULL, &sound->sound);
    if (result != MA_SUCCESS) {
        ENGINE_LOG_ERROR("Failed to load sound '%s': %d", filename, result);
        free(sound);
        return NULL;
    }
    
    sound->loaded = true;
    sound->volume = 1.0f;
    
    ENGINE_LOG_INFO("Loaded sound: %s", filename);
    return sound;
}

/* Destroy sound */
void audio_destroy_sound(audio_sound_t* sound) {
    if (!sound) return;
    
    if (sound->loaded) {
        ma_sound_uninit(&sound->sound);
    }
    
    free(sound);
}

/* Play sound */
void audio_play(audio_sound_t* sound, bool loop) {
    if (!sound || !sound->loaded) return;
    
    /* Set looping */
    ma_sound_set_looping(&sound->sound, loop ? MA_TRUE : MA_FALSE);
    
    /* Start from beginning */
    ma_sound_seek_to_pcm_frame(&sound->sound, 0);
    
    /* Start playback */
    ma_sound_start(&sound->sound);
}

/* Stop sound */
void audio_stop(audio_sound_t* sound) {
    if (!sound || !sound->loaded) return;
    
    ma_sound_stop(&sound->sound);
    ma_sound_seek_to_pcm_frame(&sound->sound, 0);
}

/* Pause sound */
void audio_pause(audio_sound_t* sound) {
    if (!sound || !sound->loaded) return;
    
    ma_sound_stop(&sound->sound);
}

/* Resume sound */
void audio_resume(audio_sound_t* sound) {
    if (!sound || !sound->loaded) return;
    
    ma_sound_start(&sound->sound);
}

/* Set sound volume */
void audio_set_volume(audio_sound_t* sound, f32 volume) {
    if (!sound || !sound->loaded) return;
    
    /* Clamp volume */
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    
    sound->volume = volume;
    ma_sound_set_volume(&sound->sound, volume);
}

/* Get sound volume */
f32 audio_get_volume(audio_sound_t* sound) {
    if (!sound || !sound->loaded) return 0.0f;
    return sound->volume;
}

/* Set master volume */
void audio_set_master_volume(f32 volume) {
    if (!g_audio.initialized) return;
    
    /* Clamp volume */
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    
    g_audio.master_volume = volume;
    ma_engine_set_volume(&g_audio.engine, volume);
}

/* Get master volume */
f32 audio_get_master_volume(void) {
    return g_audio.master_volume;
}

/* Check if sound is playing */
bool audio_is_playing(const audio_sound_t* sound) {
    if (!sound || !sound->loaded) return false;
    
    return ma_sound_is_playing(&sound->sound) == MA_TRUE;
}

/* Check if sound is looping */
bool audio_is_looping(const audio_sound_t* sound) {
    if (!sound || !sound->loaded) return false;
    
    return ma_sound_is_looping(&sound->sound) == MA_TRUE;
}
