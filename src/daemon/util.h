#pragma once

void log_message(int priority, const char *format, ...);
void log_crash_message(const char *format, ...);
char * read_file_to_string(const char * fname);
int count_substrings(const char *str);
void strip_whitespace(char *str);
char **argv_from_args_string(const char *args_str);
