#include "service.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "../shared/tomlc99/toml.h"

struct Service read_service_toml_file(const char *dirname, const char *filename) {
    struct Service service = {.command = "", .args = "", .ok = false};
    if (!dirname || !filename) {
        fprintf(stderr, "filename or dirname is NULL\n");
        return service;
    }
    char *path = malloc(strlen(dirname) + strlen(filename) + 2);
    strcpy(path, dirname);

    if (dirname[strlen(dirname) - 1] != '/')
        strcat(path, "/");

    strcat(path, filename);
    strcat(path, ".toml");

    printf("trying to open: %s\n", path);

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "could not open service file: %s\n", path);
        return service;
    }
    printf("opened service file: %s\n", path);
    char errbuf[200];
    toml_table_t *service_toml = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    if (!service_toml) {
        fprintf(stderr, "%s\n", errbuf);
        return service;
    }

    toml_table_t *program = toml_table_in(service_toml, "program");
    if (!program) {
        fprintf(stderr, "could not find table [program] in %s\n", path);
        return service;
    }
    toml_datum_t command = toml_string_in(program, "command");
    if (!command.ok) {
        fprintf(stderr, "could not find string command in %s\n", path);
        return service;
    }
    toml_datum_t args = toml_string_in(program, "args");
    if (!command.ok) {
        fprintf(stderr, "could not find string args in %s\n", path);
        return service;
    }
    strcpy(service.command, command.u.s);
    strcpy(service.args, args.u.s);
    service.ok = true;
    free(path);
    free(command.u.s);
    free(args.u.s);
    return service;
}

void start_service(struct Service service) {
    pid_t pid = fork();
    if (!pid) {
        service.pid = getpid();
        printf("pid of service: %i\n", service.pid);
        execl(service.command, service.args, NULL);
    }
}
