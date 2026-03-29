#ifndef CFILES_FILE_OPS_H
#define CFILES_FILE_OPS_H

#include <stddef.h>

#include "models.h"

/* Returns 0 on success, -1 on failure and leaves errno set. */
int load_directory(const char *path, FileEntry **entries_out, size_t *count_out);
void free_file_entries(FileEntry *entries);
int read_text_file(const char *path, char *buffer_out);
int copy_file(const char *src, const char *dst);
int delete_file(const char *path);

#endif /* CFILES_FILE_OPS_H */
