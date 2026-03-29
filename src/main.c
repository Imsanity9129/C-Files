#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "raylib.h"

#include "file_ops.h"
#include "models.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 600

static void draw_file_list(const FileEntry *entries, size_t count)
{
    const int list_top = 230;
    const int row_height = 24;
    const int max_visible_rows = (WINDOW_HEIGHT - list_top - 20) / row_height;
    size_t visible_count = count;
    size_t i;

    if ((int)visible_count > max_visible_rows) {
        visible_count = (size_t)max_visible_rows;
    }

    DrawText("Name", 40, list_top, 20, BLACK);
    DrawText("Size", 420, list_top, 20, BLACK);
    DrawText("Type", 540, list_top, 20, BLACK);
    DrawText("Modified", 680, list_top, 20, BLACK);

    for (i = 0; i < visible_count; ++i) {
        char name_text[CF_NAME_MAX + 16];
        char size_text[64];
        int row_y = list_top + 35 + ((int)i * row_height);
        Color row_color = entries[i].is_directory ? BLUE : DARKGRAY;

        snprintf(name_text,
                 sizeof(name_text),
                 "%s %s",
                 entries[i].is_directory ? "[DIR]" : "[FILE]",
                 entries[i].name);

        if (entries[i].is_directory) {
            snprintf(size_text, sizeof(size_text), "--");
        } else {
            snprintf(size_text, sizeof(size_text), "%zu", entries[i].size);
        }

        DrawText(name_text, 40, row_y, 18, row_color);
        DrawText(size_text, 420, row_y, 18, DARKGRAY);
        DrawText(entries[i].type, 540, row_y, 18, DARKGRAY);
        DrawText(entries[i].modified_time_str, 680, row_y, 18, DARKGRAY);
    }
}

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
        draw_file_list(entries, count);

        EndDrawing();
    }

    free_file_entries(entries);
    CloseWindow();

    return EXIT_SUCCESS;
}
