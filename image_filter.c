#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "bitmap.h"


#define ERROR_MESSAGE "Warning: one or more filter had an error, so the output image may not be correct.\n"
#define SUCCESS_MESSAGE "Image transformed successfully!\n"


/*
 * Check whether the given command is a valid image filter, and if so,
 * run the process.
 *
 * We've given you this function to illustrate the expected command-line
 * arguments for image_filter. No further error-checking is required for
 * the child processes.
 */
void run_command(const char *cmd) {
    if (strcmp(cmd, "copy") == 0 || strcmp(cmd, "./copy") == 0 ||
        strcmp(cmd, "greyscale") == 0 || strcmp(cmd, "./greyscale") == 0 ||
        strcmp(cmd, "gaussian_blur") == 0 || strcmp(cmd, "./gaussian_blur") == 0 ||
        strcmp(cmd, "edge_detection") == 0 || strcmp(cmd, "./edge_detection") == 0) {
        execl(cmd, cmd, NULL);
    } else if (strncmp(cmd, "scale", 5) == 0) {
        // Note: the numeric argument starts at cmd[6]
        execl("scale", "scale", cmd + 6, NULL);
    } else if (strncmp(cmd, "./scale", 7) == 0) {
        // Note: the numeric argument starts at cmd[8]
        execl("./scale", "./scale", cmd + 8, NULL);
    } else {
        fprintf(stderr, "Invalid command '%s'\n", cmd);
        exit(1);
    }
}


/*
 * Apply filters using child processes.
 *
 */
void apply_filters(const char *input_file, const char *output_file, char *const filters[]) {
    // Read the input image header
    Bitmap *bmp = read_header();

    // Create pipes for communication between processes
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("Pipe creation failed");
        exit(EXIT_FAILURE);
    }

    // Fork a child process
    pid_t child_pid = fork();

    if (child_pid == -1) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        // Child process: apply filters
        close(pipe_fd[0]); // Close unused read end of the pipe

        // Redirect stdout to the pipe
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]); // Close write end of the pipe

        // Apply filters
        run_filter(copy_filter, 1); // Example: Apply a default filter (copy)
        for (int i = 2; filters[i] != NULL; i++) {
            run_command(filters[i]);
        }

        exit(EXIT_SUCCESS);
    } else {
        // Parent process
        close(pipe_fd[1]); // Close unused write end of the pipe

        // Read data from the child process
        char buffer[1024];
        ssize_t bytes_read;
        while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer))) > 0) {
            // Write the data to stdout (or file)
            fwrite(buffer, sizeof(char), bytes_read, stdout);
        }

        // Wait for the child process to finish
        int status;
        waitpid(child_pid, &status, 0);

        // Check the exit status of the child process
        if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
            fprintf(stderr, SUCCESS_MESSAGE);
        } else {
            fprintf(stderr, ERROR_MESSAGE);
        }

        // Clean up
        close(pipe_fd[0]);
        free_bitmap(bmp);
    }
}


// TODO: Complete this function.
int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s input output [filter ...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];

    // Pass the filters as an array to apply_filters
    apply_filters(input_file, output_file, argv);

    return 0;
}
