#pragma once

#include <syslog.h>

struct Service *read_service_toml_file(const char *dirname, const char *filename);
