#pragma once
#include <stdbool.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/types.h>

#define SOCKET_PATH "/tmp/abyss.sock"
#define BUFFER_SIZE 1024
#define MAX_COMMAND_LIST_SIZE 2
#define SERVICE_AUTOSTART_LIST_FILE "./etc/abyss/services/autostart.txt"
#define MAX_AUTOSTART_SERVICES 32
#define SERVICES_DIR_PATH "./etc/abyss/services"
#define MAX_PATH_LENGTH 1024
#define MAX_SERVICE_ARRAY_SIZE 128 
#define MAX_SERVICE_COMMAND_LENGTH 128
#define MAX_SERVICE_ARGS_LENGTH 512
#define MAX_SERVICE_NAME_LENGTH 64
#define MAX_ARGS MAX_SERVICE_ARGS_LENGTH / 2 

// result codes returned from ServiceArray functions to make error checking less arcane
#define RESULT_SERVICE_NOT_IN_ARRAY -1
#define RESULT_SERVICE_ARRAY_REACHED_LIMIT -2
#define RESULT_SERVICE_ALREADY_IN_ARRAY -3
#define RESULT_SUCCESS 0



