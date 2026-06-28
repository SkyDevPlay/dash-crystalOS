#include "shell.h"
#include "sys/keymap.h"
#include "sys/io.h"
#include "io.h"
#include "fat.h"
#include "string.h"
#include "malloc.h"

/* ─────────────────────────────────────────────
   Buffer interne de la commande en cours
   ───────────────────────────────────────────── */
static char cmd[MAX_CMD];
static int  cmd_len = 0;

/* ─────────────────────────────────────────────
   Utilitaires internes
   ───────────────────────────────────────────── */

/* Affiche le prompt */
static void print_prompt(void) {
    setColor(CYAN);
    puts("D$>> ");
    setColor(WHITE);
}

/* Remet le buffer à zéro */
static void reset_cmd(void) {
    for (int i = 0; i < MAX_CMD; i++) cmd[i] = 0;
    cmd_len = 0;
}

/*
 * split_args(input, argv, argc)
 *   Découpe une chaîne séparée par des espaces en tokens.
 *   Retourne le nombre de tokens trouvés.
 *   Les pointeurs dans argv pointent directement dans `input`
 *   (qui est modifié : les espaces sont remplacés par '\0').
 */
static int split_args(char *input, char *argv[], int max_args) {
    int argc = 0;
    char *p = input;

    while (*p && argc < max_args) {
        /* Sauter les espaces */
        while (*p == ' ') p++;
        if (!*p) break;

        argv[argc++] = p;

        /* Avancer jusqu'au prochain espace */
        while (*p && *p != ' ') p++;
        if (*p == ' ') *p++ = '\0';
    }
    return argc;
}

/* ─────────────────────────────────────────────
   Commandes
   ───────────────────────────────────────────── */

/* help — liste les commandes disponibles */
static void cmd_help(void) {
    setColor(YELLOW);
    puts("Commandes disponibles :");
    setColor(WHITE);
    puts("  help             Affiche cette aide");
    puts("  ls               Liste les fichiers du repertoire racine");
    puts("  cat <fichier>    Affiche le contenu d'un fichier");
    puts("  write <fich> <texte>  Ecrit du texte dans un fichier");
    puts("  echo <texte>     Affiche du texte");
    puts("  clear            Efface l'ecran");
}

/* ls — liste le répertoire racine FAT */
static void cmd_ls(void) {
    setColor(GREEN);
    list_dir();
    setColor(WHITE);
}

/* cat <fichier> — lit et affiche un fichier */
static void cmd_cat(int argc, char *argv[]) {
    if (argc < 2) {
        setColor(RED);
        puts("Usage : cat <fichier>");
        setColor(WHITE);
        return;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) {
        setColor(RED);
        printf("cat : impossible d'ouvrir '%s'\n", argv[1]);
        setColor(WHITE);
        return;
    }

    /* Lecture par blocs de 64 octets */
    char buf[65];
    u32  n;
    while ((n = fread(buf, 1, 64, f)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }
    putchar('\n');
    fclose(f);
}

/*
 * write <fichier> <texte...>
 *   Écrase le fichier avec le texte fourni (le fichier doit déjà
 *   exister dans la FAT — la création de nouvelle entrée n'est pas
 *   encore implémentée dans fat.c).
 */
static void cmd_write(int argc, char *argv[]) {
    if (argc < 3) {
        setColor(RED);
        puts("Usage : write <fichier> <texte>");
        setColor(WHITE);
        return;
    }

    FILE *f = fopen(argv[1], "w");
    if (!f) {
        setColor(RED);
        printf("write : impossible d'ouvrir '%s'\n", argv[1]);
        setColor(WHITE);
        return;
    }

    /* Recompose la chaîne à écrire à partir des arguments restants */
    fseek(f, 0, SEEK_SET);
    for (int i = 2; i < argc; i++) {
        fwrite(argv[i], 1, strlen(argv[i]), f);
        if (i + 1 < argc) fwrite(" ", 1, 1, f);
    }

    fclose(f);
    setColor(GREEN);
    printf("Ecrit dans '%s'.\n", argv[1]);
    setColor(WHITE);
}

/* echo <texte...> — réaffiche les arguments */
static void cmd_echo(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        printf("%s", argv[i]);
        if (i + 1 < argc) putchar(' ');
    }
    putchar('\n');
}

/* clear — efface l'écran VGA */
static void cmd_clear(void) {
    clearScreen();   /* défini dans sys/io.h / krnl/sys/io.c */
}

/* ─────────────────────────────────────────────
   Dispatcher principal
   ───────────────────────────────────────────── */
static void execute_command(char *input) {
    /* Copie de travail pour split_args (modifie la chaîne) */
    static char buf[MAX_CMD];
    strncpy(buf, input, MAX_CMD - 1);
    buf[MAX_CMD - 1] = '\0';

    char *argv[MAX_ARGS];
    int   argc = split_args(buf, argv, MAX_ARGS);

    if (argc == 0) return;   /* ligne vide */

    if      (strcmp(argv[0], "help")  == 0) cmd_help();
    else if (strcmp(argv[0], "ls")    == 0) cmd_ls();
    else if (strcmp(argv[0], "cat")   == 0) cmd_cat(argc, argv);
    else if (strcmp(argv[0], "write") == 0) cmd_write(argc, argv);
    else if (strcmp(argv[0], "echo")  == 0) cmd_echo(argc, argv);
    else if (strcmp(argv[0], "clear") == 0) cmd_clear();
    else {
        setColor(RED);
        printf("Commande inconnue : '%s'  (tapez 'help')\n", argv[0]);
        setColor(WHITE);
    }
}

/* ─────────────────────────────────────────────
   API publique
   ───────────────────────────────────────────── */

void shell_init(void) {
    reset_cmd();
    setColor(CYAN);
    puts("\n D$-Shell pret. Tapez 'help' pour la liste des commandes.\n");
    setColor(WHITE);
    print_prompt();
}

void shell_handle_key(unsigned char keycode, int is_shift) {
    if (keycode == BACKSPACE) {
        if (cmd_len > 0) {
            backspace();
            cmd[--cmd_len] = '\0';
        }
        return;
    }

    if (keycode == ENTER) {
        putchar('\n');
        cmd[cmd_len] = '\0';
        if (cmd_len > 0)
            execute_command(cmd);
        reset_cmd();
        print_prompt();
        return;
    }

    /* Touche normale (scancodes < 0x80 = touche enfoncée) */
    if (keycode < 128) {
        char c = is_shift ? keymap_maj[keycode] : keymap[keycode];
        if (c && cmd_len < MAX_CMD - 1) {
            putchar(c);
            cmd[cmd_len++] = c;
        }
    }
}
