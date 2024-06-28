#pragma once
#include <stdbool.h>

struct ServiceData {
    char command[512];
    char args[1024];
    bool ok;
};


struct ServiceData read_service_toml_file(const char* dirname, const char* filename);
void start_service(struct ServiceData sv);