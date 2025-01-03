#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 100

void execute_command(char *cmd);
void parse_and_execute(char *input);
void execute_pipeline(char **commands, int num_commands);
void change_directory(char *path);

int main() {
    char input[MAX_COMMAND_LENGTH];

    printf("Custom Shell Interpreter\n");
    printf("Type 'exit' to exit the shell.\n");

    while (1) {
        printf("shell> ");
        if (fgets(input, MAX_COMMAND_LENGTH, stdin) == NULL) {
            perror("fgets failed");
            continue;
        }

        // Remove the trailing newline character
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "exit") == 0) {
            break;
        }

        parse_and_execute(input);
    }

    return 0;
}

void parse_and_execute(char *input) {
    char *commands[MAX_ARGS];
    int num_commands = 0;
    char *token = strtok(input, "|");

    // Split the input into commands using the pipe "|" delimiter
    while (token != NULL) {
        commands[num_commands++] = token;
        token = strtok(NULL, "|");
    }

    if (num_commands > 1) {
        // Handle pipelined commands
        execute_pipeline(commands, num_commands);
    } else {
        // Handle single command
        execute_command(commands[0]);
    }
}

void execute_command(char *cmd) {
    char *args[MAX_ARGS];
    char *token;
    int arg_count = 0;

    // Parse the command into arguments
    token = strtok(cmd, " ");
    while (token != NULL) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL;

    // Handle built-in commands
    if (strcmp(args[0], "cd") == 0) {
        change_directory(args[1]);
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
    } else if (pid == 0) {
        // Child process
        execvp(args[0], args);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        wait(NULL);
    }
}

void execute_pipeline(char **commands, int num_commands) {
    int pipefds[2];
    int prev_fd = -1;

    for (int i = 0; i < num_commands; i++) {
        pipe(pipefds);

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            return;
        } else if (pid == 0) {
            // Child process
            if (prev_fd != -1) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }
            if (i < num_commands - 1) {
                dup2(pipefds[1], STDOUT_FILENO);
            }
            close(pipefds[0]);
            close(pipefds[1]);

            execute_command(commands[i]);
            exit(EXIT_SUCCESS);
        } else {
            // Parent process
            wait(NULL);
            close(pipefds[1]);
            if (prev_fd != -1) {
                close(prev_fd);
            }
            prev_fd = pipefds[0];
        }
    }
}

void change_directory(char *path) {
    if (path == NULL) {
        fprintf(stderr, "cd: missing argument\n");
        return;
    }
    if (chdir(path) < 0) {
        perror("cd failed");
    }
}
