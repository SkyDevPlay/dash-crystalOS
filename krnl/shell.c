#include "shell.h"
#include "sys/keymap.h"
#include "sys/io.h"
#include "io.h"
#include "fat.h"
#include "string.h"
#include "malloc.h"

static char cmd[MAX_CMD];
static int  cmd_len = 0;


static void print_prompt(void) {
    setColor(CYAN);
    printf("D$>> ");
    setColor(WHITE);
}

static void reset_cmd(void) {
    for (int i = 0; i < MAX_CMD; i++) cmd[i] = 0;
    cmd_len = 0;
}

static int split_args(char *input, char *argv[], int max_args) {
    int argc = 0;
    char *p = input;

    while (*p && argc < max_args) {
        while (*p == ' ') p++;
        if (!*p) break;

        argv[argc++] = p;

        while (*p && *p != ' ') p++;
        if (*p == ' ') *p++ = '\0';
    }
    return argc;
}
/*              
            COMMANDES                   
                                            */
/* _help */
static void cmd_help(void) {
    setColor(GREEN);
    puts("List of initialized commands :");
    setColor(RED);
    puts("WARNING : Commands initialize themselves by having a underscore at first!");
    setColor(BLUE);
    puts(" _help              Shows this message");
    puts(" _ls                List the files in the directory");
    puts(" _var               Information about Dash Crystal OS version");
    puts(" _sw <file>         Shows the content of a file");
    puts(" _wrt <file> <txt>  Write text in a file");
    puts(" _echo <texte>      Puts text on screen");
    puts(" _clr               Clear the screen");
}
/* _ver */
static void cmd_ver(void) {
    setColor(CYAN);  
    setColor(WHITE);
}

/* _ls */
static void cmd_ls(void) {
    setColor(YELLOW);
    list_dir();
    setColor(WHITE);
}

/* _sw <file>  */
static void cmd_sw(int argc, char *argv[]) {
    if (argc < 2) {
        setColor(RED);
        puts("Usage : _sw <file>");
        setColor(WHITE);
        return;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) {
        setColor(RED);
        printf("sw : cannot open '%s'\n", argv[1]);
        setColor(WHITE);
        return;
    }

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
 * wrt <file> <txt>
 *   Écrase le fichier avec le texte fourni (le fichier doit déjà
 *   exister dans la FAT — la création de nouvelle entrée n'est pas
 *   encore implémentée dans fat.c).
 */
static void cmd_wrt(int argc, char *argv[]) {
    if (argc < 3) {
        setColor(RED);
        puts("Usage : _wrt <file> <txt>");
        setColor(WHITE);
        return;
    }

    FILE *f = fopen(argv[1], "w");
    if (!f) {
        setColor(RED);
        printf("wrt : cannot open '%s'\n", argv[1]);
        setColor(WHITE);
        return;
    }

    fseek(f, 0, SEEK_SET);
    for (int i = 2; i < argc; i++) {
        fwrite(argv[i], 1, strlen(argv[i]), f);
        if (i + 1 < argc) fwrite(" ", 1, 1, f);
    }

    fclose(f);
    setColor(GREEN);
    printf("Writing in '%s'.\n", argv[1]);
    setColor(WHITE);
}

/* _echo <txt> */
static void cmd_echo(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        printf("%s", argv[i]);
        if (i + 1 < argc) putchar(' ');
    }
    putchar('\n');
}

/* clr -> CLEAR */
static void cmd_clr(void) {
    clearScreen();
}

static void execute_command(char *input) {
    static char buf[MAX_CMD];
    strncpy(buf, input, MAX_CMD - 1);
    buf[MAX_CMD - 1] = '\0';

    char *argv[MAX_ARGS];
    int   argc = split_args(buf, argv, MAX_ARGS);

    if (argc == 0) return;

    if      (strcmp(argv[0], "_help")  == 0) cmd_help();
    else if (strcmp(argv[0], "_ls")    == 0) cmd_ls();
    else if (strcmp(argv[0], "_ver")    == 0) cmd_ver();
    else if (strcmp(argv[0], "_sw")   == 0) cmd_sw(argc, argv);
    else if (strcmp(argv[0], "_wrt") == 0) cmd_wrt(argc, argv);
    else if (strcmp(argv[0], "_echo")  == 0) cmd_echo(argc, argv);
    else if (strcmp(argv[0], "_clr") == 0) cmd_clr();
    else {
        setColor(RED);
        printf("Unknown command: '%s'  (use '_help')\n", argv[0]);
        setColor(WHITE);
    }
}

void shell_init(void) {
    reset_cmd();
    setColor(CYAN);
    puts("D$-Shell initialized. Begin with '_help' to understand the command usage");
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

    if (keycode < 128) {
        char c = is_shift ? keymap_maj[keycode] : keymap[keycode];
        if (c && cmd_len < MAX_CMD - 1) {
            putchar(c);
            cmd[cmd_len++] = c;
        }
    }
}
