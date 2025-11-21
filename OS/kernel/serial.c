#include "serial.h"
#include "io.h"

#define PORT 0x3f8          // COM1

void init_serial() {
   outb(PORT + 1, 0x00);    // Disable all interrupts
   outb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
   outb(PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
   outb(PORT + 1, 0x00);    //                  (hi byte)
   outb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
   outb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
   outb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

int is_transmit_empty() {
   return inb(PORT + 5) & 0x20;
}

void serial_putc(char c) {
   while (is_transmit_empty() == 0);
   outb(PORT, c);
}

void serial_print(const char* str) {
    while (*str) {
        serial_putc(*str++);
    }
}

void serial_print_hex(unsigned long long n) {
    serial_print("0x");
    char buffer[20];
    int i = 0;
    
    if (n == 0) {
        serial_print("0");
        return;
    }
    
    while (n > 0) {
        int digit = n % 16;
        if (digit < 10) buffer[i++] = '0' + digit;
        else buffer[i++] = 'A' + (digit - 10);
        n /= 16;
    }
    
    while (i > 0) {
        serial_putc(buffer[--i]);
    }
}

void serial_print_dec(unsigned long long n) {
    if (n == 0) {
        serial_print("0");
        return;
    }
    
    char buffer[24];
    int i = 0;
    
    while (n > 0) {
        buffer[i++] = '0' + (n % 10);
        n /= 10;
    }
    
    while (i > 0) {
        serial_putc(buffer[--i]);
    }
}
