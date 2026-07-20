#ifndef IO_H
#define IO_H

#include "sys/io.h"

int puts(const char *s);
int printf(const char *format, ...);
int serial_printf(const char *format, ...);

#endif