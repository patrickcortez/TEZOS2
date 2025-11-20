#ifndef ENGINE_DIALOGS_H
#define ENGINE_DIALOGS_H

#include "types.h"

/* File dialog filters */
typedef struct {
    const char* description;  /* e.g., "Audio Files" */
    const char* pattern;      /* e.g., "*.wav;*.mp3;*.ogg" */
} file_filter_t;

/* Open file dialog - returns allocated string (caller must free) or NULL if cancelled */
char* dialog_open_file(
    const char* title,
    const char* default_path,
    const file_filter_t* filters,
    i32 filter_count
);

/* Save file dialog - returns allocated string (caller must free) or NULL if cancelled */
char* dialog_save_file(
    const char* title,
    const char* default_path,
    const file_filter_t* filters,
    i32 filter_count
);

/* Simple message dialogs */
void dialog_message(const char* title, const char* message);
bool dialog_confirm(const char* title, const char* message);

#endif /* ENGINE_DIALOGS_H */
