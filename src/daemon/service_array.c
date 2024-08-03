#include <stdio.h>
#include <string.h>

#include "service.h"

int find_service_index_by_name(struct ServiceArray *sa, const char *service_name) {
    for (size_t i = 0; i < sa->size; i++) {
        if (!strcmp(service_name, sa->array[i].name)) {
            return i;
        }
    }
    return -1;
}

int add_service_to_array(struct ServiceArray *sa, struct Service service) {
    if (sa->size >= MAX_SERVICE_ARRAY_SIZE) {
        printf("cant add service: %s (reached MAX_SERVICE_ARRAY_SIZE)\n", service.name);
        return -2;
    }
    if (find_service_index_by_name(sa, service.name) != -1) {
        return -1;
    }
    sa->array[sa->size] = service;
    sa->size++;
    return 0;
}

void remove_service_from_array(struct ServiceArray *sa, const char *service_name) {
    int index = find_service_index_by_name(sa, service_name);
    if (index != -1) {
        for (size_t i = index; i < sa->size -1; i++) {
            sa->array[i] = sa->array[i + 1];
        }
        sa->size--;
    } else {
        printf("cant remove service: %s (service not found\n)", service_name);
    }
}
