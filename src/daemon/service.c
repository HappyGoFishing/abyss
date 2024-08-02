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

#include "../shared/tomlc99/toml.h"

int find_service_index_by_name(struct ServiceArray *array, const char *service_name) {
    for (size_t i = 0; i < array->size; i++) {
        if (!strcmp(service_name, array->array[i].name)) {
            return i;
        }
    }
    return -1;
}

int add_service_to_array(struct ServiceArray *array, struct Service service) {
    if (array->size >= MAX_SERVICE_ARRAY_SIZE) {
        printf("cant add service: %s (reached MAX_SERVICE_ARRAY_SIZE)\n", service.name);
        return -2;
    }
    if (find_service_index_by_name(array, service.name) != -1) {
        return -1;
    }
    array->array[array->size] = service;
    array->size++;
    return 0;
}

void remove_service_from_array(struct ServiceArray *array, const char *service_name) {
    int index = find_service_index_by_name(array, service_name);
    if (index != -1) {
        for (size_t i = index; i < array->size -1; i++) {
            array->array[i] = array->array[i + 1];
        }
        array->size--;
    } else {
        printf("cant remove service: %s (service not found\n)", service_name);
    }
}

struct Service read_service_toml_file(const char *dirname, const char *filename) {
    struct Service service = {.command = "", .args = "", .ok = false};
    if (!dirname || !filename) {
        fprintf(stderr, "filename or dirname is NULL\n");
        return service;
    }

    char *path = malloc(strlen(dirname) + strlen(filename) + 2);
    if (!path) {
        fprintf(stderr, "memory allocation failed\n");
        return service;
    }
    strcpy(path, dirname);

    if (dirname[strlen(dirname) - 1] != '/')
        strcat(path, "/");

    strcat(path, filename);
    strcat(path, ".toml");

    printf("trying to open service file: %s\n", path);

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "could not open service file: %s\n", path);
        free(path);
        return service;
    }
    printf("found service file: %s\n", path);

    char errbuf[200];
    toml_table_t *service_toml = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    if (!service_toml) {
        fprintf(stderr, "%s\n", errbuf);
        free(path);
        return service;
    }

    toml_table_t *program = toml_table_in(service_toml, "program");
    if (!program) {
        fprintf(stderr, "could not find table [program] in %s\n", path);
        toml_free(service_toml);
        free(path);
        return service;
    }

    toml_datum_t command = toml_string_in(program, "command");
    if (!command.ok) {
        fprintf(stderr, "could not find string command in %s\n", path);
        toml_free(service_toml);
        free(path);
        return service;
    }

    toml_datum_t args = toml_string_in(program, "args");
    if (!args.ok) {
        fprintf(stderr, "could not find string args in %s\n", path);
        free(command.u.s);
        toml_free(service_toml);
        free(path);
        return service;
    }

    strcpy(service.command, command.u.s);
    strcpy(service.args, args.u.s);
    service.ok = true;

    free(path);
    free(command.u.s);
    free(args.u.s);
    toml_free(service_toml);

    return service;
}

void start_service(struct Service service) {
    pid_t pid = fork();
    if (pid == 0) {
        service.pid = getpid();
        printf("PID of service %s is: %i\n", service.name, service.pid);
        //placeholder devnull until toml file specifies stdout and stderr redirect
        int devnull = open("/dev/null", O_WRONLY);
        dup2(devnull, STDOUT_FILENO);

        execl(service.command, service.args, NULL);
        
        perror("execl");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork");
    }
}

void stop_service(const char *service_name, struct ServiceArray *array) {
    int i = find_service_index_by_name(array, service_name);
    if (i == -1) {
        printf("couldnt stop service: %s (service not running)\n", service_name);
        return;
    }
    kill(array->array[i].pid, SIGTERM);
    printf("stopped service: %s (pid terminated %i)\n", service_name, array->array[i].pid);
}