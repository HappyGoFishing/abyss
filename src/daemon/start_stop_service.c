#include "service.h"
#include "../shared/util.h"
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

char ** argv_from_args_string(const char * args_str) {
    char ** argv = (char **)malloc((MAX_SERVICE_ARGS_LENGTH + 1) * sizeof(char *));
    char arg_str_copy[MAX_SERVICE_ARGS_LENGTH];
    strncpy(arg_str_copy, args_str, sizeof(arg_str_copy));
    arg_str_copy[sizeof(arg_str_copy) - 1] = '\0';
    
    char *token;
    int argc = 1;
    token = strtok(arg_str_copy, " ");
    while (token != NULL && argc <= MAX_ARGS) {
        argv[argc] = (char *)malloc(strlen(token) + 1);
        strcpy(argv[argc], token);
        argc++;
        token = strtok(NULL, " ");
    }
    argv[argc] = NULL;
    return argv;
}

void start_service(struct Service *service, int *child_pipefds) {
    pid_t pid = fork();
    if (pid == 0) {
        service->pid = getpid();
        printf("PID of service %s is: %i\n", service->name, service->pid);
        
        int stdout_redirect = open("/dev/null", O_WRONLY);
        int stderr_redirect = open("/dev/null", O_WRONLY);
        
        dup2(stdout_redirect, STDOUT_FILENO);
        dup2(stderr_redirect, STDERR_FILENO);
        
        close(stdout_redirect);
        close(stderr_redirect);

        close(child_pipefds[0]);
        write(child_pipefds[1], &service->pid, sizeof(service->pid));
        close(child_pipefds[1]);

        char **argv = argv_from_args_string(service->args);
        argv[0] = service->command;

        execve(service->command, argv, NULL);
        perror("execl");

        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork");
    } else {
        close(child_pipefds[1]);

        read(child_pipefds[0], &service->pid, sizeof(service->pid));
        close(child_pipefds[0]);
    }
}

int stop_service(const char *service_name, struct ServiceArray *sa) {
    int i = find_service_index_by_name(sa, service_name);
    printf("terminating PID: %i\n", sa->array[i].pid);
    kill(sa->array[i].pid, SIGTERM);
    waitpid(sa->array[i].pid, NULL, 0);
    printf("stopped service: %s (pid terminated %i)\n", service_name, sa->array[i].pid);
    return 0; // the return code of the pid
}