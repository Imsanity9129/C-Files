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

static bool reload_directory(char *current_path,
                             const char *next_path,
                             FileEntry **entries,
                             size_t *count,
                             int *selected_index,
                             char *status_text,
                             const char *success_message,
                             const char *error_message)
{
    FileEntry *new_entries = NULL;
    size_t new_count = 0;

    if (load_directory(next_path, &new_entries, &new_count) != 0) {
        snprintf(status_text, CF_STATUS_MAX, "%s", error_message);
        return false;
    }

    free_file_entries(*entries);
    *entries = new_entries;
    *count = new_count;
    *selected_index = -1;
    snprintf(current_path, CF_PATH_MAX, "%s", next_path);
    snprintf(status_text, CF_STATUS_MAX, "%s", success_message);
    return true;
}

static Rectangle get_up_button_bounds(void)
{
    Rectangle button_bounds = {40.0f, 92.0f, 72.0f, 32.0f};

    return button_bounds;
}

static bool is_up_button_clicked(void)
{
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return false;
    }

    return CheckCollisionPointRec(GetMousePosition(), get_up_button_bounds());
}

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

static void draw_up_button(void)
{
    Rectangle button_bounds = get_up_button_bounds();

    DrawRectangleRec(button_bounds, GRAY);
    DrawRectangleLines((int)button_bounds.x,
                       (int)button_bounds.y,
                       (int)button_bounds.width,
                       (int)button_bounds.height,
                       BLACK);
    DrawText("Up", (int)button_bounds.x + 21, (int)button_bounds.y + 6, 20, RAYWHITE);
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
        if (is_up_button_clicked()) {
            char old_path[CF_PATH_MAX];

            snprintf(old_path, sizeof(old_path), "%s", current_path);

            if (go_up_directory(current_path) == 0) {
                if (!reload_directory(current_path,
                                      current_path,
                                      &entries,
                                      &count,
                                      &selected_index,
                                      status_text,
                                      "Moved to parent directory",
                                      "Failed to load parent directory")) {
                    snprintf(current_path, sizeof(current_path), "%s", old_path);
                    selected_index = -1;
                }
            } else {
                snprintf(status_text, sizeof(status_text), "Failed to go up");
            }
        }

        int clicked_index = get_clicked_row_index(count);

        if (clicked_index >= 0) {
            selected_index = clicked_index;

            if (entries[clicked_index].is_directory) {
                char next_path[CF_PATH_MAX];
                snprintf(next_path, sizeof(next_path), "%s", current_path);

                if (go_to_directory(next_path, entries[clicked_index].name) == 0) {
                    if (!reload_directory(current_path,
                                          next_path,
                                          &entries,
                                          &count,
                                          &selected_index,
                                          status_text,
                                          "Entered directory",
                                          "Failed to load directory")) {
                        selected_index = -1;
                    }
                } else {
                    snprintf(status_text, sizeof(status_text), "Failed to enter directory");
                }
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("C-Files", 40, 40, 32, BLACK);
        draw_up_button();
        DrawText(current_path, 130, 100, 20, DARKGRAY);

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
