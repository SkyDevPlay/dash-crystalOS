#ifndef SERIAL_H
#define SERIAL_H

// Declare the external variable for the base I/O port, assuming it's for COM1
extern int coms;

// Function to initialize the serial port
void initSerial(void);

// If you have a print function for serial, add it here too:
// void serial_write(char a);

#endif