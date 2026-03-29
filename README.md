# C-Files

A desktop file explorer written in **C** with **raylib** that lets users browse directories, inspect file metadata, search by filename, sort entries, and perform core file operations through a simple GUI.

This project is also a **systems programming** project because it works directly with the operating system and filesystem rather than through a high-level runtime. It uses low-level C code and POSIX-style system interfaces to read directories, inspect files, rename items, create folders, and delete paths, while also managing memory and application state manually. In other words, it is software that interacts closely with OS resources like files, paths, directories, and process working directories.

## Features

- Browse the current working directory in a desktop GUI
- Move into folders and go back to parent directories
- Click path segments to jump to earlier parts of the current path
- View file metadata including:
  - name
  - size
  - type
  - last modified time
- Search entries by filename
- Toggle hidden files on and off
- Sort entries by:
  - name
  - size
  - type
  - modified time
- Copy and paste regular files
- Rename files and folders
- Delete files and folders with confirmation
- Create new folders
- Refresh the current directory view
- Scroll through long directory listings

## Why this project is useful


C-Files turns common terminal-style filesystem tasks into a visual interface while still keeping the project grounded in low-level C and systems concepts. It is useful as both:

- a more user-friendly file explorer for small local workflows with a cleaner interface and less visual clutter than a terminal-based workflow
- core file actions (copy, paste, delete, rename, navigation) are centralized and accessible without memorizing commands


## Tech stack

- **Language:** C
- **Graphics / GUI:** raylib
- **Concepts used:** systems programming, filesystem operations, manual memory management, path handling, sorting, searching, event-driven UI

## How it works

At a high level, the program:

1. Gets the current working directory
2. Loads directory entries into memory
3. Sorts and filters them based on user settings
4. Draws the interface with raylib
5. Responds to mouse and keyboard input for navigation and file actions
6. Reloads the directory after operations like rename, delete, paste, and folder creation

## Project structure

Based on the current code organization, the project is split into focused modules:

- `src/main.c` – UI flow, input handling, drawing, and application state
- `src/file_ops.c` – filesystem operations such as loading directories, copying, deleting, and freeing entries
- `src/nav.c` – directory navigation helpers
- `src/search.c` – filename search/filter logic
- `src/sort.c` – sorting logic for file entries
- `include/` – shared headers and data models

## Build and run

### Requirements

- C compiler with C11 support
- `make`
- `raylib`

### Run

```bash
make
```

If your `Makefile` is set up like the current project output suggests, this will build the executable and run it.

## Usage

- **Back**: move to the parent directory
- **Refresh**: reload the current directory
- **Hidden: On/Off**: toggle hidden files
- **Sort**: cycle through available sort modes
- **Search box**: filter entries by name
- **Single click**: select a file or folder
- **Click selected folder again**: enter the folder
- **Copy / Paste**: duplicate regular files into the current directory
- **Delete**: remove the selected file or folder after confirmation
- **New Folder**: create a new folder in the current directory
- **Rename**: rename the selected file or folder

## What this project demonstrates

This project demonstrates:

- systems programming in C
- working with directories and file metadata
- integrating a low-level language with a graphics library
- building an event-driven desktop application without a heavyweight framework
- organizing a C codebase into modules with clear responsibilities

## Possible future improvements

- Open and preview text files
- Recursive directory copy
- Cut / move support
- Keyboard shortcuts
- Better error reporting with specific system error messages
- File icons or improved visual styling
- Cross-platform packaging and setup instructions
