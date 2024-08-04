#include <string.h>
#include <malloc.h>

#include "../shared/tomlc99/toml.h"
#include "service.h"

struct Service * read_service_toml_file(const char *dirname, const char *filename) {
    if (!dirname || !filename) {
        fprintf(stderr, "filename or dirname is NULL\n");
        return NULL;
    }
    
    size_t dirname_len = strlen(dirname);
    size_t filename_len = strlen(filename);

    if (dirname_len+ filename_len + 6 >= MAX_PATH_LENGTH) {
        fprintf(stderr, "error: path to toml larger than %i\n", MAX_PATH_LENGTH);
        return NULL;
    }

    char path[MAX_PATH_LENGTH];
    
    strncpy(path, dirname, sizeof(path) -1);

    path[sizeof(path) -1] = '\0';
    
    if (dirname[dirname_len - 1] != '/') strncat(path, "/", sizeof(path) - strlen(path) -1);
    

    strncat(path, filename, sizeof(path) - strlen(path) - 1);
    strncat(path, ".toml", sizeof(path) - strlen(path) - 1);


    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "failed to open service file: %s\n", path);
        return NULL;
    }
    printf("found service file: %s\n", path);

    char errbuf[200];
    toml_table_t *toml = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    if (!toml) {
        fprintf(stderr, "%s\n", errbuf);
        return NULL;
    }

    toml_table_t *program = toml_table_in(toml, "program");
    if (!program) {
        fprintf(stderr, "could not find table [program] in %s\n", path);
        toml_free(toml);
        return NULL;
    }

    toml_datum_t command = toml_string_in(program, "command");
    if (!command.ok) {
        fprintf(stderr, "could not find string command in %s\n", path);
        toml_free(toml);
        return NULL;
    }
    
    toml_datum_t args = toml_string_in(program, "args");
    if (!args.ok) {
        fprintf(stderr, "could not find string args in %s\n", path);
        free(command.u.s);
        toml_free(toml);
        return NULL;
    }

    struct Service *service = malloc(sizeof(struct Service));

    strncpy(service->command, command.u.s, sizeof(service->command) - 1);
    service->command[sizeof(service->command) - 1] = '\0';
    
    strncpy(service->args, args.u.s, sizeof(service->args) - 1);
    service->args[sizeof(service->args) - 1] = '\0';
    
    free(command.u.s);
    free(args.u.s);
    toml_free(toml);

    return service;
}
