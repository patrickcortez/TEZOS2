#ifndef SERIAL_H
#define SERIAL_H

void init_serial();
void serial_putc(char c);
void serial_print(const char* str);
void serial_print_hex(unsigned long long n);
void serial_print_dec(unsigned long long n);

#endif
