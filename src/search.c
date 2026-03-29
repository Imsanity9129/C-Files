#include "search.h"

#include <ctype.h>
#include <stddef.h>

bool matches_search(const char *filename, const char *query)
{
    size_t query_index;

    if (filename == NULL || query == NULL) {
        return false;
    }

    if (query[0] == '\0') {
        return true;
    }

    for (; *filename != '\0'; ++filename) {
        query_index = 0;

        while (query[query_index] != '\0' &&
               filename[query_index] != '\0' &&
               tolower((unsigned char)filename[query_index]) ==
                   tolower((unsigned char)query[query_index])) {
            ++query_index;
        }

        if (query[query_index] == '\0') {
            return true;
        }
    }

    return false;
}
