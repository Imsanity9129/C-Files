#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "file_ops.h"
#include "models.h"
#include "search.h"
#include "sort.h"

static void print_entry(const FileEntry *entry, size_t index)
{
    printf("[%zu] name=%s | size=%zu | type=%s | modified=%s | directory=%s\n",
           index,
           entry->name,
           entry->size,
           entry->type,
           entry->modified_time_str,
           entry->is_directory ? "yes" : "no");
}

static void print_entries(const FileEntry *entries, size_t count)
{
    size_t i;

    for (i = 0; i < count; ++i) {
        print_entry(&entries[i], i);
    }
}

static void print_search_results(const FileEntry *entries, size_t count, const char *query)
{
    size_t i;
    int matches_found = 0;

    printf("\nSearch results for \"%s\":\n", query);

    for (i = 0; i < count; ++i) {
        if (matches_search(entries[i].name, query)) {
            print_entry(&entries[i], i);
            matches_found = 1;
        }
    }

    if (!matches_found) {
        printf("No matches found.\n");
    }
}

int main(void)
{
    char current_path[CF_PATH_MAX];
    FileEntry *entries = NULL;
    size_t count = 0;

    if (getcwd(current_path, sizeof(current_path)) == NULL) {
        fprintf(stderr, "Failed to get current working directory: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (load_directory(current_path, &entries, &count) != 0) {
        fprintf(stderr, "Failed to load directory \"%s\": %s\n", current_path, strerror(errno));
        return EXIT_FAILURE;
    }

    printf("Current path: %s\n", current_path);
    printf("Loaded %zu entries:\n", count);
    print_entries(entries, count);

    print_search_results(entries, count, "c");

    if (sort_entries(entries, count, SORT_NAME) != 0) {
        fprintf(stderr, "Failed to sort entries: %s\n", strerror(errno));
        free_file_entries(entries);
        return EXIT_FAILURE;
    }

    printf("\nEntries sorted by name:\n");
    print_entries(entries, count);

    free_file_entries(entries);
    return EXIT_SUCCESS;
}
