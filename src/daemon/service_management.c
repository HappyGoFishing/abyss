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
#include <syslog.h>
#include <errno.h>

#include "defines.h"
#include "util.h"
#include "service.h"
#include "dynamic_service_array.h"


void start_service(struct Service *service, int *child_pipes) {
    if (service == NULL) {
        log_message(LOG_ERR, "error: failed to start a service, service pointer null");
        return;
    }

    pid_t pid = fork();
    char **argv;
    
    if (pid == 0) {
        service->pid = getpid();
        log_message(LOG_INFO, "service: %s PID=%i", service->name, service->pid);
        
        int fd_devnull = open("/dev/null", O_WRONLY);
        
        dup2(fd_devnull, STDOUT_FILENO);
        dup2(fd_devnull, STDERR_FILENO);
        
        close(fd_devnull);

        close(child_pipes[0]);
        write(child_pipes[1], &service->pid, sizeof(service->pid));
        close(child_pipes[1]);

        argv = argv_from_args_string(service->args);
        if (argv == NULL) {
            log_message(LOG_ERR, "error: couldnt start service %s malformed argv", service->name);
            exit(EXIT_FAILURE);
        }

        argv[0] = service->command;

        if (service->working_dir[0] != '\0') {
            if (chdir(service->working_dir) != 0) {
                log_message(LOG_ERR, "error: chdir failed to %s: %s", service->working_dir, strerror(errno));
                exit(EXIT_FAILURE);
            }
        } else {
            log_message(LOG_INFO, "service %s did not specify a working_directory, falling back to parent's", service->name);
        }

        printf("%s\n", service->working_dir);
        execve(service->command, argv, NULL);
        log_message(LOG_ERR, "error: execve %s", strerror(errno));

        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        log_message(LOG_ERR, "error: fork %s", strerror(errno));

    } else {
        close(child_pipes[1]);
        read(child_pipes[0], &service->pid, sizeof(service->pid));
        close(child_pipes[0]);
        
        // argv causes memory leak, eventually should fix it.
    }
}


int stop_service(const char *service_name, struct ServiceArray *sa) {
    int i = find_service_index_by_name(sa, service_name);
    kill(sa->array[i].pid, SIGTERM);
    waitpid(sa->array[i].pid, NULL, 0);
    log_message(LOG_INFO, "stopped service: %s (pid terminated %i)", service_name, sa->array[i].pid);
    
    // placeholder return because eventually i want to return pid's exit status
    return 0;
}


