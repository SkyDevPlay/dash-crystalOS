#ifndef SERIAL_H
#define SERIAL_H

#include "../types.h"

int initSerial(u16 port);
void writeSerial(u16 port, u8 c);
u8 readSerial(u16 port);
void serialString(u16 port, char *str);

#endif