#ifndef CFILES_NAV_H
#define CFILES_NAV_H

/* Returns 0 on success, -1 on failure and leaves errno set. */
int go_to_directory(char *current_path, const char *next_dir);
int go_up_directory(char *current_path);

#endif /* CFILES_NAV_H */
