#ifndef SHELL_H
#define SHELL_H

#define MAX_CMD     1024
#define MAX_ARGS    16
#define MAX_ARG_LEN 128

void shell_init(void);
void shell_handle_key(unsigned char keycode, int is_shift);

#endif /* SHELL_H */
