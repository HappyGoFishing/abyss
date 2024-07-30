#pragma once
#include <stdbool.h>
#include <fcntl.h>
#include <stddef.h>
#define SERVICE_PATH "./"
#define MAX_SERVICE_ARRAY_SIZE 128 // number of services that can be started (might make the array dynamically resize later on but cba right now)

struct Service {
    char command[128];
    char args[1024];
    char name[64];
    pid_t pid;
    
    bool ok; // used to indicate if service is malformed when parsed from toml etc.
};

struct ServiceArray {
    struct Service array[MAX_SERVICE_ARRAY_SIZE];
    size_t size;
};

bool is_service_in_array(struct ServiceArray *array, const char *service_name);

void add_service_to_array(struct ServiceArray *array, struct Service service);

void remove_service_from_array(struct ServiceArray *array, const char *service_name);


struct Service read_service_toml_file(const char* dirname, const char* filename);


void start_service(struct Service service);
