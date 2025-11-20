#include "../include/dialogs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Helper to run zenity command and get output */
static char* run_zenity(const char* command) {
    char cmd_with_output[2048];
    const char* temp_file = "/tmp/engine_dialog_output.txt";
    
    /* Remove old temp file */
    remove(temp_file);
    
    /* Redirect output to temp file */
    snprintf(cmd_with_output, sizeof(cmd_with_output), "%s > %s", command, temp_file);
    
    int ret = system(cmd_with_output);
    if (ret != 0) {
        return NULL;
    }
    
    /* Read output from file */
    FILE* fp = fopen(temp_file, "r");
    if (!fp) {
        return NULL;
    }
    
    char buffer[1024];
    char* result = NULL;
    
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        /* Remove newline */
        size_t len = strlen(buffer);
        while (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\r')) {
            buffer[len-1] = '\0';
            len--;
        }
        
        /* Manually duplicate string */
        size_t path_len = strlen(buffer);
        result = (char*)malloc(path_len + 1);
        if (result) {
            strcpy(result, buffer);
        }
    }
    
    fclose(fp);
    remove(temp_file);
    return result;
}

/* Open file dialog */
char* dialog_open_file(
    const char* title,
    const char* default_path,
    const file_filter_t* filters,
    i32 filter_count
) {
    char command[2048];
    char filter_arg[1024] = "";
    
    /* Construct filter argument */
    if (filters && filter_count > 0) {
        /* Zenity format: --file-filter="Description | *.ext *.ext2" */
        /* We'll just take the first one for simplicity and convert ; to space */
        char pattern_copy[256];
        strncpy(pattern_copy, filters[0].pattern, 255);
        
        /* Replace ; with space for zenity */
        for(int i=0; pattern_copy[i]; i++) {
            if(pattern_copy[i] == ';') pattern_copy[i] = ' ';
        }
        
        snprintf(filter_arg, sizeof(filter_arg), "--file-filter=\"%s | %s\"", 
                 filters[0].description, pattern_copy);
    }
    
    snprintf(command, sizeof(command), 
             "zenity --file-selection --title=\"%s\" %s %s",
             title ? title : "Open File",
             default_path ? default_path : "",
             filter_arg);
             
    return run_zenity(command);
}

/* Save file dialog */
char* dialog_save_file(
    const char* title,
    const char* default_path,
    const file_filter_t* filters,
    i32 filter_count
) {
    char command[2048];
    char filter_arg[1024] = "";
    
    if (filters && filter_count > 0) {
        char pattern_copy[256];
        strncpy(pattern_copy, filters[0].pattern, 255);
        for(int i=0; pattern_copy[i]; i++) {
            if(pattern_copy[i] == ';') pattern_copy[i] = ' ';
        }
        snprintf(filter_arg, sizeof(filter_arg), "--file-filter=\"%s | %s\"", 
                 filters[0].description, pattern_copy);
    }
    
    snprintf(command, sizeof(command), 
             "zenity --file-selection --save --confirm-overwrite --title=\"%s\" %s %s",
             title ? title : "Save File",
             default_path ? default_path : "",
             filter_arg);
             
    return run_zenity(command);
}

/* Message dialog */
void dialog_message(const char* title, const char* message) {
    char command[2048];
    snprintf(command, sizeof(command), 
             "zenity --info --title=\"%s\" --text=\"%s\"",
             title ? title : "Message",
             message ? message : "");
    system(command);
}

/* Confirm dialog */
bool dialog_confirm(const char* title, const char* message) {
    char command[2048];
    snprintf(command, sizeof(command), 
             "zenity --question --title=\"%s\" --text=\"%s\"",
             title ? title : "Confirm",
             message ? message : "");
    return system(command) == 0;
}
