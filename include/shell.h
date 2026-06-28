#ifndef SHELL_H
#define SHELL_H

#define MAX_CMD     1024
#define MAX_ARGS    16
#define MAX_ARG_LEN 128

/*
 * shell_init()
 *   Affiche le prompt initial.
 *   À appeler une seule fois depuis main(), après init_fs().
 */
void shell_init(void);

/*
 * shell_handle_key(keycode, is_shift)
 *   À appeler depuis handle_keyboard() à chaque touche reçue.
 *   Gère l'écho, le buffer de commande, et l'exécution au Enter.
 */
void shell_handle_key(unsigned char keycode, int is_shift);

#endif /* SHELL_H */
