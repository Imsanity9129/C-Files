#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "raylib.h"

#include "file_ops.h"
#include "models.h"
#include "nav.h"
#include "search.h"
#include "sort.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 600
#define TITLE_X 40
#define TITLE_Y 36
#define BUTTON_ROW_Y 82
#define FILTER_ROW_Y 124
#define PATH_X 40
#define PATH_Y 168
#define INFO_X 40
#define INFO_Y 204
#define STATUS_Y 234
#define PATH_FONT_SIZE 20
#define LIST_TOP 268
#define ROW_HEIGHT 24
#define ROW_START_Y (LIST_TOP + 35)
#define ROW_FONT_SIZE 18
#define NAME_COLUMN_WIDTH 360
#define SIZE_COLUMN_WIDTH 100
#define TYPE_COLUMN_WIDTH 120
#define MODIFIED_COLUMN_WIDTH 240
#define SEARCH_BOX_X 250
#define SEARCH_BOX_Y FILTER_ROW_Y
#define SEARCH_BOX_WIDTH 420
#define SEARCH_BOX_HEIGHT 32
#define SORT_BUTTON_X 690
#define SORT_BUTTON_Y FILTER_ROW_Y
#define SORT_BUTTON_WIDTH 140
#define SORT_BUTTON_HEIGHT 32
#define COPY_BUTTON_X 240
#define COPY_BUTTON_Y BUTTON_ROW_Y
#define COPY_BUTTON_WIDTH 90
#define COPY_BUTTON_HEIGHT 32
#define PASTE_BUTTON_X 340
#define PASTE_BUTTON_Y BUTTON_ROW_Y
#define PASTE_BUTTON_WIDTH 90
#define PASTE_BUTTON_HEIGHT 32
#define DELETE_BUTTON_X 440
#define DELETE_BUTTON_Y BUTTON_ROW_Y
#define DELETE_BUTTON_WIDTH 100
#define DELETE_BUTTON_HEIGHT 32
#define NEW_FOLDER_BUTTON_X 550
#define NEW_FOLDER_BUTTON_Y BUTTON_ROW_Y
#define NEW_FOLDER_BUTTON_WIDTH 150
#define NEW_FOLDER_BUTTON_HEIGHT 32
#define RENAME_BUTTON_X 710
#define RENAME_BUTTON_Y BUTTON_ROW_Y
#define RENAME_BUTTON_WIDTH 120
#define RENAME_BUTTON_HEIGHT 32
#define RENAME_BOX_X 300
#define RENAME_BOX_Y 194
#define RENAME_BOX_WIDTH 360
#define RENAME_BOX_HEIGHT 32

static size_t get_filtered_entry_count(const FileEntry *entries, size_t count, const char *search_query)
{
    size_t filtered_count = 0;
    size_t i;

    for (i = 0; i < count; ++i) {
        if (matches_search(entries[i].name, search_query)) {
            filtered_count++;
        }
    }

    return filtered_count;
}

static int get_filtered_entry_index(const FileEntry *entries,
                                    size_t count,
                                    const char *search_query,
                                    int filtered_index)
{
    int current_filtered_index = 0;
    size_t i;

    for (i = 0; i < count; ++i) {
        if (!matches_search(entries[i].name, search_query)) {
            continue;
        }

        if (current_filtered_index == filtered_index) {
            return (int)i;
        }

        current_filtered_index++;
    }

    return -1;
}

static void truncate_text_to_width(const char *input,
                                   char *output,
                                   size_t output_size,
                                   int font_size,
                                   int max_width)
{
    const char *ellipsis = "...";
    size_t length;

    if (output_size == 0) {
        return;
    }

    output[0] = '\0';

    if (input == NULL) {
        return;
    }

    snprintf(output, output_size, "%s", input);
    if (MeasureText(output, font_size) <= max_width) {
        return;
    }

    if (MeasureText(ellipsis, font_size) > max_width) {
        return;
    }

    length = strlen(input);
    while (length > 0) {
        snprintf(output, output_size, "%.*s%s", (int)length, input, ellipsis);
        if (MeasureText(output, font_size) <= max_width) {
            return;
        }
        length--;
    }

    snprintf(output, output_size, "%s", ellipsis);
}

static const char *get_sort_mode_name(SortMode sort_mode)
{
    switch (sort_mode) {
        case SORT_SIZE:
            return "Size";
        case SORT_TYPE:
            return "Type";
        case SORT_MODIFIED:
            return "Modified";
        case SORT_NAME:
        default:
            return "Name";
    }
}

static SortMode get_next_sort_mode(SortMode sort_mode)
{
    switch (sort_mode) {
        case SORT_NAME:
            return SORT_SIZE;
        case SORT_SIZE:
            return SORT_TYPE;
        case SORT_TYPE:
            return SORT_MODIFIED;
        case SORT_MODIFIED:
        default:
            return SORT_NAME;
    }
}

static bool reload_directory(char *current_path,
                             const char *next_path,
                             FileEntry **entries,
                             size_t *count,
                             SortMode sort_mode,
                             int *selected_index,
                             int *scroll_offset,
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

    if (sort_entries(new_entries, new_count, sort_mode) != 0) {
        free_file_entries(new_entries);
        snprintf(status_text, CF_STATUS_MAX, "%s", error_message);
        return false;
    }

    free_file_entries(*entries);
    *entries = new_entries;
    *count = new_count;
    *selected_index = -1;
    *scroll_offset = 0;
    snprintf(current_path, CF_PATH_MAX, "%s", next_path);
    snprintf(status_text, CF_STATUS_MAX, "%s", success_message);
    return true;
}

static Rectangle get_back_button_bounds(void)
{
    Rectangle button_bounds = {(float)TITLE_X, (float)BUTTON_ROW_Y, 82.0f, 32.0f};

    return button_bounds;
}

static Rectangle get_refresh_button_bounds(void)
{
    Rectangle button_bounds = {132.0f, (float)BUTTON_ROW_Y, 96.0f, 32.0f};

    return button_bounds;
}

static Rectangle get_search_box_bounds(void)
{
    Rectangle box_bounds = {(float)SEARCH_BOX_X,
                            (float)SEARCH_BOX_Y,
                            (float)SEARCH_BOX_WIDTH,
                            (float)SEARCH_BOX_HEIGHT};

    return box_bounds;
}

static Rectangle get_sort_button_bounds(void)
{
    Rectangle button_bounds = {(float)SORT_BUTTON_X,
                               (float)SORT_BUTTON_Y,
                               (float)SORT_BUTTON_WIDTH,
                               (float)SORT_BUTTON_HEIGHT};

    return button_bounds;
}

static Rectangle get_copy_button_bounds(void)
{
    Rectangle button_bounds = {(float)COPY_BUTTON_X,
                               (float)COPY_BUTTON_Y,
                               (float)COPY_BUTTON_WIDTH,
                               (float)COPY_BUTTON_HEIGHT};

    return button_bounds;
}

static Rectangle get_paste_button_bounds(void)
{
    Rectangle button_bounds = {(float)PASTE_BUTTON_X,
                               (float)PASTE_BUTTON_Y,
                               (float)PASTE_BUTTON_WIDTH,
                               (float)PASTE_BUTTON_HEIGHT};

    return button_bounds;
}

static Rectangle get_delete_button_bounds(void)
{
    Rectangle button_bounds = {(float)DELETE_BUTTON_X,
                               (float)DELETE_BUTTON_Y,
                               (float)DELETE_BUTTON_WIDTH,
                               (float)DELETE_BUTTON_HEIGHT};

    return button_bounds;
}

static Rectangle get_new_folder_button_bounds(void)
{
    Rectangle button_bounds = {(float)NEW_FOLDER_BUTTON_X,
                               (float)NEW_FOLDER_BUTTON_Y,
                               (float)NEW_FOLDER_BUTTON_WIDTH,
                               (float)NEW_FOLDER_BUTTON_HEIGHT};

    return button_bounds;
}

static Rectangle get_rename_button_bounds(void)
{
    Rectangle button_bounds = {(float)RENAME_BUTTON_X,
                               (float)RENAME_BUTTON_Y,
                               (float)RENAME_BUTTON_WIDTH,
                               (float)RENAME_BUTTON_HEIGHT};

    return button_bounds;
}

static Rectangle get_rename_box_bounds(void)
{
    Rectangle box_bounds = {(float)RENAME_BOX_X,
                            (float)RENAME_BOX_Y,
                            (float)RENAME_BOX_WIDTH,
                            (float)RENAME_BOX_HEIGHT};

    return box_bounds;
}

static bool is_back_button_clicked(bool mouse_clicked, Vector2 mouse_position)
{
    if (!mouse_clicked) {
        return false;
    }

    return CheckCollisionPointRec(mouse_position, get_back_button_bounds());
}

static bool is_refresh_button_clicked(bool mouse_clicked, Vector2 mouse_position)
{
    if (!mouse_clicked) {
        return false;
    }

    return CheckCollisionPointRec(mouse_position, get_refresh_button_bounds());
}

static bool is_search_box_clicked(bool mouse_clicked, Vector2 mouse_position)
{
    if (!mouse_clicked) {
        return false;
    }

    return CheckCollisionPointRec(mouse_position, get_search_box_bounds());
}

static bool is_sort_button_clicked(bool mouse_clicked, Vector2 mouse_position)
{
    if (!mouse_clicked) {
        return false;
    }

    return CheckCollisionPointRec(mouse_position, get_sort_button_bounds());
}

static bool is_copy_button_clicked(bool mouse_clicked, Vector2 mouse_position)
{
    if (!mouse_clicked) {
        return false;
    }

    return CheckCollisionPointRec(mouse_position, get_copy_button_bounds());
}

static bool is_paste_button_clicked(bool mouse_clicked, Vector2 mouse_position)
{
    if (!mouse_clicked) {
        return false;
    }

    return CheckCollisionPointRec(mouse_position, get_paste_button_bounds());
}

static bool is_delete_button_clicked(bool mouse_clicked, Vector2 mouse_position)
{
    if (!mouse_clicked) {
        return false;
    }

    return CheckCollisionPointRec(mouse_position, get_delete_button_bounds());
}

static bool is_new_folder_button_clicked(bool mouse_clicked, Vector2 mouse_position)
{
    if (!mouse_clicked) {
        return false;
    }

    return CheckCollisionPointRec(mouse_position, get_new_folder_button_bounds());
}

static bool is_rename_button_clicked(bool mouse_clicked, Vector2 mouse_position)
{
    if (!mouse_clicked) {
        return false;
    }

    return CheckCollisionPointRec(mouse_position, get_rename_button_bounds());
}

static int get_max_visible_rows(void)
{
    return (WINDOW_HEIGHT - LIST_TOP - 20) / ROW_HEIGHT;
}

static size_t get_visible_row_count(size_t count, int scroll_offset)
{
    size_t remaining_count;
    size_t visible_count;
    int max_visible_rows = get_max_visible_rows();

    if (count == 0 || scroll_offset < 0 || (size_t)scroll_offset >= count) {
        return 0;
    }

    remaining_count = count - (size_t)scroll_offset;
    visible_count = remaining_count;

    if ((int)visible_count > max_visible_rows) {
        visible_count = (size_t)max_visible_rows;
    }

    return visible_count;
}

static int get_max_scroll_offset(size_t count)
{
    int max_visible_rows = get_max_visible_rows();

    if ((int)count <= max_visible_rows) {
        return 0;
    }

    return (int)count - max_visible_rows;
}

static void update_scroll_offset(size_t count, int *scroll_offset)
{
    float wheel_move = GetMouseWheelMove();
    int max_scroll_offset = get_max_scroll_offset(count);

    if (wheel_move > 0.0f) {
        *scroll_offset -= 1;
    } else if (wheel_move < 0.0f) {
        *scroll_offset += 1;
    }

    if (*scroll_offset < 0) {
        *scroll_offset = 0;
    }

    if (*scroll_offset > max_scroll_offset) {
        *scroll_offset = max_scroll_offset;
    }
}

static int get_clicked_row_index(size_t count,
                                 const FileEntry *entries,
                                 const char *search_query,
                                 int scroll_offset,
                                 bool mouse_clicked,
                                 Vector2 mouse_position)
{
    size_t filtered_count = get_filtered_entry_count(entries, count, search_query);
    size_t visible_count = get_visible_row_count(filtered_count, scroll_offset);
    size_t i;

    if (!mouse_clicked) {
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
            return get_filtered_entry_index(entries,
                                            count,
                                            search_query,
                                            scroll_offset + (int)i);
        }
    }

    return -1;
}

static bool get_clicked_path_segment(const char *current_path,
                                     bool mouse_clicked,
                                     Vector2 mouse_position,
                                     char *next_path)
{
    char built_path[CF_PATH_MAX];
    const char *cursor = current_path;
    int x = PATH_X;

    if (!mouse_clicked) {
        return false;
    }

    built_path[0] = '\0';

    if (current_path[0] == '/') {
        int slash_width = MeasureText("/", PATH_FONT_SIZE);
        Rectangle root_bounds = {(float)x, (float)PATH_Y, (float)slash_width, (float)PATH_FONT_SIZE};

        snprintf(built_path, sizeof(built_path), "/");

        if (CheckCollisionPointRec(mouse_position, root_bounds)) {
            snprintf(next_path, CF_PATH_MAX, "/");
            return true;
        }

        x += slash_width + 6;
        cursor++;
    }

    while (*cursor != '\0') {
        char segment[CF_NAME_MAX];
        size_t segment_length = 0;
        int segment_width;
        Rectangle segment_bounds;

        while (cursor[segment_length] != '\0' && cursor[segment_length] != '/') {
            segment_length++;
        }

        if (segment_length >= sizeof(segment)) {
            segment_length = sizeof(segment) - 1;
        }

        memcpy(segment, cursor, segment_length);
        segment[segment_length] = '\0';

        if (strcmp(built_path, "/") == 0) {
            snprintf(next_path, CF_PATH_MAX, "/%s", segment);
        } else if (built_path[0] == '\0') {
            snprintf(next_path, CF_PATH_MAX, "%s", segment);
        } else {
            snprintf(next_path, CF_PATH_MAX, "%s/%s", built_path, segment);
        }

        segment_width = MeasureText(segment, PATH_FONT_SIZE);
        segment_bounds = (Rectangle){(float)x, (float)PATH_Y, (float)segment_width, (float)PATH_FONT_SIZE};

        if (CheckCollisionPointRec(mouse_position, segment_bounds)) {
            return true;
        }

        snprintf(built_path, sizeof(built_path), "%s", next_path);
        x += segment_width;

        cursor += segment_length;
        if (*cursor == '/') {
            x += MeasureText("/", PATH_FONT_SIZE) + 6;
            cursor++;
        }
    }

    return false;
}

static void draw_file_list(const FileEntry *entries,
                           size_t count,
                           const char *search_query,
                           int selected_index,
                           int scroll_offset)
{
    size_t filtered_count = get_filtered_entry_count(entries, count, search_query);
    size_t visible_count = get_visible_row_count(filtered_count, scroll_offset);
    size_t i;

    DrawText("Name", 40, LIST_TOP, 20, BLACK);
    DrawText("Size", 420, LIST_TOP, 20, BLACK);
    DrawText("Type", 540, LIST_TOP, 20, BLACK);
    DrawText("Modified", 680, LIST_TOP, 20, BLACK);

    for (i = 0; i < visible_count; ++i) {
        int filtered_index = scroll_offset + (int)i;
        int entry_index = get_filtered_entry_index(entries, count, search_query, filtered_index);
        char name_text[CF_NAME_MAX + 16];
        char name_display[CF_NAME_MAX + 16];
        char size_text[64];
        char size_display[64];
        char type_display[CF_TYPE_MAX + 4];
        char modified_display[CF_TIME_STR_MAX + 4];
        int row_y = ROW_START_Y + ((int)i * ROW_HEIGHT);
        Color row_color;
        Color detail_color = DARKGRAY;

        if (entry_index < 0) {
            continue;
        }

        row_color = entries[entry_index].is_directory ? BLUE : DARKGRAY;

        if (entry_index == selected_index) {
            DrawRectangle(35, row_y - 2, WINDOW_WIDTH - 70, ROW_HEIGHT, SKYBLUE);
            row_color = DARKBLUE;
            detail_color = BLACK;
        }

        snprintf(name_text,
                 sizeof(name_text),
                 "%s %s",
                 entries[entry_index].is_directory ? "[DIR]" : "[FILE]",
                 entries[entry_index].name);

        if (entries[entry_index].is_directory) {
            snprintf(size_text, sizeof(size_text), "--");
        } else {
            snprintf(size_text, sizeof(size_text), "%zu", entries[entry_index].size);
        }

        truncate_text_to_width(name_text,
                               name_display,
                               sizeof(name_display),
                               ROW_FONT_SIZE,
                               NAME_COLUMN_WIDTH);
        truncate_text_to_width(size_text,
                               size_display,
                               sizeof(size_display),
                               ROW_FONT_SIZE,
                               SIZE_COLUMN_WIDTH);
        truncate_text_to_width(entries[entry_index].type,
                               type_display,
                               sizeof(type_display),
                               ROW_FONT_SIZE,
                               TYPE_COLUMN_WIDTH);
        truncate_text_to_width(entries[entry_index].modified_time_str,
                               modified_display,
                               sizeof(modified_display),
                               ROW_FONT_SIZE,
                               MODIFIED_COLUMN_WIDTH);

        DrawText(name_display, 40, row_y, ROW_FONT_SIZE, row_color);
        DrawText(size_display, 420, row_y, ROW_FONT_SIZE, detail_color);
        DrawText(type_display, 540, row_y, ROW_FONT_SIZE, detail_color);
        DrawText(modified_display, 680, row_y, ROW_FONT_SIZE, detail_color);
    }
}

static void draw_back_button(void)
{
    Rectangle button_bounds = get_back_button_bounds();

    DrawRectangleRec(button_bounds, GRAY);
    DrawRectangleLines((int)button_bounds.x,
                       (int)button_bounds.y,
                       (int)button_bounds.width,
                       (int)button_bounds.height,
                       BLACK);
    DrawText("Back", (int)button_bounds.x + 10, (int)button_bounds.y + 6, 20, RAYWHITE);
}

static void draw_refresh_button(void)
{
    Rectangle button_bounds = get_refresh_button_bounds();

    DrawRectangleRec(button_bounds, GRAY);
    DrawRectangleLines((int)button_bounds.x,
                       (int)button_bounds.y,
                       (int)button_bounds.width,
                       (int)button_bounds.height,
                       BLACK);
    DrawText("Refresh", (int)button_bounds.x + 8, (int)button_bounds.y + 6, 20, RAYWHITE);
}

static void draw_search_box(const char *search_query, bool search_active)
{
    Rectangle box_bounds = get_search_box_bounds();
    char display_text[CF_SEARCH_MAX + 16];
    Color border_color = search_active ? BLUE : BLACK;

    DrawRectangleRec(box_bounds, RAYWHITE);
    DrawRectangleLines((int)box_bounds.x,
                       (int)box_bounds.y,
                       (int)box_bounds.width,
                       (int)box_bounds.height,
                       border_color);

    if (search_active) {
        DrawRectangleLines((int)box_bounds.x - 1,
                           (int)box_bounds.y - 1,
                           (int)box_bounds.width + 2,
                           (int)box_bounds.height + 2,
                           SKYBLUE);
        DrawRectangleLines((int)box_bounds.x + 1,
                           (int)box_bounds.y + 1,
                           (int)box_bounds.width - 2,
                           (int)box_bounds.height - 2,
                           BLUE);
    }

    if (search_query[0] == '\0') {
        snprintf(display_text, sizeof(display_text), "Search...");
        DrawText(display_text, SEARCH_BOX_X + 10, SEARCH_BOX_Y + 6, 20, GRAY);
    } else {
        truncate_text_to_width(search_query,
                               display_text,
                               sizeof(display_text),
                               20,
                               SEARCH_BOX_WIDTH - 20);
        DrawText(display_text, SEARCH_BOX_X + 10, SEARCH_BOX_Y + 6, 20, BLACK);
    }
}

static void draw_sort_button(SortMode sort_mode)
{
    Rectangle button_bounds = get_sort_button_bounds();
    char button_text[64];
    char display_text[64];

    DrawRectangleRec(button_bounds, GRAY);
    DrawRectangleLines((int)button_bounds.x,
                       (int)button_bounds.y,
                       (int)button_bounds.width,
                       (int)button_bounds.height,
                       BLACK);

    snprintf(button_text, sizeof(button_text), "Sort: %s", get_sort_mode_name(sort_mode));
    truncate_text_to_width(button_text, display_text, sizeof(display_text), 20, SORT_BUTTON_WIDTH - 16);
    DrawText(display_text, SORT_BUTTON_X + 8, SORT_BUTTON_Y + 6, 20, RAYWHITE);
}

static void draw_copy_button(void)
{
    Rectangle button_bounds = get_copy_button_bounds();

    DrawRectangleRec(button_bounds, GRAY);
    DrawRectangleLines((int)button_bounds.x,
                       (int)button_bounds.y,
                       (int)button_bounds.width,
                       (int)button_bounds.height,
                       BLACK);
    DrawText("Copy", COPY_BUTTON_X + 18, COPY_BUTTON_Y + 6, 20, RAYWHITE);
}

static void draw_paste_button(void)
{
    Rectangle button_bounds = get_paste_button_bounds();

    DrawRectangleRec(button_bounds, GRAY);
    DrawRectangleLines((int)button_bounds.x,
                       (int)button_bounds.y,
                       (int)button_bounds.width,
                       (int)button_bounds.height,
                       BLACK);
    DrawText("Paste", PASTE_BUTTON_X + 15, PASTE_BUTTON_Y + 6, 20, RAYWHITE);
}

static void draw_delete_button(void)
{
    Rectangle button_bounds = get_delete_button_bounds();

    DrawRectangleRec(button_bounds, GRAY);
    DrawRectangleLines((int)button_bounds.x,
                       (int)button_bounds.y,
                       (int)button_bounds.width,
                       (int)button_bounds.height,
                       BLACK);
    DrawText("Delete", DELETE_BUTTON_X + 12, DELETE_BUTTON_Y + 6, 20, RAYWHITE);
}

static void draw_new_folder_button(void)
{
    Rectangle button_bounds = get_new_folder_button_bounds();

    DrawRectangleRec(button_bounds, GRAY);
    DrawRectangleLines((int)button_bounds.x,
                       (int)button_bounds.y,
                       (int)button_bounds.width,
                       (int)button_bounds.height,
                       BLACK);
    DrawText("New Folder", NEW_FOLDER_BUTTON_X + 12, NEW_FOLDER_BUTTON_Y + 6, 20, RAYWHITE);
}

static void draw_rename_button(void)
{
    Rectangle button_bounds = get_rename_button_bounds();

    DrawRectangleRec(button_bounds, GRAY);
    DrawRectangleLines((int)button_bounds.x,
                       (int)button_bounds.y,
                       (int)button_bounds.width,
                       (int)button_bounds.height,
                       BLACK);
    DrawText("Rename", RENAME_BUTTON_X + 12, RENAME_BUTTON_Y + 6, 20, RAYWHITE);
}

static void draw_rename_box(const char *rename_text)
{
    Rectangle box_bounds = get_rename_box_bounds();
    char display_text[CF_NAME_MAX + 16];

    DrawText("Rename to:", INFO_X, INFO_Y, 20, BLACK);
    DrawRectangleRec(box_bounds, RAYWHITE);
    DrawRectangleLines((int)box_bounds.x,
                       (int)box_bounds.y,
                       (int)box_bounds.width,
                       (int)box_bounds.height,
                       BLUE);
    DrawRectangleLines((int)box_bounds.x - 1,
                       (int)box_bounds.y - 1,
                       (int)box_bounds.width + 2,
                       (int)box_bounds.height + 2,
                       SKYBLUE);

    truncate_text_to_width(rename_text,
                           display_text,
                           sizeof(display_text),
                           20,
                           RENAME_BOX_WIDTH - 20);
    DrawText(display_text, RENAME_BOX_X + 10, RENAME_BOX_Y + 6, 20, BLACK);
}

static bool build_paste_destination_path(const char *source_path,
                                         const char *target_directory,
                                         char *destination_path,
                                         size_t destination_size)
{
    const char *last_slash = strrchr(source_path, '/');
    const char *filename = source_path;
    const char *last_dot;
    char name_part[CF_NAME_MAX];
    char extension_part[CF_NAME_MAX];
    size_t name_length;
    int written;

    if (last_slash != NULL) {
        filename = last_slash + 1;
    }

    last_dot = strrchr(filename, '.');
    if (last_dot != NULL && last_dot != filename) {
        name_length = (size_t)(last_dot - filename);
        if (name_length >= sizeof(name_part)) {
            return false;
        }

        memcpy(name_part, filename, name_length);
        name_part[name_length] = '\0';
        snprintf(extension_part, sizeof(extension_part), "%s", last_dot);
    } else {
        snprintf(name_part, sizeof(name_part), "%s", filename);
        extension_part[0] = '\0';
    }

    written = snprintf(destination_path,
                       destination_size,
                       "%s/%s_copy%s",
                       target_directory,
                       name_part,
                       extension_part);

    return written >= 0 && (size_t)written < destination_size;
}

static bool create_new_folder_in_directory(const char *current_path,
                                           char *created_path,
                                           size_t created_path_size)
{
    int index;

    for (index = 0; index < 1000; ++index) {
        int written;

        if (index == 0) {
            written = snprintf(created_path, created_path_size, "%s/NewFolder", current_path);
        } else {
            written = snprintf(created_path,
                               created_path_size,
                               "%s/NewFolder_%d",
                               current_path,
                               index);
        }

        if (written < 0 || (size_t)written >= created_path_size) {
            errno = ENAMETOOLONG;
            return false;
        }

        if (mkdir(created_path, 0755) == 0) {
            return true;
        }

        if (errno != EEXIST) {
            return false;
        }
    }

    errno = EEXIST;
    return false;
}

static bool build_renamed_path(const char *current_path,
                               const char *new_name,
                               char *renamed_path,
                               size_t renamed_path_size)
{
    int written;

    if (strcmp(current_path, "/") == 0) {
        written = snprintf(renamed_path, renamed_path_size, "/%s", new_name);
    } else {
        written = snprintf(renamed_path, renamed_path_size, "%s/%s", current_path, new_name);
    }

    return written >= 0 && (size_t)written < renamed_path_size;
}

static void update_search_input(char *search_query, bool search_active, int *scroll_offset)
{
    int character = 0;
    size_t length;

    if (!search_active) {
        return;
    }

    character = GetCharPressed();
    while (character > 0) {
        length = strlen(search_query);

        if (character >= 32 && character <= 126 && length < (size_t)(CF_SEARCH_MAX - 1)) {
            search_query[length] = (char)character;
            search_query[length + 1] = '\0';
            *scroll_offset = 0;
        }

        character = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        length = strlen(search_query);

        if (length > 0) {
            search_query[length - 1] = '\0';
            *scroll_offset = 0;
        }
    }
}

static void update_rename_input(char *rename_text)
{
    int character = GetCharPressed();
    size_t length;

    while (character > 0) {
        length = strlen(rename_text);

        if (character >= 32 && character <= 126 && character != '/' &&
            length < (size_t)(CF_NAME_MAX - 1)) {
            rename_text[length] = (char)character;
            rename_text[length + 1] = '\0';
        }

        character = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        length = strlen(rename_text);

        if (length > 0) {
            rename_text[length - 1] = '\0';
        }
    }
}

static void draw_path_segments(const char *current_path)
{
    const char *cursor = current_path;
    int x = PATH_X;

    if (current_path[0] == '/') {
        DrawText("/", x, PATH_Y, PATH_FONT_SIZE, BLUE);
        x += MeasureText("/", PATH_FONT_SIZE) + 6;
        cursor++;
    }

    while (*cursor != '\0') {
        char segment[CF_NAME_MAX];
        size_t segment_length = 0;

        while (cursor[segment_length] != '\0' && cursor[segment_length] != '/') {
            segment_length++;
        }

        if (segment_length >= sizeof(segment)) {
            segment_length = sizeof(segment) - 1;
        }

        memcpy(segment, cursor, segment_length);
        segment[segment_length] = '\0';

        DrawText(segment, x, PATH_Y, PATH_FONT_SIZE, BLUE);
        x += MeasureText(segment, PATH_FONT_SIZE);

        cursor += segment_length;
        if (*cursor == '/') {
            DrawText("/", x + 2, PATH_Y, PATH_FONT_SIZE, DARKGRAY);
            x += MeasureText("/", PATH_FONT_SIZE) + 6;
            cursor++;
        }
    }
}

int main(void)
{
    char current_path[CF_PATH_MAX];
    FileEntry *entries = NULL;
    size_t count = 0;
    size_t filtered_count = 0;
    size_t visible_filtered_count = 0;
    int selected_index = -1;
    int scroll_offset = 0;
    bool search_active = false;
    bool rename_mode = false;
    SortMode sort_mode = SORT_NAME;
    char copied_file_path[CF_PATH_MAX];
    char rename_text[CF_NAME_MAX];
    char search_query[CF_SEARCH_MAX];
    char status_text[CF_STATUS_MAX];

    if (getcwd(current_path, sizeof(current_path)) == NULL) {
        fprintf(stderr, "Failed to get current working directory: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (load_directory(current_path, &entries, &count) != 0) {
        fprintf(stderr, "Failed to load directory \"%s\": %s\n", current_path, strerror(errno));
        return EXIT_FAILURE;
    }

    if (sort_entries(entries, count, sort_mode) != 0) {
        fprintf(stderr, "Failed to sort directory \"%s\": %s\n", current_path, strerror(errno));
        free_file_entries(entries);
        return EXIT_FAILURE;
    }

    copied_file_path[0] = '\0';
    rename_text[0] = '\0';
    search_query[0] = '\0';
    status_text[0] = '\0';

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "C-Files");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        bool mouse_clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        Vector2 mouse_position = GetMousePosition();
        bool search_clicked = is_search_box_clicked(mouse_clicked, mouse_position);

        if (mouse_clicked && !rename_mode) {
            search_active = search_clicked;
        }

        if (rename_mode) {
            update_rename_input(rename_text);
        } else {
            update_search_input(search_query, search_active, &scroll_offset);
        }
        filtered_count = get_filtered_entry_count(entries, count, search_query);

        if (!rename_mode) {
            update_scroll_offset(filtered_count, &scroll_offset);
        }
        visible_filtered_count = get_visible_row_count(filtered_count, scroll_offset);

        if (rename_mode) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                rename_mode = false;
                rename_text[0] = '\0';
                snprintf(status_text, sizeof(status_text), "Rename canceled");
            } else if (IsKeyPressed(KEY_ENTER)) {
                if (selected_index < 0 || (size_t)selected_index >= count) {
                    rename_mode = false;
                    snprintf(status_text, sizeof(status_text), "No item selected to rename");
                } else if (rename_text[0] == '\0') {
                    snprintf(status_text, sizeof(status_text), "Name cannot be empty");
                } else {
                    char renamed_path[CF_PATH_MAX];

                    if (!build_renamed_path(current_path,
                                            rename_text,
                                            renamed_path,
                                            sizeof(renamed_path))) {
                        snprintf(status_text, sizeof(status_text), "Rename path is too long");
                    } else if (rename(entries[selected_index].full_path, renamed_path) != 0) {
                        snprintf(status_text, sizeof(status_text), "Failed to rename item");
                    } else {
                        rename_mode = false;
                        rename_text[0] = '\0';

                        if (!reload_directory(current_path,
                                              current_path,
                                              &entries,
                                              &count,
                                              sort_mode,
                                              &selected_index,
                                              &scroll_offset,
                                              status_text,
                                              "Renamed item",
                                              "Failed to reload after rename")) {
                            selected_index = -1;
                        }
                    }
                }
            }
        } else if (is_back_button_clicked(mouse_clicked, mouse_position)) {
            char old_path[CF_PATH_MAX];

            snprintf(old_path, sizeof(old_path), "%s", current_path);

            if (go_up_directory(current_path) == 0) {
                if (!reload_directory(current_path,
                                      current_path,
                                      &entries,
                                      &count,
                                      sort_mode,
                                      &selected_index,
                                      &scroll_offset,
                                      status_text,
                                      "",
                                      "Failed to load parent directory")) {
                    snprintf(current_path, sizeof(current_path), "%s", old_path);
                    selected_index = -1;
                }
            } else {
                snprintf(status_text, sizeof(status_text), "Failed to go up");
            }
        }

        if (!rename_mode && is_refresh_button_clicked(mouse_clicked, mouse_position)) {
            if (!reload_directory(current_path,
                                  current_path,
                                  &entries,
                                  &count,
                                  sort_mode,
                                  &selected_index,
                                  &scroll_offset,
                                  status_text,
                                  "",
                                  "Failed to refresh directory")) {
                selected_index = -1;
            }
        }

        if (!rename_mode && is_sort_button_clicked(mouse_clicked, mouse_position)) {
            sort_mode = get_next_sort_mode(sort_mode);

            if (sort_entries(entries, count, sort_mode) != 0) {
                snprintf(status_text, sizeof(status_text), "Failed to sort directory");
            } else {
                selected_index = -1;
                scroll_offset = 0;
                status_text[0] = '\0';
            }
        }

        if (!rename_mode && is_copy_button_clicked(mouse_clicked, mouse_position)) {
            if (selected_index < 0 || (size_t)selected_index >= count) {
                snprintf(status_text, sizeof(status_text), "Select a file to copy");
            } else if (entries[selected_index].is_directory) {
                snprintf(status_text, sizeof(status_text), "Cannot copy directories yet");
            } else {
                struct stat st;

                if (stat(entries[selected_index].full_path, &st) != 0 || !S_ISREG(st.st_mode)) {
                    snprintf(status_text, sizeof(status_text), "Only regular files can be copied");
                } else {
                    snprintf(copied_file_path,
                             sizeof(copied_file_path),
                             "%s",
                             entries[selected_index].full_path);
                    snprintf(status_text, sizeof(status_text), "File copied. Click Paste");
                }
            }
        }

        if (!rename_mode && is_paste_button_clicked(mouse_clicked, mouse_position)) {
            if (copied_file_path[0] == '\0') {
                snprintf(status_text, sizeof(status_text), "No copied file to paste");
            } else {
                struct stat st;
                char destination_path[CF_PATH_MAX];

                if (stat(copied_file_path, &st) != 0 || !S_ISREG(st.st_mode)) {
                    snprintf(status_text, sizeof(status_text), "Copied item is not a regular file");
                } else if (!build_paste_destination_path(copied_file_path,
                                                         current_path,
                                                         destination_path,
                                                         sizeof(destination_path))) {
                    snprintf(status_text, sizeof(status_text), "Paste path is too long");
                } else if (copy_file(copied_file_path, destination_path) != 0) {
                    snprintf(status_text, sizeof(status_text), "Failed to paste file");
                } else if (!reload_directory(current_path,
                                             current_path,
                                             &entries,
                                             &count,
                                             sort_mode,
                                             &selected_index,
                                             &scroll_offset,
                                             status_text,
                                             "Pasted file",
                                             "Failed to reload after paste")) {
                    selected_index = -1;
                }
            }
        }

        if (!rename_mode && is_delete_button_clicked(mouse_clicked, mouse_position)) {
            if (selected_index < 0 || (size_t)selected_index >= count) {
                snprintf(status_text, sizeof(status_text), "Select a file to delete");
            } else if (entries[selected_index].is_directory) {
                snprintf(status_text, sizeof(status_text), "Cannot delete directories yet");
            } else {
                struct stat st;

                if (stat(entries[selected_index].full_path, &st) != 0 || !S_ISREG(st.st_mode)) {
                    snprintf(status_text, sizeof(status_text), "Only regular files can be deleted");
                } else if (delete_file(entries[selected_index].full_path) != 0) {
                    snprintf(status_text, sizeof(status_text), "Failed to delete file");
                } else if (!reload_directory(current_path,
                                             current_path,
                                             &entries,
                                             &count,
                                             sort_mode,
                                             &selected_index,
                                             &scroll_offset,
                                             status_text,
                                             "Deleted file",
                                             "Failed to reload after delete")) {
                    selected_index = -1;
                }
            }
        }

        if (!rename_mode && is_new_folder_button_clicked(mouse_clicked, mouse_position)) {
            char created_path[CF_PATH_MAX];

            if (!create_new_folder_in_directory(current_path, created_path, sizeof(created_path))) {
                snprintf(status_text, sizeof(status_text), "Failed to create folder");
            } else if (!reload_directory(current_path,
                                         current_path,
                                         &entries,
                                         &count,
                                         sort_mode,
                                         &selected_index,
                                         &scroll_offset,
                                         status_text,
                                         "Created folder",
                                         "Failed to reload after folder create")) {
                selected_index = -1;
            }
        }

        if (!rename_mode && is_rename_button_clicked(mouse_clicked, mouse_position)) {
            if (selected_index < 0 || (size_t)selected_index >= count) {
                snprintf(status_text, sizeof(status_text), "Select an item to rename");
            } else {
                snprintf(rename_text, sizeof(rename_text), "%s", entries[selected_index].name);
                rename_mode = true;
                search_active = false;
                snprintf(status_text,
                         sizeof(status_text),
                         "Rename mode: Enter to confirm, Esc to cancel");
            }
        }

        if (!rename_mode) {
            char next_path[CF_PATH_MAX];

            if (get_clicked_path_segment(current_path, mouse_clicked, mouse_position, next_path) &&
                strcmp(next_path, current_path) != 0) {
                if (!reload_directory(current_path,
                                      next_path,
                                      &entries,
                                      &count,
                                      sort_mode,
                                      &selected_index,
                                      &scroll_offset,
                                      status_text,
                                      "",
                                      "Failed to load selected path")) {
                    selected_index = -1;
                }
            }
        }

        if (!rename_mode) {
            int clicked_index = get_clicked_row_index(count,
                                                      entries,
                                                      search_query,
                                                      scroll_offset,
                                                      mouse_clicked,
                                                      mouse_position);

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
                                              sort_mode,
                                              &selected_index,
                                              &scroll_offset,
                                              status_text,
                                              "",
                                              "Failed to load directory")) {
                            selected_index = -1;
                        }
                    } else {
                        snprintf(status_text, sizeof(status_text), "Failed to enter directory");
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("C-Files", TITLE_X, TITLE_Y, 32, BLACK);
        draw_back_button();
        draw_refresh_button();
        draw_search_box(search_query, search_active);
        draw_sort_button(sort_mode);
        draw_copy_button();
        draw_paste_button();
        draw_delete_button();
        draw_new_folder_button();
        draw_rename_button();
        draw_path_segments(current_path);

        if (rename_mode) {
            draw_rename_box(rename_text);
        } else {
            char count_text[128];

            snprintf(count_text,
                     sizeof(count_text),
                     "Showing %zu of %zu entries",
                     visible_filtered_count,
                     count);
            DrawText(count_text, INFO_X, INFO_Y, 20, BLACK);
        }

        if (status_text[0] != '\0') {
            DrawText(status_text, INFO_X, STATUS_Y, 20, MAROON);
        }
        draw_file_list(entries, count, search_query, selected_index, scroll_offset);

        EndDrawing();
    }

    free_file_entries(entries);
    CloseWindow();

    return EXIT_SUCCESS;
}
