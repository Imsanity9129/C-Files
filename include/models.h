#ifndef CFILES_MODELS_H
#define CFILES_MODELS_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#define CF_NAME_MAX 256
#define CF_PATH_MAX 1024
#define CF_TYPE_MAX 32
#define CF_TIME_STR_MAX 32
#define CF_SEARCH_MAX 256
#define CF_STATUS_MAX 256
#define CF_PREVIEW_MAX 4096

typedef enum SortMode {
    SORT_NAME = 0,
    SORT_SIZE,
    SORT_MODIFIED,
    SORT_TYPE
} SortMode;

typedef struct FileEntry {
    char name[CF_NAME_MAX];
    char full_path[CF_PATH_MAX];
    size_t size;
    bool is_directory;
    char type[CF_TYPE_MAX];
    time_t modified_time;
    char modified_time_str[CF_TIME_STR_MAX];
} FileEntry;

typedef struct AppState {
    FileEntry *entries; /* Dynamic array owned by the application state. */
    size_t count;
    size_t capacity;
    char current_path[CF_PATH_MAX];
    int selected_index;
    char search_query[CF_SEARCH_MAX];
    SortMode sort_mode;
    char status_message[CF_STATUS_MAX];
    char preview_buffer[CF_PREVIEW_MAX];
    bool has_preview;
} AppState;

#endif /* CFILES_MODELS_H */
