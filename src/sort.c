#include "sort.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static SortMode current_sort_mode = SORT_NAME;

static int compare_names(const FileEntry *left, const FileEntry *right)
{
    return strcmp(left->name, right->name);
}

static int compare_entries(const void *left_ptr, const void *right_ptr)
{
    const FileEntry *left = (const FileEntry *)left_ptr;
    const FileEntry *right = (const FileEntry *)right_ptr;
    int result = 0;

    if (left->is_directory != right->is_directory) {
        return left->is_directory ? -1 : 1;
    }

    switch (current_sort_mode) {
        case SORT_SIZE:
            if (left->size < right->size) {
                result = -1;
            } else if (left->size > right->size) {
                result = 1;
            }
            break;
        case SORT_MODIFIED:
            if (left->modified_time < right->modified_time) {
                result = -1;
            } else if (left->modified_time > right->modified_time) {
                result = 1;
            }
            break;
        case SORT_TYPE:
            result = strcmp(left->type, right->type);
            break;
        case SORT_NAME:
        default:
            result = compare_names(left, right);
            break;
    }

    if (result != 0) {
        return result;
    }

    return compare_names(left, right);
}

int sort_entries(FileEntry *entries, size_t count, SortMode sort_mode)
{
    if (entries == NULL && count != 0U) {
        errno = EINVAL;
        return -1;
    }

    current_sort_mode = sort_mode;
    qsort(entries, count, sizeof(*entries), compare_entries);
    return 0;
}
