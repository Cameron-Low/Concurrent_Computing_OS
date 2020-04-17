#include "console.h"

void gets(char* x, int n) {
    for (int i = 0; i < n; i++) {
        x[i] = PL011_getc(UART1, true);  
        if (x[i] == '\x0A') {
            x[i] = '\x00';
            break;
        }
    }
}

void* loader(char* x) {
    if (0 == strcmp(x, "P3")) {
        return &main_P3;
    } else if (0 == strcmp(x, "P4")) {
        return &main_P4;
    } else if (0 == strcmp(x, "P5")) {
        return &main_P5;
    } else if (0 == strcmp(x, "Dining")) {
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
        char* cmd_argv[MAX_CMD_ARGS] = {'\0'};

        for (char* t = strtok(cmd, " "); t != NULL; t = strtok(NULL," ")) {
            cmd_argv[cmd_argc++] = t;
        }

        // Handle the command
        if (argc == 1) {
            if (strcmp(cmd_argv[0], "help") == 0) {
                print("List of commands: \n");
                print("\texec {PROGRAM} - execute a user process\n");
                print("\tkill {PID} - terminate a process\n");
                print("\tlist - list all currently running processes\n");
                print("\ttouch {FILEPATH} - creates a file at the given path\n");
                print("\tcat {FILEPATH} - prints contents of file\n");
                print("\tconcat {FILEPATH} {WORD} - writes a word to file\n");
                print("\trm {FILEPATH} - deletes a file from disk\n");
                print("\tmkdir {DIRPATH} - creates a new directory at the specified path\n");
                print("\trmdir {DIRPATH} - deletes an empty directory from disk\n");
                print("\tcd {DIRPATH} - change the current directory\n");
                print("\tls {DIRPATH} - prints the contents of the given directory\n");
            } else if (strcmp(cmd_argv[0], "list") == 0) {
                list_procs();
            } else {
                print("Unknown command\n");
                print("Enter 'help' for a list of commands\n");
            }
        } else if (argc == 2) {
            if (strcmp(cmd_argv[0], "exec") == 0) {
                void* addr = loader(cmd_argv[1]);
                if (addr != NULL) {
                    if (fork() == 0) {
                        exec(addr);
                    }
                } else {
                    print("Unknown program\n");
                    print("List of available user programs: \n");
                    print("\tP3 - Looping program that does basic bit arithmetic on some numbers");
                    print("\tP4 - Looping program that calculates the gcd of some numbers, includes recursion\n");
                    print("\tP5 - Terminating program that calculates which numbers are prime betweem to values\n");
                    print("\tDining - dining philosophers example program\n");
                }
            } else if (strcmp(cmd_argv[0], "kill") == 0) {
                kill(atoi(cmd_argv[1]), SIG_TERM);
            } else if (strcmp(cmd_argv[0], "touch") == 0) {
                int file = open(cmd_argv[1]);
                close(file);
            } else if (strcmp(cmd_argv[0], "cat") == 0) {
                int file = open(cmd_argv[1]);
                char txt[16];
                read(file, &txt, 15);
                close(file);
                print(txt);
                print("\n");
            } else if (strcmp(cmd_argv[0], "rm") == 0) {
                remove(cmd_argv[1]);
            } else if (strcmp(cmd_argv[0], "mkdir") == 0) {
                mkdir(cmd_argv[1]);
            } else if (strcmp(cmd_argv[0], "rmdir") == 0) {
                rmdir(cmd_argv[1]);
            } else if (strcmp(cmd_argv[0], "cd") == 0) {
                chdir(cmd_argv[1]);
                strcpy(cwd, getcwd());
            } else if (strcmp(cmd_argv[0], "ls") == 0) {
                listdir(cmd_argv[1]);
            } else {
                print("Unknown command\n");
                print("Enter 'help' for a list of commands\n");
            }
        } else {
            if (strcmp(cmd_argv[0], "concat") == 0) {
                int file = open(cmd_argv[1]);
                write(file, cmd_argv[2], strlen(cmd_argv[2]) + 1);
                close(file);
            } else {
                print("Unknown command\n");
                print("Enter 'help' for a list of commands\n");
            }
        }
    }
    exit(EXIT_SUCCESS);
}
