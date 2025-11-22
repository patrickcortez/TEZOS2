#include "io.h"
#include "types.h"
#include "video.h"

extern void print_char(char c);
extern void print_str(const char* str);

/* Modifier key states */
static int shift_pressed = 0;
static int ctrl_pressed = 0;

/* Scan code set 1 mapping (lowercase) */
char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',   
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,   
  '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-',   
    0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Scan code set 1 mapping (shifted) */
char kbd_us_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',   
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',   
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',   
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   
  '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-',   
    0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

extern void shell_handle_key(char c);

int keyboard_get_shift() {
    return shift_pressed;
}

int keyboard_get_ctrl() {
    return ctrl_pressed;
}

void keyboard_handler() {
    u8 scancode = inb(0x60);
    
    /* Check for shift key press/release */
    if (scancode == 0x2A || scancode == 0x36) {  /* Left/Right Shift press */
        shift_pressed = 1;
    } else if (scancode == 0xAA || scancode == 0xB6) {  /* Left/Right Shift release */
        shift_pressed = 0;
    } else if (scancode == 0x1D) {  /* Ctrl press */
        ctrl_pressed = 1;
    } else if (scancode == 0x9D) {  /* Ctrl release */
        ctrl_pressed = 0;
    } else if (scancode & 0x80) {
        /* Other key release - ignore */
    } else {
        /* Key press - use shift state to determine character */
        char c = shift_pressed ? kbd_us_shift[scancode] : kbd_us[scancode];
        if (c) {
            shell_handle_key(c);
        }
    }
    
    /* Send End of Interrupt (EOI) to PIC1 */
    outb(0x20, 0x20);
}

void keyboard_init() {
    shift_pressed = 0;
    ctrl_pressed = 0;
}
