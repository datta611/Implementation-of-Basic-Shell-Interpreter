#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 100
#define MAX_HISTORY 50

typedef struct Job {
    pid_t pid;
    char command[MAX_COMMAND_LENGTH];
    int is_active;
} Job;

Job jobs[MAX_HISTORY];
int job_count = 0;
char history[MAX_HISTORY][MAX_COMMAND_LENGTH];
int history_count = 0;

void handle_signal(int signal);
void add_to_history(const char *command);
void display_history();
void execute_command(char *cmd);
void execute_pipeline(char **commands, int num_commands);
void handle_redirection(char *cmd);
void change_directory(char *path);
void handle_jobs();
void kill_job(int job_id);
void add_job(pid_t pid, const char *command);
void mark_job_completed(pid_t pid);

int main() {
    char input[MAX_COMMAND_LENGTH];

    signal(SIGINT, handle_signal);
    signal(SIGTSTP, handle_signal);

    printf("Enhanced Shell Interpreter\n");
    printf("Type 'exit' to quit.\n");

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

        if (strlen(input) > 0) {
            add_to_history(input);
            execute_command(input);
        }
    }

    return 0;
}

void handle_signal(int signal) {
    if (signal == SIGINT) {
        printf("\nCtrl+C pressed. Use 'exit' to quit the shell.\n");
    } else if (signal == SIGTSTP) {
        printf("\nCtrl+Z pressed. Background jobs are managed automatically.\n");
    }
}

void add_to_history(const char *command) {
    if (history_count < MAX_HISTORY) {
        strcpy(history[history_count++], command);
    } else {
        // Shift history when full
        for (int i = 1; i < MAX_HISTORY; i++) {
            strcpy(history[i - 1], history[i]);
        }
        strcpy(history[MAX_HISTORY - 1], command);
    }
}

void display_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d: %s\n", i + 1, history[i]);
    }
}

void execute_command(char *cmd) {
    char *args[MAX_ARGS];
    char *token;
    int arg_count = 0;
    int background = 0;

    // Check for background execution
    if (cmd[strlen(cmd) - 1] == '&') {
        background = 1;
        cmd[strlen(cmd) - 1] = '\0';
    }

    // Parse the command into arguments
    token = strtok(cmd, " ");
    while (token != NULL) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL;

    if (arg_count == 0) return;

    // Handle built-in commands
    if (strcmp(args[0], "cd") == 0) {
        change_directory(args[1]);
        return;
    } else if (strcmp(args[0], "history") == 0) {
        display_history();
        return;
    } else if (strcmp(args[0], "jobs") == 0) {
        handle_jobs();
        return;
    } else if (strcmp(args[0], "kill") == 0 && arg_count == 2) {
        kill_job(atoi(args[1]));
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
    } else if (pid == 0) {
        // Child process
        handle_redirection(cmd);
        execvp(args[0], args);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        if (background) {
            printf("Started background job: %d\n", pid);
            add_job(pid, cmd);
        } else {
            waitpid(pid, NULL, 0);
        }
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

void handle_redirection(char *cmd) {
    char *in_file = strstr(cmd, "<");
    char *out_file = strstr(cmd, ">");
    if (in_file) {
        *in_file = '\0';
        in_file = strtok(in_file + 1, " ");
        int fd = open(in_file, O_RDONLY);
        if (fd < 0) perror("Input redirection failed");
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    if (out_file) {
        *out_file = '\0';
        out_file = strtok(out_file + 1, " ");
        int fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) perror("Output redirection failed");
        dup2(fd, STDOUT_FILENO);
        close(fd);
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

void handle_jobs() {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].is_active) {
            printf("[%d] %d %s\n", i + 1, jobs[i].pid, jobs[i].command);
        }
    }
}

void kill_job(int job_id) {
    if (job_id <= 0 || job_id > job_count || !jobs[job_id - 1].is_active) {
        fprintf(stderr, "Invalid job ID\n");
        return;
    }
    kill(jobs[job_id - 1].pid, SIGKILL);
    jobs[job_id - 1].is_active = 0;
}

void add_job(pid_t pid, const char *command) {
    if (job_count < MAX_HISTORY) {
        jobs[job_count].pid = pid;
        strcpy(jobs[job_count].command, command);
        jobs[job_count].is_active = 1;
        job_count++;
    }
}

void mark_job_completed(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) {
            jobs[i].is_active = 0;
            break;
        }
    }
}
