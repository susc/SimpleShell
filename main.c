#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>

#define MAX_CMD_SIZE 128
#define MAX_ARG_SIZE 64

char* trim(char* str) {
    char* p = str;

    while (isblank(*p)) {
        ++p;
    }

    while (isblank(p[strlen(p) - 1])) {
        p[strlen(p) - 1] = '\0';
    }

    return p;
}

// return 1 for success; 0 for failure.
int parse_pipe(char* cmd, char* pipe_left, char* pipe_right) {
    int i;

    for (i = strlen(cmd) - 1; i > 0; i--) {
        if (cmd[i] == '|') {
            break;
        }
    }

    if (i > 0) {
        strncpy(pipe_left, cmd, i);
        pipe_left[i] = '\0';
        strncpy(pipe_right, cmd + i + 1, strlen(cmd) - i - 1);
        pipe_right[strlen(cmd) - i - 1] = '\0';
        return 1;
    }
    return 0;
}

void parse_argv(char* cmd, char** argv, int* argc) {
    int i = 0;

    *argc = 0;
    for (; cmd[i] != '\0'; ++i) {
        if (!isblank(cmd[i])) {
            if (i == 0 || isblank(cmd[i - 1]) || cmd[i - 1] == '\0') {
                argv[*argc] = cmd + i;
                (*argc)++;
                continue;
            }
        }
        else {
            if (i != 0) {
                if (!isblank(cmd[i - 1])) {
                    cmd[i] = '\0';
                    continue;
                }
            }
        }
    }

    argv[*argc] = NULL;
}

void execute_cmd(char* cmd) {
    char pipe_left[MAX_CMD_SIZE], pipe_right[MAX_CMD_SIZE];

    cmd = trim(cmd);

    // check for empty command
    if (strlen(cmd) == 0) {
        return;
    }

    // DEBUG: print current command
    // printf("Executing command: %s\n", cmd);

    if (parse_pipe(cmd, pipe_left, pipe_right)) {
        int simple_pipe[2];
        pid_t pid;

        // create a pipe
        if (pipe(simple_pipe) != 0) {
            perror("Error");
            exit(1);
        }

        // create child processes to execute the command
        // left
        pid = fork();
        if (pid < 0) {
            perror("Error");
            exit(0);
        }
        if (pid == 0) {
            dup2(simple_pipe[1], 1);
            close(simple_pipe[0]);
            close(simple_pipe[1]);
            execute_cmd(pipe_left);
            exit(0);
        }

        // right
        pid = fork();
        if (pid < 0) {
            perror("Error");
            exit(0);
        }
        if (pid == 0) {
            dup2(simple_pipe[0], 0);
            close(simple_pipe[0]);
            close(simple_pipe[1]);
            execute_cmd(pipe_right);
            exit(0);
        }

        close(simple_pipe[0]);
        close(simple_pipe[1]);
        // wait 2 child processes to stop
        wait(NULL);
        wait(NULL);

        return;
    }

    if(strcmp(cmd, "exit") == 0) {
        exit(0);
    }

    if (strcmp(cmd, "help") == 0) {
        printf("Usage: just type your command whatever you want and then press enter to execute.\n");
        printf("\nPipe: use \"|\" for pipe. For example: ls | grep main\n");
        printf("\nBuilt-in commands:\n");
        printf("\thelp: show how to use this shell.\n");
        printf("\texit: exit this shell.\n");
        return;
    }

    // parse args
    char** argv = (char **)malloc(sizeof (char*) * MAX_ARG_SIZE);
    int argc;
    parse_argv(cmd, argv, &argc);
    if (argc > MAX_ARG_SIZE) {
        printf("Error: too many arguments!\n");
        exit(1);
    }

    // change directory
    if (strcmp(argv[0], "cd") == 0) {
        if (argc <= 1) {
            printf("Error: please provide target directory!\n");
        }
        if (chdir(argv[1]) != 0) {
            perror("Error");
        }
        return;
    }

    // create a new process to execute command
    pid_t pid;
    pid = fork();
    if (pid < 0) {
        printf("Error: can not create a new process!\n");
        exit(1);
    }
    else if (pid == 0) {
        // exec
        if (execvp(argv[0], argv) == -1) {
            perror("Error");
        }
        exit(1);
    }
    else {
        wait(NULL);
    }
}

_Noreturn void get_cmd_loop() {
    char cmd[MAX_CMD_SIZE];

    while (1) {
        printf("> ");
        fgets(cmd, MAX_CMD_SIZE, stdin);

        // check command size
        if (strlen(cmd) == MAX_CMD_SIZE - 1)
            if (cmd[MAX_CMD_SIZE - 2] != '\n') {
                printf("Error: your command is too long!\n");
                // cut off remain input
                fflush(stdin);
                continue;
            }

        // remove \n at the last position
        if (cmd[strlen(cmd) - 1] == '\n') {
            cmd[strlen(cmd) - 1] = '\0';
        }

        // execute command
        execute_cmd(cmd);
    }
}

int main() {
    system("clear");
    get_cmd_loop();
}
