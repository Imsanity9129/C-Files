#ifndef CFILES_SORT_H
#define CFILES_SORT_H

#include <stddef.h>

#include "models.h"

/* Returns 0 on success, -1 on failure and leaves errno set. */
int sort_entries(FileEntry *entries, size_t count, SortMode sort_mode);

#endif /* CFILES_SORT_H */
