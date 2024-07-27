#pragma once
#include <stdbool.h>
#include <fcntl.h>

struct Service {
    char command[128];
    char args[1024];
    char name[64];
    pid_t pid;
    
    bool ok; // used to indicate if service is malformed when parsed from toml etc.
};

struct Service read_service_toml_file(const char* dirname, const char* filename);
void start_service(struct Service service);

