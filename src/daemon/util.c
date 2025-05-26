#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/socket.h>
#include <stddef.h>
#include <errno.h>
#include "defines.h"

void log_message(int priority, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsyslog(priority, format, args);

#ifdef DEBUG
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
#endif

    va_end(args);
}


void log_crash_message(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsyslog(LOG_CRIT, format, args);

#ifdef DEBUG
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
#endif

    va_end(args);
    exit(1);
}

char * read_file_to_string(const char * fname) {
    FILE *fp = fopen(fname, "rb");

    if (fp == NULL) {
        return NULL;
    }
    
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    long fsize = ftell(fp);
    
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }
    
    char *rdbuf = malloc(fsize + 1);

    if (rdbuf == NULL) {
        fclose(fp);
        return NULL;
    }

    if (fread(rdbuf, 1, fsize, fp) != (size_t) fsize) {
        free(rdbuf);
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    rdbuf[fsize] = '\0';
    
    return rdbuf;
}

char ** argv_from_args_string(const char *args_str) {
    if (args_str == NULL) {
        return NULL;
    }
    char **argv = (char **)malloc((MAX_SERVICE_ARGS_LENGTH + 1) * sizeof(char *));
    if (argv == NULL) {
        return NULL;
    }
    char arg_str_copy[MAX_SERVICE_ARGS_LENGTH];
    strncpy(arg_str_copy, args_str, sizeof(arg_str_copy));
    arg_str_copy[sizeof(arg_str_copy) - 1] = '\0';
    
    char *token;
    int argc = 1;
    token = strtok(arg_str_copy, " ");
    while (token != NULL && argc <= MAX_ARGS) {
        argv[argc] = (char *)malloc(strlen(token) + 1);
        strcpy(argv[argc], token);
        argc++;
        token = strtok(NULL, " ");
    }
    argv[argc] = NULL;
    return argv;
}

int count_substrings(const char *str) {
    int count = 0;
    const char *p = str;
    while(*p) {
        while (*p  == ' ') p++;
        if (*p) {
            count++;
        }
        while (*p && *p != ' ') {
            p++;
        }
    }
    return count;
}

void strip_whitespace(char *str) {
    if (str == NULL)
        return;
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\n')) {
        str[--len] = '\0';
    }
}

