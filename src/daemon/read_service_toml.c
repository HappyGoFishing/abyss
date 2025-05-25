#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include "../vendor/tomlc99/toml.h"

#include "daemon.h"

struct Service *read_service_toml_file(const char *dirname, const char *filename) {
    if (!dirname || !filename) {
        log_message(LOG_ERR, "filename or dirname is NULL");
        return NULL;
    }
    
    size_t dirname_len = strlen(dirname);
    size_t filename_len = strlen(filename);
    // 6 because ".toml/\0" = 6 chars
    if (dirname_len + filename_len + 6 >= MAX_PATH_LENGTH) {
        log_message(LOG_ERR, "error: path to toml larger than %i", MAX_PATH_LENGTH);
        return NULL;
    }

    char path[MAX_PATH_LENGTH];
    strncpy(path, dirname, sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';
    
    if (dirname[dirname_len - 1] != '/') {
        strncat(path, "/", sizeof(path) - strlen(path) - 1);
    }

    strncat(path, filename, sizeof(path) - strlen(path) - 1);
    strncat(path, ".toml", sizeof(path) - strlen(path) - 1);

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        log_message(LOG_ERR, "failed to open service file: %s", path);
        return NULL;
    }

    log_message(LOG_INFO, "found service file: %s", path);

    char errbuf[200];
    toml_table_t *toml = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);
    if (!toml) {
        log_message(LOG_ERR, "%s", errbuf);
        return NULL;
    }
    // [program] table
    toml_table_t *program = toml_table_in(toml, "program");
    if (!program) {
        log_message(LOG_ERR, "could not find table [program] in %s", path);
        goto cleanup_toml;
    }

    toml_datum_t command = toml_string_in(program, "command");
    if (!command.ok) {
        log_message(LOG_ERR, "could not find string command in %s", path);
        goto cleanup_toml;
    }

    toml_datum_t args = toml_string_in(program, "args");
    if (!args.ok) {
        log_message(LOG_ERR, "could not find string args in %s", path);
        goto cleanup_command;
    }
    // [environment] table
    toml_table_t *environment = toml_table_in(toml, "environment");
    if (!environment) {
        log_message(LOG_ERR, "could not find table [environment] in %s", path);
        goto cleanup_command;
    }

    // we dont error check this because if it fails it doesnt matter, (more info below).
    toml_datum_t working_dir = toml_string_in(environment, "working_dir");
    
    struct Service *service = malloc(sizeof(struct Service));
    if (!service) {
        log_message(LOG_ERR, "failed to allocate memory for service");
        goto cleanup_args;
    }

    strncpy(service->command, command.u.s, sizeof(service->command) - 1);
    service->command[sizeof(service->command) - 1] = '\0';
    
    strncpy(service->args, args.u.s, sizeof(service->args) - 1);
    service->args[sizeof(service->args) - 1] = '\0';

    /* if working_dir does not exist in service config we set service->working_dir[0] to '\0'
       to indicate to start_service() to skip calling chdir before execve */
    if (!working_dir.ok) {
        service->working_dir[0] = '\0';
    } else {
        strncpy(service->working_dir, working_dir.u.s, sizeof(service->working_dir) -1);
        service->working_dir[sizeof(service->working_dir) -1] = '\0';
    }
    free(command.u.s);
    free(args.u.s);
    free(working_dir.u.s);
    toml_free(toml);

    return service;

cleanup_args:
    
    free(args.u.s);
    return NULL;
cleanup_command:
    free(command.u.s);
    return NULL;
cleanup_toml:
    toml_free(toml);
    return NULL;
}

