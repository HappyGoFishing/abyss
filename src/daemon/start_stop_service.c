#include "service.h"

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

void start_service(struct Service *service, int *child_pipefds) {
    pid_t pid = fork();
    if (pid == 0) {
        service->pid = getpid();
        printf("PID of service %s is: %i\n", service->name, service->pid);
        
        int devnull = open("/dev/null", O_WRONLY);
        dup2(devnull, STDOUT_FILENO);

        close(child_pipefds[0]);

        write(child_pipefds[1], &service->pid, sizeof(service->pid));
        close(child_pipefds[1]);

        execl(service->command, service->args, NULL);
        
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

void stop_service(const char *service_name, struct ServiceArray *sa) {
    int i = find_service_index_by_name(sa, service_name);
    if (i == -1) {
        printf("couldnt stop service: %s (service not running)\n", service_name);
        return;
    }
    printf("terminating PID: %i\n", sa->array[i].pid);
    kill(sa->array[i].pid, SIGTERM);
    printf("stopped service: %s (pid terminated %i)\n", service_name, sa->array[i].pid);
}