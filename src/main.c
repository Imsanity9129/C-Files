#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <raylib.h>

#include "file_ops.h"
#include "models.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 600

int main(void)
{
    char current_path[CF_PATH_MAX];
    FileEntry *entries = NULL;
    size_t count = 0;
    char status_text[CF_STATUS_MAX];

    if (getcwd(current_path, sizeof(current_path)) == NULL) {
        fprintf(stderr, "Failed to get current working directory: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (load_directory(current_path, &entries, &count) != 0) {
        fprintf(stderr, "Failed to load directory \"%s\": %s\n", current_path, strerror(errno));
        return EXIT_FAILURE;
    }

    snprintf(status_text, sizeof(status_text), "GUI skeleton working");

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "C-Files");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("C-Files", 40, 40, 32, BLACK);
        DrawText(current_path, 40, 100, 20, DARKGRAY);

        {
            char count_text[128];

            snprintf(count_text, sizeof(count_text), "Loaded entries: %zu", count);
            DrawText(count_text, 40, 140, 20, BLACK);
        }

        DrawText(status_text, 40, 180, 20, DARKGRAY);

        EndDrawing();
    }

    free_file_entries(entries);
    CloseWindow();

    return EXIT_SUCCESS;
}
