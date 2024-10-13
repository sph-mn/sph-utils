#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>

#define path_max 4096

// function to print errors
void print_errno() {
    perror("");
}

// function to ensure directory structure exists
void ensure_directory_structure(const char* path) {
    char tmp[path_max];
    snprintf(tmp, sizeof(tmp), "%s", path);
    size_t len = strlen(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    }
    mkdir(tmp, S_IRWXU);
}

// function to find a numeric directory in the path
int find_numeric_directory_in_path(char* path, int* dir_start, int* dir_len) {
    int len = strlen(path);
    int i = len - 1;
    int num_start = -1, num_end = -1;
    while (i >= 0) {
        if (path[i] == '/') {
            if (num_end != -1) {
                // found a numeric directory
                num_start = i + 1;
                *dir_start = num_start;
                *dir_len = num_end - num_start + 1;
                return 1;
            }
        } else if (isdigit(path[i])) {
            if (num_end == -1) {
                num_end = i;
            }
        } else {
            num_end = -1;
        }
        i--;
    }
    // no numeric directory found
    return 0;
}

// function to parse command-line arguments
int cli(int argc, char** argv, int16_t* new_rating, uint8_t* options) {
    char help_text[] = "arguments: [-m] [+-]rating path ...\n";
    if (argc < 3) {
        write(2, help_text, strlen(help_text));
        exit(1);
    }
    int i = 1;
    *options = 0;
    if (strcmp(argv[i], "-m") == 0) {
        *options = 1; // modify existing rating
        i += 1;
        if (i >= argc) {
            write(2, help_text, strlen(help_text));
            exit(1);
        }
    }
    char* rating_str = argv[i];
    int rating = atoi(rating_str);
    *new_rating = rating;
    i += 1;
    return i;
}

int main(int argc, char** argv) {
    int16_t new_rating;
    uint8_t options;
    char old_path[path_max];
    char new_path[path_max];

    int i = cli(argc, argv, &new_rating, &options);
    for (; i < argc; i++) {
        if (!realpath(argv[i], old_path)) {
            perror("realpath");
            continue;
        }
        int dir_start, dir_len;
        int found = find_numeric_directory_in_path(old_path, &dir_start, &dir_len);
        if (found) {
            // numeric directory found in the path
            char numeric_dir[dir_len + 1];
            memcpy(numeric_dir, old_path + dir_start, dir_len);
            numeric_dir[dir_len] = '\0';
            int old_rating = atoi(numeric_dir);
            int modified_rating;
            if (options == 1) {
                // modify existing rating
                modified_rating = old_rating + new_rating;
            } else {
                // replace with new rating
                modified_rating = new_rating;
            }
            // build the new path
            // copy up to dir_start
            memcpy(new_path, old_path, dir_start);
            new_path[dir_start] = '\0';
            // append modified rating
            char modified_dir[16];
            sprintf(modified_dir, "%d", modified_rating);
            strcat(new_path, modified_dir);
            // append rest of the path
            strcat(new_path, old_path + dir_start + dir_len);
        } else {
            // no numeric directory found
            // create numeric directory in current working directory
            char cwd[path_max];
            if (!getcwd(cwd, sizeof(cwd))) {
                perror("getcwd");
                continue;
            }
            size_t cwd_len = strlen(cwd);
            char* relative_path;
            if (strcmp(cwd, "/") == 0) {
                relative_path = old_path;
            } else if (strncmp(old_path, cwd, cwd_len) == 0 && old_path[cwd_len] == '/') {
                // old_path is under cwd
                relative_path = old_path + cwd_len;
            } else {
                // old_path is not under cwd
                relative_path = old_path;
            }
            snprintf(new_path, sizeof(new_path), "%s/%d%s", cwd, new_rating, relative_path);
        }

        // ensure the destination directory exists
        char* dir_end = strrchr(new_path, '/');
        if (dir_end) {
            *dir_end = '\0';
            ensure_directory_structure(new_path);
            *dir_end = '/';
        }

        // check if the target file exists
        struct stat st;
        if (stat(new_path, &st) == 0) {
            fprintf(stderr, "target file %s already exists\n", new_path);
            continue;
        }

        // move the file
        if (rename(old_path, new_path) != 0) {
            perror("rename");
            continue;
        } else {
            printf("moved %s to %s\n", old_path, new_path);
        }
    }
    return 0;
}
