#include "service.h"

#include <iomanip>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../shared/tomlc99/toml.h"

struct ServiceData read_service_toml_file(const char *dirname, const char *filename) {
    struct ServiceData data = {.command = "", .args = "", .ok = false};
    if (!dirname || !filename) {
        fprintf(stderr, "filename or dirname is NULL\n");
        return data;
    }
    char *path = malloc(strlen(dirname) + strlen(filename) + 2);
    strcpy(path, dirname);

    if (dirname[strlen(dirname) - 1] != '/')
        strcat(path, "/");

    strcat(path, filename);
    strcat(path, ".toml");

    printf("toml: %s\n", path);

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "could not open %s\n", path);
        return data;
    }
    char errbuf[200];
    toml_table_t *service_toml = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    if (!service_toml) {
        fprintf(stderr, "%s\n", errbuf);
        return data;
    }

    toml_table_t *program = toml_table_in(service_toml, "program");
    if (!program) {
        fprintf(stderr, "could not find table [program] in %s\n", path);
        return data;
    }
    toml_datum_t command = toml_string_in(program, "command");
    if (!command.ok) {
        fprintf(stderr, "could not find string command in %s\n", path);
        return data;
    }
    toml_datum_t args = toml_string_in(program, "args");
    if (!command.ok) {
        fprintf(stderr, "could not find string args in %s\n", path);
        return data;
    }
    strcpy(data.command, command.u.s);
    strcpy(data.args, args.u.s);
    data.ok = true;
    free(path);
    free(command.u.s);
    free(args.u.s);
    return data;
}

void start_service(struct ServiceData service_data) {
    pid_t pid = fork();
    if (!pid) {
        execl(service_data.command, service_data.args, NULL);
    }
}
