#include "editor.h"
#include "video.h"
#include "fs.h"
#include "io.h"
#include "types.h"

#define MAX_LINES 100
#define MAX_LINE_LENGTH 80
#define SCREEN_HEIGHT 25
#define SCREEN_WIDTH 80

static char lines[MAX_LINES][MAX_LINE_LENGTH];
static int line_count = 0;
static int cursor_line = 0;
static int cursor_col = 0;
static int top_line = 0;  /* First line displayed on screen */
static char filename[256];
static int modified = 0;

extern void print_str(const char* str);
extern void print_char(char c);
extern void clear_screen();
extern int str_strcmp(const char* s1, const char* s2);
extern void strcpy(char* dst, const char* src);
extern int strlen(const char* str);
extern int keyboard_get_ctrl();
extern int keyboard_get_shift();

static void editor_clear_lines() {
    for (int i = 0; i < MAX_LINES; i++) {
        for (int j = 0; j < MAX_LINE_LENGTH; j++) {
            lines[i][j] = 0;
        }
    }
    line_count = 1;  /* Start with one empty line */
    lines[0][0] = 0;
}

static void editor_load_file(const char* fname) {
    u8 buffer[4096];
    file_info_t info;
    int size = -1;
    if (fs_stat(fname, &info) == 0) {
        size = fs_read_file(fname, buffer, 0, info.size > 4096 ? 4096 : info.size);
    }
    
    editor_clear_lines();
    
    if (size < 0) {
        /* New file */
        return;
    }
    
    /* Parse into lines */
    line_count = 0;
    int col = 0;
    
    for (int i = 0; i < size && line_count < MAX_LINES; i++) {
        if (buffer[i] == '\n') {
            lines[line_count][col] = 0;
            line_count++;
            col = 0;
        } else if (col < MAX_LINE_LENGTH - 1) {
            lines[line_count][col++] = buffer[i];
        }
    }
    
    if (line_count < MAX_LINES) {
        lines[line_count][col] = 0;
        if (col > 0 || line_count == 0) line_count++;
    }
    
    if (line_count == 0) {
        line_count = 1;
        lines[0][0] = 0;
    }
}

static void editor_save_file() {
    /* Build file content */
    u8 buffer[4096];
    int buf_idx = 0;
    
    for (int i = 0; i < line_count && buf_idx < 4090; i++) {
        int j = 0;
        while (lines[i][j] && buf_idx < 4090) {
            buffer[buf_idx++] = lines[i][j++];
        }
        if (i < line_count - 1 && buf_idx < 4095) {
            buffer[buf_idx++] = '\n';
        }
    }
    
    /* Save to file */
    fs_write_file(filename, buffer, 0, buf_idx);
    modified = 0;
}

static void editor_display() {
    clear_screen();
    
    /* Display header */
    print_str("=== CORTEZ EDITOR === ");
    print_str(filename);
    if (modified) print_str(" [MODIFIED]");
    print_str("\n");
    print_str("Ctrl+S: Save | ESC: Exit | Arrows: Move | Enter: New Line\n");
    print_str("----------------------------------------------------------------\n");
    
    /* Display lines */
    int screen_line = 3;
    for (int i = top_line; i < line_count && screen_line < SCREEN_HEIGHT - 1; i++) {
        /* Move cursor to line */
        volatile u16* vga = (u16*)0xB8000;
        int offset = screen_line * SCREEN_WIDTH;
        
        /* Print line number */
        char line_num[5];
        int num = i + 1;
        int idx = 0;
        if (num >= 10) line_num[idx++] = '0' + (num / 10);
        line_num[idx++] = '0' + (num % 10);
        line_num[idx++] = ' ';
        line_num[idx] = 0;
        
        for (int j = 0; line_num[j]; j++) {
            vga[offset++] = 0x0700 | line_num[j];
        }
        
        /* Print line content */
        for (int j = 0; j < MAX_LINE_LENGTH && lines[i][j]; j++) {
            vga[offset++] = 0x0700 | lines[i][j];
        }
        
        screen_line++;
    }
    
    /* Position cursor */
    int display_line = 3 + (cursor_line - top_line);
    int display_col = 3 + cursor_col;  /* 3 = line number width */
    
    /* Set VGA cursor position */
    u16 pos = display_line * SCREEN_WIDTH + display_col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (u8)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (u8)((pos >> 8) & 0xFF));
}

void editor_edit(const char* fname) {
    strcpy(filename, fname);
    editor_load_file(fname);
    cursor_line = 0;
    cursor_col = 0;
    top_line = 0;
    modified = 0;
    
    editor_display();
    
    /* Editor loop */    
    while (1) {
        /* Wait for keyboard input */
        u8 scancode;
        while (!(inb(0x64) & 1));  /* Wait for key */
        scancode = inb(0x60);
        
        if (scancode & 0x80) continue;  /* Key release */
        
        /* Handle special keys */
        if (scancode == 0x01) {  /* ESC */
            if (modified) {
                clear_screen();
                print_str("File modified. Save? (y/n): ");
                /* Simple y/n - just break for now */
            }
            break;
        } else if (scancode == 0x1F && keyboard_get_ctrl()) {  /* Ctrl+S */
            editor_save_file();
            editor_display();
        } else if (scancode == 0x48) {  /* Up arrow */
            if (cursor_line > 0) {
                cursor_line--;
                if (cursor_line < top_line) top_line = cursor_line;
                int len = strlen(lines[cursor_line]);
                if (cursor_col > len) cursor_col = len;
                editor_display();
            }
        } else if (scancode == 0x50) {  /* Down arrow */
            if (cursor_line < line_count - 1) {
                cursor_line++;
                if (cursor_line >= top_line + (SCREEN_HEIGHT - 4)) {
                    top_line++;
                }
                int len = strlen(lines[cursor_line]);
                if (cursor_col > len) cursor_col = len;
                editor_display();
            }
        } else if (scancode == 0x4B) {  /* Left arrow */
            if (cursor_col > 0) {
                cursor_col--;
                editor_display();
            }
        } else if (scancode == 0x4D) {  /* Right arrow */
            int len = strlen(lines[cursor_line]);
            if (cursor_col < len && cursor_col < MAX_LINE_LENGTH - 1) {
                cursor_col++;
                editor_display();
            }
        } else if (scancode == 0x1C) {  /* Enter */
            if (line_count < MAX_LINES - 1) {
                /* Insert new line */
                for (int i = line_count; i > cursor_line; i--) {
                    strcpy(lines[i], lines[i - 1]);
                }
                line_count++;
                cursor_line++;
                lines[cursor_line][0] = 0;
                cursor_col = 0;
                modified = 1;
                editor_display();
            }
        } else if (scancode == 0x0E) {  /* Backspace */
            if (cursor_col > 0) {
                int len = strlen(lines[cursor_line]);
                for (int i = cursor_col - 1; i < len; i++) {
                    lines[cursor_line][i] = lines[cursor_line][i + 1];
                }
                cursor_col--;
                modified = 1;
                editor_display();
            }
        } else if (scancode >= 0x10 && scancode <= 0x32) {
            /* Letter/number/symbol keys - use shift-aware mapping */
            extern char kbd_us[128];
            extern char kbd_us_shift[128];
            
            char c = keyboard_get_shift() ? kbd_us_shift[scancode] : kbd_us[scancode];
            
            if (c && cursor_col < MAX_LINE_LENGTH - 1 && scancode != 0x1F) {
                /* Skip if Ctrl is pressed (for Ctrl+S, etc) */
                if (keyboard_get_ctrl()) continue;
                
                int len = strlen(lines[cursor_line]);
                /* Shift characters right */
                for (int i = len; i >= cursor_col; i--) {
                    lines[cursor_line][i + 1] = lines[cursor_line][i];
                }
                lines[cursor_line][cursor_col] = c;
                cursor_col++;
                modified = 1;
                editor_display();
            }
        }
    }
    
    clear_screen();
}
