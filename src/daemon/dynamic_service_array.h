#pragma once
#include "defines.h"
#include "service.h"
struct ServiceArray {
    struct Service array[MAX_SERVICE_ARRAY_SIZE];
    size_t size;
};



int find_service_index_by_name(struct ServiceArray *sa, const char *service_name);

int add_service_to_array(struct ServiceArray *sa, struct Service service);

int remove_service_from_array(struct ServiceArray *sa, const char *service_name);

void start_service(struct Service *service, int *child_pipeds);

int stop_service(const char *service_name, struct ServiceArray *sa);



