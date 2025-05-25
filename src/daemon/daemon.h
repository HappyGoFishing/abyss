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

int setup_socket();
int send_socket(int sock_fd, const char* msg);
ssize_t recv_socket(int sock_fd, char* response_buffer, size_t max_len);

void log_message(int priority, const char *format, ...);
void log_crash_message(const char *format, ...);
void strip_whitespace(char *str);

// result codes returned from ServiceArray functions to make error checking less arcane
#define RESULT_SERVICE_NOT_IN_ARRAY -1
#define RESULT_SERVICE_ARRAY_REACHED_LIMIT -2
#define RESULT_SERVICE_ALREADY_IN_ARRAY -3
#define RESULT_SUCCESS 0


struct Service {
    char command[MAX_SERVICE_COMMAND_LENGTH];
    char args[MAX_SERVICE_ARGS_LENGTH];
    char name[MAX_SERVICE_NAME_LENGTH];
    char working_dir[MAX_PATH_LENGTH];
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






