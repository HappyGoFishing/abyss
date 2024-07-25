#pragma once
#include <stdbool.h>
#include <fcntl.h>

struct ServiceData {
    char command[512];
    char args[1024];
    bool ok;
};

struct ServiceInfo {
    struct ServiceData data;
    char service_name[128];
    pid_t pid;
    
};

struct ServiceData read_service_toml_file(const char* dirname, const char* filename);
void start_service(struct ServiceInfo service);