#include "io.h"
#include "types.h"
#include "video.h"

extern void print_char(char c);
extern void print_str(const char* str);

/* Scan code set 1 mapping (incomplete) */
char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',   
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,   
  '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-',   
    0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

extern void shell_handle_key(char c);

void keyboard_handler() {
    u8 scancode = inb(0x60);
    
    /* If top bit is set, it's a key release */
    if (scancode & 0x80) {
        /* Key release - ignore for now */
    } else {
        /* Key press */
        char c = kbd_us[scancode];
        if (c) {
            shell_handle_key(c);
        }
    }
    
    /* Send End of Interrupt (EOI) to PIC1 */
    outb(0x20, 0x20);
}
