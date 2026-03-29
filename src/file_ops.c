#include "file_ops.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define CF_INITIAL_ENTRY_CAPACITY 32
#define CF_COPY_BUFFER_SIZE 8192

static void fill_type_string(const struct stat *st, const char *name, char *type_out)
{
    const char *dot;
    size_t i;

    if (S_ISDIR(st->st_mode)) {
        (void)snprintf(type_out, CF_TYPE_MAX, "Directory");
        return;
    }

    if (S_ISLNK(st->st_mode)) {
        (void)snprintf(type_out, CF_TYPE_MAX, "Symlink");
        return;
    }

    if (!S_ISREG(st->st_mode)) {
        (void)snprintf(type_out, CF_TYPE_MAX, "Other");
        return;
    }

    dot = strrchr(name, '.');
    if (dot == NULL || dot[1] == '\0') {
        (void)snprintf(type_out, CF_TYPE_MAX, "File");
        return;
    }

    for (i = 0; i < (size_t)(CF_TYPE_MAX - 1) && dot[1 + i] != '\0'; ++i) {
        type_out[i] = (char)tolower((unsigned char)dot[1 + i]);
    }
    type_out[i] = '\0';
}

static void fill_modified_time_string(time_t modified_time, char *buffer_out)
{
    struct tm time_info;

    if (localtime_r(&modified_time, &time_info) == NULL) {
        buffer_out[0] = '\0';
        return;
    }

    if (strftime(buffer_out, CF_TIME_STR_MAX, "%Y-%m-%d %H:%M", &time_info) == 0) {
        buffer_out[0] = '\0';
    }
}

int load_directory(const char *path, FileEntry **entries_out, size_t *count_out)
{
    DIR *dir;
    struct dirent *entry;
    FileEntry *entries;
    size_t count;
    size_t capacity;

    if (path == NULL || entries_out == NULL || count_out == NULL) {
        errno = EINVAL;
        return -1;
    }

    dir = opendir(path);
    if (dir == NULL) {
        return -1;
    }

    entries = NULL;
    count = 0;
    capacity = 0;

    while ((entry = readdir(dir)) != NULL) {
        FileEntry *grown_entries;
        FileEntry *current;
        struct stat st;
        int written;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (count == capacity) {
            size_t new_capacity = (capacity == 0) ? CF_INITIAL_ENTRY_CAPACITY : capacity * 2;

            grown_entries = realloc(entries, new_capacity * sizeof(*entries));
            if (grown_entries == NULL) {
                free(entries);
                (void)closedir(dir);
                return -1;
            }

            entries = grown_entries;
            capacity = new_capacity;
        }

        current = &entries[count];
        memset(current, 0, sizeof(*current));

        (void)snprintf(current->name, sizeof(current->name), "%s", entry->d_name);
        written = snprintf(current->full_path, sizeof(current->full_path), "%s/%s", path, entry->d_name);
        if (written < 0 || (size_t)written >= sizeof(current->full_path)) {
            free(entries);
            (void)closedir(dir);
            errno = ENAMETOOLONG;
            return -1;
        }

        if (lstat(current->full_path, &st) != 0) {
            free(entries);
            (void)closedir(dir);
            return -1;
        }

        current->size = (size_t)st.st_size;
        current->is_directory = S_ISDIR(st.st_mode);
        current->modified_time = st.st_mtime;
        fill_type_string(&st, current->name, current->type);
        fill_modified_time_string(current->modified_time, current->modified_time_str);

        ++count;
    }

    if (closedir(dir) != 0) {
        free(entries);
        return -1;
    }

    *entries_out = entries;
    *count_out = count;
    return 0;
}

void free_file_entries(FileEntry *entries)
{
    free(entries);
}

int read_text_file(const char *path, char *buffer_out)
{
    FILE *file;
    size_t bytes_read;

    if (path == NULL || buffer_out == NULL) {
        errno = EINVAL;
        return -1;
    }

    file = fopen(path, "r");
    if (file == NULL) {
        return -1;
    }

    bytes_read = fread(buffer_out, 1, CF_PREVIEW_MAX - 1, file);
    buffer_out[bytes_read] = '\0';

    if (ferror(file)) {
        int saved_errno = errno;

        (void)fclose(file);
        errno = (saved_errno != 0) ? saved_errno : EIO;
        return -1;
    }

    if (fclose(file) != 0) {
        return -1;
    }

    return 0;
}

int copy_file(const char *src, const char *dst)
{
    int src_fd;
    int dst_fd;
    struct stat st;
    char buffer[CF_COPY_BUFFER_SIZE];

    if (src == NULL || dst == NULL) {
        errno = EINVAL;
        return -1;
    }

    src_fd = open(src, O_RDONLY);
    if (src_fd < 0) {
        return -1;
    }

    if (fstat(src_fd, &st) != 0) {
        int saved_errno = errno;

        (void)close(src_fd);
        errno = saved_errno;
        return -1;
    }

    dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777);
    if (dst_fd < 0) {
        int saved_errno = errno;

        (void)close(src_fd);
        errno = saved_errno;
        return -1;
    }

    for (;;) {
        ssize_t bytes_read = read(src_fd, buffer, sizeof(buffer));

        if (bytes_read < 0) {
            int saved_errno = errno;

            (void)close(src_fd);
            (void)close(dst_fd);
            errno = saved_errno;
            return -1;
        }

        if (bytes_read == 0) {
            break;
        }

        {
            ssize_t total_written = 0;

            while (total_written < bytes_read) {
                ssize_t bytes_written = write(dst_fd,
                                              buffer + total_written,
                                              (size_t)(bytes_read - total_written));

                if (bytes_written < 0) {
                    int saved_errno = errno;

                    (void)close(src_fd);
                    (void)close(dst_fd);
                    errno = saved_errno;
                    return -1;
                }

                total_written += bytes_written;
            }
        }
    }

    if (close(src_fd) != 0) {
        int saved_errno = errno;

        (void)close(dst_fd);
        errno = saved_errno;
        return -1;
    }

    if (close(dst_fd) != 0) {
        return -1;
    }

    return 0;
}

int delete_file(const char *path)
{
    if (path == NULL) {
        errno = EINVAL;
        return -1;
    }

    return unlink(path);
}
