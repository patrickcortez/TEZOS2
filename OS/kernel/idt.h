#ifndef IDT_H
#define IDT_H

#include "types.h"

void idt_install();
void set_idt_gate(u8 num, u64 base, u16 sel, u8 flags);

#endif
