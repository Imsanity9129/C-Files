#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "raylib.h"

#include "file_ops.h"
#include "models.h"
#include "nav.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 600
#define LIST_TOP 230
#define ROW_HEIGHT 24
#define ROW_START_Y (LIST_TOP + 35)

static size_t get_visible_row_count(size_t count)
{
    const int max_visible_rows = (WINDOW_HEIGHT - LIST_TOP - 20) / ROW_HEIGHT;
    size_t visible_count = count;

    if ((int)visible_count > max_visible_rows) {
        visible_count = (size_t)max_visible_rows;
    }

    return visible_count;
}

static int get_clicked_row_index(size_t count)
{
    Vector2 mouse_position = GetMousePosition();
    size_t visible_count = get_visible_row_count(count);
    size_t i;

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return -1;
    }

    for (i = 0; i < visible_count; ++i) {
        Rectangle row_bounds = {
            35.0f,
            (float)(ROW_START_Y + ((int)i * ROW_HEIGHT) - 2),
            (float)(WINDOW_WIDTH - 70),
            (float)ROW_HEIGHT
        };

        if (CheckCollisionPointRec(mouse_position, row_bounds)) {
            return (int)i;
        }
    }

    return -1;
}

static void draw_file_list(const FileEntry *entries, size_t count, int selected_index)
{
    size_t visible_count = get_visible_row_count(count);
    size_t i;

    DrawText("Name", 40, LIST_TOP, 20, BLACK);
    DrawText("Size", 420, LIST_TOP, 20, BLACK);
    DrawText("Type", 540, LIST_TOP, 20, BLACK);
    DrawText("Modified", 680, LIST_TOP, 20, BLACK);

    for (i = 0; i < visible_count; ++i) {
        char name_text[CF_NAME_MAX + 16];
        char size_text[64];
        int row_y = ROW_START_Y + ((int)i * ROW_HEIGHT);
        Color row_color = entries[i].is_directory ? BLUE : DARKGRAY;
        Color detail_color = DARKGRAY;

        if ((int)i == selected_index) {
            DrawRectangle(35, row_y - 2, WINDOW_WIDTH - 70, ROW_HEIGHT, SKYBLUE);
            row_color = DARKBLUE;
            detail_color = BLACK;
        }

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
        DrawText(size_text, 420, row_y, 18, detail_color);
        DrawText(entries[i].type, 540, row_y, 18, detail_color);
        DrawText(entries[i].modified_time_str, 680, row_y, 18, detail_color);
    }
}

int main(void)
{
    char current_path[CF_PATH_MAX];
    FileEntry *entries = NULL;
    size_t count = 0;
    int selected_index = -1;
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
        int clicked_index = get_clicked_row_index(count);

        if (clicked_index >= 0) {
            selected_index = clicked_index;

            if (entries[clicked_index].is_directory) {
                char next_path[CF_PATH_MAX];
                char old_path[CF_PATH_MAX];
                FileEntry *old_entries = entries;

                snprintf(old_path, sizeof(old_path), "%s", current_path);
                snprintf(next_path, sizeof(next_path), "%s", current_path);

                if (go_to_directory(next_path, entries[clicked_index].name) == 0) {
                    free_file_entries(old_entries);
                    entries = NULL;
                    count = 0;

                    if (load_directory(next_path, &entries, &count) == 0) {
                        snprintf(current_path, sizeof(current_path), "%s", next_path);
                        snprintf(status_text, sizeof(status_text), "Entered directory");
                        selected_index = -1;
                    } else {
                        snprintf(status_text, sizeof(status_text), "Failed to load directory");

                        if (load_directory(old_path, &entries, &count) == 0) {
                            snprintf(current_path, sizeof(current_path), "%s", old_path);
                            selected_index = -1;
                        } else {
                            count = 0;
                            entries = NULL;
                            selected_index = -1;
                        }
                    }
                } else {
                    snprintf(status_text, sizeof(status_text), "Failed to enter directory");
                }
            }
        }

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
        draw_file_list(entries, count, selected_index);

        EndDrawing();
    }

    free_file_entries(entries);
    CloseWindow();

    return EXIT_SUCCESS;
}
