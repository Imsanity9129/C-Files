#include "nav.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "models.h"

static int copy_checked_path(char *destination, const char *source)
{
    int written = snprintf(destination, CF_PATH_MAX, "%s", source);

    if (written < 0 || written >= CF_PATH_MAX) {
        errno = ENAMETOOLONG;
        return -1;
    }

    return 0;
}

static void trim_trailing_slashes(char *path)
{
    size_t length = strlen(path);

    while (length > 1 && path[length - 1] == '/') {
        path[length - 1] = '\0';
        --length;
    }
}

int go_to_directory(char *current_path, const char *next_dir)
{
    char candidate[CF_PATH_MAX];
    struct stat st;
    int written;

    if (current_path == NULL || next_dir == NULL || next_dir[0] == '\0') {
        errno = EINVAL;
        return -1;
    }

    if (next_dir[0] == '/') {
        written = snprintf(candidate, sizeof(candidate), "%s", next_dir);
    } else if (strcmp(current_path, "/") == 0) {
        written = snprintf(candidate, sizeof(candidate), "/%s", next_dir);
    } else {
        written = snprintf(candidate, sizeof(candidate), "%s/%s", current_path, next_dir);
    }

    if (written < 0 || (size_t)written >= sizeof(candidate)) {
        errno = ENAMETOOLONG;
        return -1;
    }

    trim_trailing_slashes(candidate);

    if (stat(candidate, &st) != 0) {
        return -1;
    }

    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return -1;
    }

    return copy_checked_path(current_path, candidate);
}

int go_up_directory(char *current_path)
{
    char parent[CF_PATH_MAX];
    char *last_slash;

    if (current_path == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (strcmp(current_path, "/") == 0) {
        return copy_checked_path(current_path, "/");
    }

    if (copy_checked_path(parent, current_path) != 0) {
        return -1;
    }

    trim_trailing_slashes(parent);

    last_slash = strrchr(parent, '/');
    if (last_slash == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (last_slash == parent) {
        parent[1] = '\0';
    } else {
        *last_slash = '\0';
    }

    return copy_checked_path(current_path, parent);
}
