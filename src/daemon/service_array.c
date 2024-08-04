#include <stdio.h>
#include <string.h>

#include "service.h"

int find_service_index_by_name(struct ServiceArray *sa, const char *service_name) {
    for (size_t i = 0; i < sa->size; i++) {
        if (!strcmp(service_name, sa->array[i].name)) {
            return i;
        }
    }
    return -1; //service is not in array
}

int add_service_to_array(struct ServiceArray *sa, struct Service service) {
    if (sa->size >= MAX_SERVICE_ARRAY_SIZE) return -2; //cant add service, reached max service limit
    if (find_service_index_by_name(sa, service.name) != -1) return -1; //cant add service, service is already in array
    sa->array[sa->size] = service;
    sa->size++;
    return 0; //added service to array
}

int remove_service_from_array(struct ServiceArray *sa, const char *service_name) {
    int index = find_service_index_by_name(sa, service_name);
    if (index != -1) {
        for (size_t i = index; i < sa->size -1; i++) sa->array[i] = sa->array[i + 1];
        sa->size--;
        return 0; //removed service from array
    } 
    return -1; //cant remove service because its not in array
}
