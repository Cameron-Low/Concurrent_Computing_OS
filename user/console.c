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

char cwd[1024];
void main_console() {
    strcpy(cwd, "/");
    while (1) {
        // Get command input
        print(cwd);
        print(" >> "); 
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
                print("Unknown program\n");
            }
        } else if (strcmp(cmd_argv[0], "kill") == 0) {
            kill(atoi(cmd_argv[1]), SIG_TERM);
        } else if (strcmp(cmd_argv[0], "list") == 0) {
            list_procs();
        } else if (strcmp(cmd_argv[0], "touch") == 0) {
            int file = open(cmd_argv[1]);
            printI(file);
            print("\n");
            close(file);
        } else if (strcmp(cmd_argv[0], "cat") == 0) {
            int file = open(cmd_argv[1]);
            print("\n");
            char txt[16];
            read(file, &txt, 15);
            close(file);
            print(txt);
            print("\n");
        } else if (strcmp(cmd_argv[0], "concat") == 0) {
            int file = open(cmd_argv[1]);
            printI(file);
            print("\n");
            write(file, cmd_argv[2], strlen(cmd_argv[2]) + 1);
            close(file);
        } else if (strcmp(cmd_argv[0], "rm") == 0) {
            remove(cmd_argv[1]);
        } else if (strcmp(cmd_argv[0], "mkdir") == 0) {
            mkdir(cmd_argv[1]);
        } else if (strcmp(cmd_argv[0], "rmdir") == 0) {
            rmdir(cmd_argv[1]);
        } else if (strcmp(cmd_argv[0], "cd") == 0) {
            chdir(cmd_argv[1]);
            strcpy(cwd, getcwd());
        } else {
            print("Unknown command\n");
        }
    }
    exit(EXIT_SUCCESS);
}
