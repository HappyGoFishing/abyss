#pragma once

#include <sys/types.h>
#include "defines.h"

void start_service(struct Service *service, int *child_pipefds);
int stop_service(const char *service_name, struct ServiceArray *sa);
