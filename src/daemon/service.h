#pragma once
#include <stdbool.h>
#include <fcntl.h>
#include <stddef.h>

#define SERVICE_PATH "./services"
#define MAX_PATH_LENGTH 1024
#define MAX_SERVICE_ARRAY_SIZE 128 
#define MAX_SERVICE_COMMAND_LENGTH 128
#define MAX_SERVICE_ARGS_LENGTH 512
#define MAX_SERVICE_NAME_LENGTH 64

struct Service {
    char command[MAX_SERVICE_COMMAND_LENGTH];
    char args[MAX_SERVICE_ARGS_LENGTH];
    char name[MAX_SERVICE_NAME_LENGTH];
    pid_t pid;
    
    bool ok; // used to indicate if service is malformed when parsed from toml etc.
};

struct ServiceArray {
    struct Service array[MAX_SERVICE_ARRAY_SIZE];
    size_t size;
};

int find_service_index_by_name(struct ServiceArray *sa, const char *service_name);

int add_service_to_array(struct ServiceArray *sa, struct Service service);

void remove_service_from_array(struct ServiceArray *sa, const char *service_name);

struct Service read_service_toml_file(const char* dirname, const char* filename);

void start_service(struct Service *service, int *child_pipeds);

void stop_service(const char *service_name, struct ServiceArray *sa);
