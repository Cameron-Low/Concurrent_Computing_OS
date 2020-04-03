#include "console.h"

void puts(char* x, int n) {
    for (int i = 0; i < n; i++) {
        PL011_putc(UART1, x[i], true);
    }
}

void gets(char* x, int n) {
    for (int i = 0; i < n; i++) {
        x[i] = PL011_getc(UART1, true);  
        if (x[i] == '\x0A') {
            x[i] = '\x00';
            break;
        }
    }
}

extern void main_P3(); 
extern void main_P4(); 
extern void main_P5(); 
extern void main_dining();

void* load(char* x) {
    if (0 == strcmp(x, "P3")) {
        return &main_P3;
    } else if(0 == strcmp(x, "P4")) {
        return &main_P4;
    } else if(0 == strcmp(x, "P5")) {
        return &main_P5;
    } else if(0 == strcmp(x, "Dining")) {
        return &main_dining;
    } else {
        return NULL;
    }
}

void main_console() {
    while (1) {
        // Get command input
        puts(">> ", 3); 
        char cmd[MAX_CMD_CHARS];
        gets(cmd, MAX_CMD_CHARS);

        // Tokenize the input
        int cmd_argc = 0;
        char* cmd_argv[MAX_CMD_ARGS];

        for (char* t = strtok(cmd, " "); t != NULL; t = strtok(NULL," ")) {
            cmd_argv[cmd_argc++] = t;
        }

        // Handle the command
        if (strcmp(cmd_argv[0], "exec") == 0) {
            void* addr = load(cmd_argv[1]);
            if (addr != NULL) {
                if (fork() == 0) {
                    exec(addr);
                }
            } else {
                puts("Unknown program\n", 16);
            }
        } else if (strcmp(cmd_argv[0], "kill") == 0) {
            kill(atoi(cmd_argv[1]), SIG_TERM);
        } else if (strcmp(cmd_argv[0], "list") == 0) {
            list_procs();
        } else if (strcmp(cmd_argv[0], "touch") == 0) {
            int file = open("test.txt");
            printI(file);
            print("\n");
            write(file, "hello world!\n", 14);
        } else if (strcmp(cmd_argv[0], "cat") == 0) {
            int file = open("test.txt");
            printI(file);
            char txt[16];
            read(file, &txt, 15);
        } else {
            puts("Unknown command\n", 16);
        }
    }
    exit(EXIT_SUCCESS);
}
