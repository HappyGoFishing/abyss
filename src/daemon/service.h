#pragma once
#include "defines.h"
struct Service {
    char command[MAX_SERVICE_COMMAND_LENGTH];
    char args[MAX_SERVICE_ARGS_LENGTH];
    char name[MAX_SERVICE_NAME_LENGTH];
    char working_dir[MAX_PATH_LENGTH];
    pid_t pid; 
};
