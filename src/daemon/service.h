#pragma once
#include <stdbool.h>
#include <fcntl.h>
#include <stddef.h>

#include "constant_defines.h"

// result codes returned from functions to make error checking less arcane
#define RESULT_SERVICE_NOT_IN_ARRAY -1
#define RESULT_SERVICE_ARRAY_REACHED_LIMIT -2
#define RESULT_SERVICE_ALREADY_IN_ARRAY -3
#define RESULT_SUCCESS 0


struct Service {
    char command[MAX_SERVICE_COMMAND_LENGTH];
    char args[MAX_SERVICE_ARGS_LENGTH];
    char name[MAX_SERVICE_NAME_LENGTH];
    char working_directory[MAX_PATH_LENGTH];
    pid_t pid; 
};

struct ServiceArray {
    struct Service array[MAX_SERVICE_ARRAY_SIZE];
    size_t size;
};

int find_service_index_by_name(struct ServiceArray *sa, const char *service_name);

int add_service_to_array(struct ServiceArray *sa, struct Service service);

int remove_service_from_array(struct ServiceArray *sa, const char *service_name);

struct Service * read_service_toml_file(const char* dirname, const char* filename);

void start_service(struct Service *service, int *child_pipeds);

int stop_service(const char *service_name, struct ServiceArray *sa);
